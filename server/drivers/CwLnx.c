/* #define COUNT -1 */
/*  This is the LCDproc driver for Cwlinux devices (http://www.cwlinux.com)

        Copyright (C) 1999, William Ferrell and Scott Scriven
                      2001, Philip Pokorny
                      2001, David Douthitt
                      2001, Joris Robijn
	              2001, Eddie Sheldrake
	              2001, Rene Wagner
	              2002, Mike Patnode
	              2002, Luis Llorente

        Copyright (C) 2002, Andrew Ip
                      2002, David Glaude

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "lcd.h"
#include "CwLnx.h"
#include "render.h"
#include "input.h"
#include "shared/str.h"
#include "shared/report.h"
#include "server/configfile.h"

static int custom = 0;
typedef enum {
    hbar = 1,
    vbar = 2,
    bign = 4,
    beat = 8
} custom_type;

static int fd;
static char *backingstore = NULL;
static char pause_key = CWLNX_DEF_PAUSE_KEY, back_key = CWLNX_DEF_BACK_KEY;
static char forward_key = CWLNX_DEF_FORWARD_KEY, main_menu_key = CWLNX_DEF_MAIN_MENU_KEY;
static int keypad_test_mode = 0;

static void CwLnx_linewrap(int on);
static void CwLnx_autoscroll(int on);
static void CwLnx_hidecursor();
static void CwLnx_reboot();
static void CwLnx_heartbeat(int type);
static char CwLnx_getkey();

/* TODO:  Get rid of this variable? */
lcd_logical_driver *CwLnx;
/* TODO:  Get the frame buffers working right */

#define LCD_CMD			254
#define LCD_CMD_END		253
#define LCD_INIT_CHINESE_T	56
#define LCD_INIT_CHINESE_S	55
#define LCD_LIGHT_ON		66
#define LCD_LIGHT_OFF		70
#define LCD_CLEAR		88
#define LCD_SET_INSERT		71
#define LCD_INIT_INSERT		72
#define LCD_SET_BAUD		57
#define LCD_ENABLE_WRAP		67
#define LCD_DISABLE_WRAP	68
#define LCD_SETCHAR		78
#define LCD_ENABLE_CURSOR	81
#define LCD_DISABLE_CURSOR	82

#define LCD_LENGTH		20

#define DELAY			20
#define UPDATE_DELAY		0	/* 1 sec */
#define SETUP_DELAY		1	/* 2 sec */

/* Parse one key from the configfile */
static char CwLnx_parse_keypad_setting (char * sectionname, char * keyname, char default_value)
{
	char return_val = 0;
	char * s;
	char buf [255];

	s = config_get_string ( sectionname, keyname, 0, NULL);
	if (s != NULL){
		strncpy (buf, s, sizeof(buf));
		buf[sizeof(buf)-1]=0;
		return_val = buf[0];
	} else {
		return_val=default_value;
	}
	return return_val;
}

int Read_LCD(int fd, char *c, int size)
{
    int rc;
    rc = read(fd, c, size);
/*    usleep(DELAY); */
    return rc;
}

int Write_LCD(int fd, char *c, int size)
{
    int rc;
    rc = write(fd, c, size);
/* Debuging code to be cleaned when very stable */
/* 
    if (size==1) {
	    if (*c>=0) 
		    printf("%3d ", *c);
          else
              {
		    if (*c+256==254)

		    printf("\n%3d ", *c+256);
		    else printf("%3d ", *c+256);
	      }
    }
*/
/*    usleep(DELAY); */
    return rc;
}


void Enable_Cursor(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_ENABLE_CURSOR;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

void Disable_Cursor(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_DISABLE_CURSOR;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

void Clear_Screen(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CLEAR;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
    usleep(UPDATE_DELAY);
}

void Enable_Wrap(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_ENABLE_WRAP;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

void Init_Port(fd)
{
    /* Posix - set baudrate to 0 and back */
    struct termios tty, old;

    tcgetattr(fd, &tty);
    tcgetattr(fd, &old);
    cfsetospeed(&tty, B0);
    cfsetispeed(&tty, B0);
    tcsetattr(fd, TCSANOW, &tty);
    usleep(SETUP_DELAY);
    tcsetattr(fd, TCSANOW, &old);
}

void Setup_Port(int fd, speed_t speed)
{
    struct termios portset;

    tcgetattr(fd, &portset);
    cfsetospeed(&portset, speed);
    cfsetispeed(&portset, speed);
    portset.c_iflag = IGNBRK;
    portset.c_lflag = 0;
    portset.c_oflag = 0;
    portset.c_cflag |= CLOCAL | CREAD;
    portset.c_cflag &= ~CRTSCTS;
    portset.c_cc[VMIN] = 1;
    portset.c_cc[VTIME] = 5;
    tcsetattr(fd, TCSANOW, &portset);
}

void Set_9600(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_SET_BAUD;
    rc = Write_LCD(fd, &c, 1);
    c = 0x20;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

void Set_19200(int fd)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_SET_BAUD;
    rc = Write_LCD(fd, &c, 1);
    c = 0xf;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

int Write_Line_LCD(int fd, char *buf)
{
    int i;
    char c;
    int isEnd = 0;
    int rc;

    for (i = 0; i < LCD_LENGTH; i++) {
	if (buf[i] == '\0') {
	    isEnd = 1;
	}
	if (isEnd) {
	    c = ' ';
	} else {
	    c = buf[i];
	}
	rc = Write_LCD(fd, &c, 1);
    }
/*    printf("%s\n", buf); */
    return 0;
}


/*****************************************************
 * Opens com port and sets baud correctly...
 */
int CwLnx_init(lcd_logical_driver * driver, char *args)
{
    int tmp, w, h;
    int reboot = 0;
    struct termios portset_save;


    char device[200] = CWLNX_DEF_DEVICE;
    int speed = CWLNX_DEF_SPEED;
    char size[200] = CWLNX_DEF_SIZE;

    char buf[256] = "";

    CwLnx = driver;

    debug(RPT_INFO, "CwLnx: init(%p,%s)", driver, args);

    /* TODO: replace DriverName with driver->name when that field exists. */
#define DriverName "CwLnx"


    /*Read config file */

    /*Which serial device should be used */
    strncpy(device,
	    config_get_string(DriverName, "Device", 0, CWLNX_DEF_DEVICE),
	    sizeof(device));
    device[sizeof(device) - 1] = 0;
    report(RPT_INFO, "CwLnx: Using device: %s", device);

    /*Which size */
    strncpy(size, config_get_string(DriverName, "Size", 0, CWLNX_DEF_SIZE),
	    sizeof(size));
    size[sizeof(size) - 1] = 0;
    if (sscanf(size, "%dx%d", &w, &h) != 2
	|| (w <= 0) || (w > LCD_MAX_WIDTH)
	|| (h <= 0) || (h > LCD_MAX_HEIGHT)) {
	report(RPT_WARNING,
	       "CwLnx: Cannot read size: %s. Using default value.\n",
	       size);
	sscanf(CWLNX_DEF_SIZE, "%dx%d", &w, &h);
    }
    driver->wid = w;
    driver->hgt = h;

    /*Which speed */
    tmp = config_get_int(DriverName, "Speed", 0, CWLNX_DEF_SPEED);

    switch (tmp) {
    case 9600:
	speed = B9600;
	break;
    case 19200:
	speed = B19200;
	break;
    default:
	speed = CWLNX_DEF_SPEED;
	switch (speed) {
	case B9600:
	    strncpy(buf, "9600", sizeof(buf));
	    break;
	case B19200:
	    strncpy(buf, "19200", sizeof(buf));
	    break;
	}
	report(RPT_WARNING,
	       "CwLnx: Speed must be 9600 or 19200. Using default value of %s baud!",
	       buf);
	strncpy(buf, "", sizeof(buf));
    }

    /*Reboot display? */
    if (config_get_bool(DriverName, "Reboot", 0, 0)) {
	report(RPT_INFO, "CwLnx: Rebooting Cwlinux LCD...\n");
	reboot = 1;
    }

    /* keypad test mode? */
    if (config_get_bool( DriverName , "keypad_test_mode" , 0 , 0)) {
	report (RPT_INFO, "CwLnx: Entering keypad test mode...\n");
	keypad_test_mode = 1;
    }


    if (!keypad_test_mode) {
	/* pause_key */
	pause_key = CwLnx_parse_keypad_setting (DriverName, "PauseKey", CWLNX_DEF_PAUSE_KEY);
	report (RPT_DEBUG, "CwLnx: Using \"%c\" as pause_key.", pause_key);
	/* back_key */
	back_key = CwLnx_parse_keypad_setting (DriverName, "BackKey", CWLNX_DEF_BACK_KEY);
	report (RPT_DEBUG, "CwLnx: Using \"%c\" as back_key", back_key);
		
	/* forward_key */
	forward_key = CwLnx_parse_keypad_setting (DriverName, "ForwardKey", CWLNX_DEF_FORWARD_KEY);
	report (RPT_DEBUG, "CwLnx: Using \"%c\" as forward_key", forward_key);
		
	/* main_menu_key
	* */
	main_menu_key = CwLnx_parse_keypad_setting (DriverName, "MainMenuKey", CWLNX_DEF_MAIN_MENU_KEY);
	report (RPT_DEBUG, "CwLnx: Using \"%c\" as main_menu_key", main_menu_key);
	}


    /* End of config file parsing */


    /* Allocate framebuffer memory */
    /* You must use driver->framebuf here, but may use lcd.framebuf later. */
    if (!driver->framebuf) {
	driver->framebuf = malloc(driver->wid * driver->hgt);
	backingstore = calloc(driver->wid * driver->hgt, 1);
        memset(backingstore, ' ', driver->wid * driver->hgt);
    }

    if (!driver->framebuf) {
	report(RPT_ERR, "CwLnx: Error: unable to create framebuffer.\n");
	return -1;
    }

    /* Set up io port correctly, and open it... */
    debug(RPT_DEBUG, "CwLnx: Opening serial device: %s", device);
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
	report(RPT_ERR, "CwLnx: init() failed (%s)\n", strerror(errno));
	return -1;
    } else {
	report(RPT_INFO, "CwLnx: Opened display on %s", device);
    }

    Init_Port(fd);
    tcgetattr(fd, &portset_save);
    speed = B19200;
    Setup_Port(fd, speed);
    Set_9600(fd); 
    close(fd);

    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
	report(RPT_ERR, "CwLnx: init() failed (%s)\n", strerror(errno));
	return -1;
    } else {
	report(RPT_INFO, "CwLnx: Opened display on %s", device);
    }
    Init_Port(fd);
    speed = B9600; 
    Setup_Port(fd, speed);

    /* Set display-specific stuff.. */
    if (reboot) {
	CwLnx_reboot();
	sleep(4);
	reboot = 0;
    }
    sleep(1);
    CwLnx_hidecursor();
    CwLnx_linewrap(1);
    CwLnx_autoscroll(0);
    CwLnx_backlight(1);

    /* Set the functions the driver supports... */

    driver->daemonize = 1;	/* make the server daemonize after initialization */

    driver->clear = CwLnx_clear;
    driver->string = CwLnx_string;
    driver->chr = CwLnx_chr;
    driver->vbar = CwLnx_vbar;
    driver->init_vbar = CwLnx_init_vbar;
    driver->hbar = CwLnx_hbar;
    driver->init_hbar = CwLnx_init_hbar;
    driver->num = CwLnx_num;

    driver->init = CwLnx_init;
    driver->close = CwLnx_close;
    driver->flush = CwLnx_flush;
    driver->flush_box = CwLnx_flush_box;
    driver->backlight = CwLnx_backlight;
    driver->set_char = CwLnx_set_char;
    driver->icon = CwLnx_icon;
    driver->draw_frame = CwLnx_draw_frame;

    driver->cellwid = CWLNX_DEF_CELL_WIDTH;
    driver->cellhgt = CWLNX_DEF_CELL_HEIGHT;

    driver->heartbeat = CwLnx_heartbeat;

    driver->getkey = CwLnx_getkey;

    report(RPT_DEBUG, "CwLnx_init: done\n");

    Clear_Screen(fd);
    CwLnx_clear();
    usleep(SETUP_DELAY);

    return fd;
}

/******************************************************
 * Clean-up
 */
void CwLnx_close()
{
    close(fd);

    if (CwLnx->framebuf)
	free(CwLnx->framebuf);

    if (backingstore)
	free(backingstore);

    CwLnx->framebuf = NULL;
    backingstore = NULL;
}

void CwLnx_flush()
{
    CwLnx_draw_frame(CwLnx->framebuf);
}

void Set_Insert(int fd, int row, int col)
{
    char c;
    int rc;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    if (row==0 && col==0) 
    	{
    	c = LCD_INIT_INSERT;
    	rc = Write_LCD(fd, &c, 1);
    	}
    else
    	{
    c = LCD_SET_INSERT;
    rc = Write_LCD(fd, &c, 1);
    c = col;
    rc = Write_LCD(fd, &c, 1);
    c = row;
    rc = Write_LCD(fd, &c, 1);
    	}
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

void CwLnx_flush_box(int lft, int top, int rgt, int bot)
{
    int y;

    debug(RPT_DEBUG, "CwLnx: flush_box (%i,%i)-(%i,%i)\n", lft, top, rgt,
	  bot);
    for (y = top; y <= bot; y++) {
	Set_Insert(fd, top, lft);
	Write_Line_LCD(fd, CwLnx->framebuf + (y * CwLnx->wid) + lft);
    }
}

/*******************************************************************
 * Prints a character on the lcd display, at position (x,y).  The
 * upper-left is (1,1), and the lower right should be (20,4).
 */
void CwLnx_chr(int x, int y, char c)
{
    y--;
    x--;

    CwLnx->framebuf[(y * CwLnx->wid) + x] = c;
}

/*****************************************************
 * Changes screen contrast (0-255; 140 seems good)
 */
int CwLnx_contrast(int contrast)
{
    return -1;
}

/*********************************************************
 * Sets the backlight brightness
 */
void CwLnx_backlight(int on)
{
    static int current = -1;
    int realbacklight = -1;
    char c;
    int rc;

    if (on == current)
	return;

    /* validate backlight value */
    if (on > 255)
	on = 255;
    if (on < 0)
	on = 0;

    current = on;

    realbacklight = (int) (current * 100 / 255);

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_LIGHT_ON;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

/*********************************************************
 * Toggle the built-in linewrapping feature
 */
static void CwLnx_linewrap(int on)
{
    char out[4];
    if (on)
	snprintf(out, sizeof(out), "%c", 23);
    else
	snprintf(out, sizeof(out), "%c", 24);
    Enable_Wrap(fd);
}

/****************************************************************
 * Toggle the built-in automatic scrolling feature
 */
static void CwLnx_autoscroll(int on)
{
    return;
}

/*******************************************************************
 * Get rid of the blinking curson
 */
static void CwLnx_hidecursor()
{
    return;
}

/********************************************************************
 * Reset the display bios
 */
static void CwLnx_reboot()
{
    return;
}

/*************************************************************
 * Sets up for vertical bars.  Call before CwLnx->vbar()
 */
void CwLnx_init_vbar()
{
    char a[] = {
	1, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0,
    };
    char b[] = {
	1, 1, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0,
    };
    char c[] = {
	1, 1, 1, 0, 0, 0, 0, 0,
	1, 1, 1, 0, 0, 0, 0, 0,
	1, 1, 1, 0, 0, 0, 0, 0,
	1, 1, 1, 0, 0, 0, 0, 0,
	1, 1, 1, 0, 0, 0, 0, 0,
	1, 1, 1, 0, 0, 0, 0, 0,
    };
    char d[] = {
	1, 1, 1, 1, 0, 0, 0, 0,
	1, 1, 1, 1, 0, 0, 0, 0,
	1, 1, 1, 1, 0, 0, 0, 0,
	1, 1, 1, 1, 0, 0, 0, 0,
	1, 1, 1, 1, 0, 0, 0, 0,
	1, 1, 1, 1, 0, 0, 0, 0,
    };
    char e[] = {
	1, 1, 1, 1, 1, 0, 0, 0,
	1, 1, 1, 1, 1, 0, 0, 0,
	1, 1, 1, 1, 1, 0, 0, 0,
	1, 1, 1, 1, 1, 0, 0, 0,
	1, 1, 1, 1, 1, 0, 0, 0,
	1, 1, 1, 1, 1, 0, 0, 0,
    };
    char f[] = {
	1, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 1, 1, 1, 0, 0,
    };
    char g[] = {
	1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 0,
    };

    if (custom != vbar) {
	CwLnx_set_char(1, a);
	CwLnx_set_char(2, b);
	CwLnx_set_char(3, c);
	CwLnx_set_char(4, d);
	CwLnx_set_char(5, e);
	CwLnx_set_char(6, f);
	CwLnx_set_char(7, g);
	custom = vbar;
    }
}

/*********************************************************
 * Inits horizontal bars...
 */
void CwLnx_init_hbar()
{
    char a[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
    };
    char b[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
    };
    char c[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
    };
    char d[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
    };
    char e[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
    };
    char f[] = {
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
    };

    if (custom != hbar) {
	CwLnx_set_char(1, a);
	CwLnx_set_char(2, b);
	CwLnx_set_char(3, c);
	CwLnx_set_char(4, d);
	CwLnx_set_char(5, e);
	CwLnx_set_char(6, f);
	custom = hbar;
    }

}

/*************************************************************
 * Draws a vertical bar...
 */
void CwLnx_vbar(int x, int len)
{
    char map[9] = { 32, 1, 2, 3, 4, 5, 6, 7, 255 };

    int y;
    for (y = CwLnx->hgt; y > 0 && len > 0; y--) {
	if (len >= CwLnx->cellhgt)
	    CwLnx_chr(x, y, 255);
	else
	    CwLnx_chr(x, y, map[len]);

	len -= CwLnx->cellhgt;
    }

}

/*****************************************************************
 * Draws a horizontal bar to the right.
 */
void CwLnx_hbar(int x, int y, int len)
{
    char map[7] = { 32, 1, 2, 3, 4, 5, 255 };

    for (; x <= CwLnx->wid && len > 0; x++) {
	if (len >= CwLnx->cellwid)
	    CwLnx_chr(x, y, 255);
	else
	    CwLnx_chr(x, y, map[len]);

	len -= CwLnx->cellwid;

    }

}


/*******************************************************************
 * Writes a big number.
 */
void CwLnx_num(int x, int num)
{
    return;
}

/*********************************************************************
 * Sets a custom character from 0-7...
 * For input, values > 0 mean "on" and values <= 0 are "off".
 *
 * The input is just an array of characters...
 */
void CwLnx_set_char(int n, char *dat)
{
    int row, col;
    int letter;
    char c;
    int rc;

    if (n < 1 || n > 8)
	return;
    if (!dat)
	return;

    c = LCD_CMD;
    rc = Write_LCD(fd, &c, 1);
    c = LCD_SETCHAR;
    rc = Write_LCD(fd, &c, 1);
    c = (char) n;
    rc = Write_LCD(fd, &c, 1);

    for (col = 0; col < CwLnx->cellwid; col++) {
	letter = 0;
	for (row = 0; row < CwLnx->cellhgt; row++) {
	    letter <<= 1;
	    letter |= (dat[(col * CwLnx->cellhgt) + row] > 0);
	}
	c=letter;
	Write_LCD(fd, &c, 1);
    }
    c = LCD_CMD_END;
    rc = Write_LCD(fd, &c, 1);
}

void CwLnx_icon(int which, char dest)
{
    char icons[3][6 * 8] = {
	{
	 1, 1, 1, 0, 0, 0, 1, 1,	/* Empty Heart */
	 1, 1, 0, 0, 0, 0, 0, 1,
	 1, 0, 0, 0, 0, 0, 1, 1,
	 1, 1, 0, 0, 0, 0, 0, 1,
	 1, 1, 1, 0, 0, 0, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1,
	 },

	{
	 1, 1, 1, 0, 0, 0, 1, 1,	/* Filled Heart */
	 1, 1, 0, 1, 1, 1, 0, 1,
	 1, 0, 1, 1, 1, 0, 1, 1,
	 1, 1, 0, 1, 1, 1, 0, 1,
	 1, 1, 1, 0, 0, 0, 1, 1,
	 1, 1, 1, 1, 1, 1, 1, 1,
	 },

	{
	 1, 0, 0, 0, 0, 0, 0, 0,	/* Ellipsis */
	 0, 0, 0, 0, 0, 0, 0, 0,
	 1, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 1, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,
	 },

    };

    if (custom == bign)
	custom = beat;
    CwLnx_set_char(dest, &icons[which][0]);
}

/**********************************************************
 * Blasts a single frame onscreen, to the lcd...
 *
 * Input is a character array, sized CwLnx->wid*CwLnx->hgt
 */
void CwLnx_draw_frame(char *dat)
{
    int i, j, mv, rc;
    char *p, *q;
/*  char c; */
/*  static int count=0; */

    if (!dat)
	return;

    mv = 1;
    p = dat;
    q = backingstore;

/*    printf("\n_draw_frame: %d\n", count);   */

    for (i = 0; i < CwLnx->hgt; i++) 
            {
	    for (j = 0; j < CwLnx->wid; j++) 
	            {
		    if ( (*p == *q) && !( (0<*p) && (*p<16) ) )
		    	{
				mv = 1;
/*         count++; if (count==COUNT) exit(0);       */
			}
		    else
		        {
			    /* Draw characters that have changed, as well
			     * as custom characters.  We know not if a custom
			     * character has changed.
			     */ 
		        if (mv == 1) 
			    {
			    Set_Insert(fd, i, j);
			    mv = 0;
		   	    }
                        rc = Write_LCD(fd, p, 1);
		        }
		    p++;
		    q++; 
	            }
            }
  strncpy(backingstore, dat, CwLnx->wid * CwLnx->hgt);

}

/*********************************************************
 * Clears the LCD screen
 */
void CwLnx_clear()
{
    memset(CwLnx->framebuf, ' ', CwLnx->wid * CwLnx->hgt);
}

/*****************************************************************
 * Prints a string on the lcd display, at position (x,y).  The
 * upper-left is (1,1), and the lower right should be (20,4).
 */
void CwLnx_string(int x, int y, char string[])
{
    int i;

    x -= 1;			/* Convert 1-based coords to 0-based... */
    y -= 1;

    for (i = 0; string[i]; i++) {

	/* Check for buffer overflows... */
	if ((y * CwLnx->wid) + x + i > (CwLnx->wid * CwLnx->hgt))
	    break;
	CwLnx->framebuf[(y * CwLnx->wid) + x + i] = string[i];
    }
}

static char CwLnx_getkey()
{
	char in = 0;

	read (fd, &in, 1);

	if (in != 0) {
		if (!keypad_test_mode) {
			if (in==pause_key) {
				in = INPUT_PAUSE_KEY;
			} else if (in==back_key) {
				in = INPUT_BACK_KEY;
			} else if (in==forward_key){
				in = INPUT_FORWARD_KEY;
			} else if (in==main_menu_key) {
				in = INPUT_MAIN_MENU_KEY;
			} else {
				in = 0;
			}
			report(RPT_DEBUG, "CwLnx: getkey(): returning %c", in);
		} else {
			fprintf (stdout, "CwLnx: Received character %c\n", in);
			in = 0;
			fprintf (stdout, "CwLnx: Press another key of your device.\n");
		}
	}

	return in;
}

/**********************************************************
 * Does the heartbeat...
 */
static void CwLnx_heartbeat(int type)
{
    static int timer = 0;
    int whichIcon;
    static int saved_whichIcon=123;
    static int saved_type = HEARTBEAT_ON;

    if (type)
	saved_type = type;

    if (type == HEARTBEAT_ON) {
	/* Set this to pulsate like a real heart beat... */
	whichIcon = (!((timer + 4) & 5));

	/* This defines a custom character EVERY time... */
	/* not efficient... is this necessary? */
	if (whichIcon != saved_whichIcon) {
		CwLnx_icon(whichIcon, 8); 
		saved_whichIcon = whichIcon;
	}

	/* Put character on screen... */
	CwLnx_chr(CwLnx->wid, 1, 8 );

	/* change display... */
	CwLnx_flush();
    }

    timer++;
    timer &= 0x0f;
}

