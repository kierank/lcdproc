/*lcdm001.c*/
/*  This is the LCDproc driver for the "LCDM001" device from kernelconcepts.de

    Copyright (C) 2001  Rene Wagner <reenoo@gmx.de>

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

/* This driver is mostly based on the MtxOrb driver.
   See the file MtxOrb.c for copyright details */
/* The heartbeat workaround has been taken from the curses driver
   See the file curses_drv.c for copyright details */
/* The hbar and vbar workarounds are based on the curses driver
   See the file curses_drv.c for copyright details */
/* The function calls needed for reporting and getting settings from the
   configfile have been written taking the calls in
   sed1330.c ((C) by Joris Robijn) as examples*/
/* The backingstore implementation is based on the code in CFontz.c
   (C) 2002 by Mike Patnode */
/* (Hopefully I have NOT forgotten any file I have stolen code from.
   If so send me an e-mail or add your copyright here!) */

/* LCDM001 does NOT support custom chars
   Most of the displaying problems have been fixed
   using ASCII workarounds*/

/* You can modify the characters that are displayed instead of
   the normal icons for the heartbeat in lcdm001.h*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
# if TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#  if HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif

#include "lcd.h"
#include "lcdm001.h"
#include "shared/str.h"
#include "shared/report.h"
#include "configfile.h"
#include "render.h"
#include "input.h"

/* Moved here from lcdm001.h to reduce the number of warnings.*/
static void lcdm001_close ();
static void lcdm001_clear ();
static void lcdm001_flush ();
static void lcdm001_string (int x, int y, char string[]);
static void lcdm001_chr (int x, int y, char c);
static void lcdm001_output (int on);
static void lcdm001_vbar (int x, int len);
static void lcdm001_hbar (int x, int y, int len);
static void lcdm001_num (int x, int num);
static void lcdm001_icon (int which, char dest);
static void lcdm001_flush_box (int lft, int top, int rgt, int bot);
static void lcdm001_draw_frame (char *dat);
static char lcdm001_getkey ();
/* End of extract from lcdm001.h by David GLAUDE.*/
static void lcdm001_heartbeat (int type);

#define NotEnoughArgs (i + 1 > argc)

lcd_logical_driver *lcdm001;
static int fd;
static int hbarworkaround=0, vbarworkaround=0;
static char icon_char = LCDM001_PAD;
static char pause_key = LCDM001_DOWN_KEY, back_key = LCDM001_LEFT_KEY, forward_key = LCDM001_RIGHT_KEY, main_menu_key = LCDM001_UP_KEY;

static char* oldframebuf = NULL;

static char num_icon [10][4][3] = 	{{{' ','_',' '}, /*0*/
					  {'|',' ','|'},
					  {'|','_','|'},
					  {' ',' ',' '}},
					  {{' ',' ',' '},/*1*/
					  {' ',' ','|'},
					  {' ',' ','|'},
					  {' ',' ',' '}},
					  {{' ','_',' '},/*2*/
					  {' ','_','|'},
					  {'|','_',' '},
					  {' ',' ',' '}},
					  {{' ','_',' '},/*3*/
					  {' ','_','|'},
					  {' ','_','|'},
					  {' ',' ',' '}},
					  {{' ',' ',' '},/*4*/
					  {'|','_','|'},
					  {' ',' ','|'},
					  {' ',' ',' '}},
					  {{' ','_',' '},/*5*/
					  {'|','_',' '},
					  {' ','_','|'},
					  {' ',' ',' '}},
					  {{' ','_',' '},/*6*/
					  {'|','_',' '},
					  {'|','_','|'},
					  {' ',' ',' '}},
					  {{' ','_',' '},/*7*/
					  {' ',' ','|'},
					  {' ',' ','|'},
					  {' ',' ',' '}},
					  {{' ','_',' '},/*8*/
					  {'|','_','|'},
					  {'|','_','|'},
					  {' ',' ',' '}},
					  {{' ','_',' '},/*9*/
					  {'|','_','|'},
					  {' ','_','|'},
					  {' ',' ',' '}}};

static void lcdm001_cursorblink (int on);
static void lcdm001_string (int x, int y, char *string);
static char lcdm001_parse_keypad_setting ( char * sectionname, char * keyname, char * default_value );

#define ValidX(x) if ((x) > lcdm001->wid) { (x) = lcdm001->wid; } else (x) = (x) < 1 ? 1 : (x);
#define ValidY(y) if ((y) > lcdm001->hgt) { (y) = lcdm001->hgt; } else (y) = (y) < 1 ? 1 : (y);

/* Parse one key from the configfile */
static char lcdm001_parse_keypad_setting (char * sectionname, char * keyname, char * default_value)
{
	char return_val = 0;

	if (strcmp( config_get_string ( sectionname, keyname, 0, default_value), "LeftKey")==0) {
		return_val=LCDM001_LEFT_KEY;
	} else if (strcmp( config_get_string ( sectionname, keyname, 0, default_value), "RightKey")==0) {
		return_val=LCDM001_RIGHT_KEY;
	} else if (strcmp( config_get_string ( sectionname, keyname, 0, default_value), "UpKey")==0) {
		return_val=LCDM001_UP_KEY;
	} else if (strcmp( config_get_string ( sectionname, keyname, 0, default_value), "DownKey")==0) {
		return_val=LCDM001_DOWN_KEY;
	} else {
		report (RPT_WARNING, "LCDM001: Invalid  config file setting for %s. Using default value %s", keyname, default_value);
		if (strcmp (default_value, "LeftKey")==0) {
			return_val=LCDM001_LEFT_KEY;
		} else if (strcmp (default_value, "RightKey")==0) {
			return_val=LCDM001_RIGHT_KEY;
		} else if (strcmp (default_value, "UpKey")==0) {
			return_val=LCDM001_UP_KEY;
		} else if (strcmp (default_value, "DownKey")==0) {
			return_val=LCDM001_DOWN_KEY;
		}
	}
	return return_val;
}

/* Set cursorblink on/off */
static void
lcdm001_cursorblink (int on)
{
	if (on) {
		write (fd, "~K1", 3);
		debug(RPT_INFO, "LCDM001: cursorblink turned on");
	} else {
		write (fd, "~K0", 3);
		debug(RPT_INFO, "LCDM001: cursorblink turned off");
	}
}


/* TODO: Get lcd.framebuf to properly work as whatever driver is running...*/

/*********************************************************************
 * init() should set up any device-specific stuff, and
 * point all the function pointers.
 */
int
lcdm001_init (struct lcd_logical_driver *driver, char *args)
{
        char device[200];
	int speed=B38400;
        struct termios portset;

	char out[5]="";

	lcdm001 = driver;

	debug( RPT_INFO, "LCDM001: init(%p,%s)", driver, args );

	driver->wid = 20;
	driver->hgt = 4;

	/* You must use driver->framebuf here, but may use lcd.framebuf later.*/
	if (!driver->framebuf) {
		driver->framebuf = malloc (driver->wid * driver->hgt);
	}
	oldframebuf = calloc (driver->wid * driver->hgt, 1);

	if (!(driver->framebuf && oldframebuf)) {
		report(RPT_ERR, "Error: unable to create LCDM001 framebuffer.");
		return -1;
	}
/* Debugging...
 *  if(lcd.framebuf) printf("Frame buffer: %i\n", (int)lcd.framebuf);
 */

	memset (driver->framebuf, ' ', driver->wid * driver->hgt);
/*  lcdm001_clear();*/

	driver->cellwid = 5;
	driver->cellhgt = 8;

	/* TODO: replace DriverName with driver->name when that field exists.*/
	#define DriverName "lcdm001"

	/* READ CONFIG FILE:*/

	/* Get fd of serial device to be used */
	strncpy(device, config_get_string ( DriverName , "Device" , 0 , "/dev/lcd"), sizeof(device));
	device[sizeof(device)-1]=0;
	report (RPT_INFO,"LCDM001: Using device: %s", device);

	/* Get keypad settings */
	pause_key = lcdm001_parse_keypad_setting (DriverName, "PauseKey", "DownKey");
	back_key = lcdm001_parse_keypad_setting (DriverName, "BackKey", "LeftKey");
	forward_key = lcdm001_parse_keypad_setting (DriverName, "ForwardKey", "RightKey");
	main_menu_key = lcdm001_parse_keypad_setting (DriverName, "MainMenuKey", "UpKey");

	/* Enable the hbar workaround? */
	if(config_get_bool( DriverName , "HBarWorkaround" , 0 , 0)) {
		hbarworkaround = 1;
	}
	/* Enable the vbar workaround? */
	if(config_get_bool( DriverName , "VBarWorkaround" , 0 , 0)) {
		vbarworkaround = 1;
	}

	/* End of config file parsing*/


	/* Set up io port correctly, and open it...*/
	debug( RPT_DEBUG, "LCDM001: Opening serial device: %s", device);
	fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		switch (errno) {
			case ENOENT: report( RPT_ERR, "LCDM001: lcdm001_init() failed: Device file missing: %s", device);
				break;
			case EACCES: report( RPT_ERR, "LCDM001: lcdm001_init() failed: Could not open device: %s", device);
				report( RPT_ERR, "LCDM001: lcdm001_init() failed: Make sure you have rw access to %s!", device);
				break;
			default: report( RPT_ERR, "LCDM001: lcdm001_init() failed (%s)", strerror (errno));
				break;
		}
  		return -1;
	} else {
		report (RPT_INFO, "LCDM001: opened display on %s", device);
	}
	tcgetattr(fd, &portset);
	
	/* cfmakeraw seems to be simple to use but not error free
	 * e.g. it doesn't work properly on FreeBSD
	 * So, we have to do it the hard way:
	 */
	portset.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP
	                      | INLCR | IGNCR | ICRNL | IXON );
	portset.c_oflag &= ~OPOST;
	portset.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
	portset.c_cflag &= ~( CSIZE | PARENB | CRTSCTS );
	portset.c_cflag |= CS8 | CREAD | CLOCAL ;

	cfsetospeed(&portset, speed);
	cfsetispeed(&portset, speed);
	tcsetattr(fd, TCSANOW, &portset);
	tcflush(fd, TCIOFLUSH);

	/* Reset and clear the LCDM001 */
	write (fd, "~C", 2);
	/* Set cursorblink to default */
	lcdm001_cursorblink (LCDM001_DEF_CURSORBLINK);
	/* Turn all LEDs off */
	snprintf (out, sizeof(out), "%cL%c%c", 126, 0, 0);
	write (fd, out, 4);

        /*
         * Configure the display functions
        */
	driver->daemonize = 1; /* make the server daemonize after initialisation*/

	driver->clear = lcdm001_clear;
	driver->string = lcdm001_string;
	driver->chr = lcdm001_chr;
	driver->vbar =lcdm001_vbar;
	/*init_vbar is not needed*/
	driver->hbar = lcdm001_hbar;
	/*init_hbar is not needed*/
	driver->num = lcdm001_num;
	/*init_num is not needed*/

	driver->init = lcdm001_init;
	driver->close = lcdm001_close;
	driver->flush = lcdm001_flush;
	driver->flush_box = lcdm001_flush_box;
	/* contrast and backlight are not implemented as
	 * changing the contrast or the state of the backlight
	 * is not supported by the device
	 * Well ... you could make use of your screw driver and
	 * soldering iron ;)
	 */
	driver->output = lcdm001_output;
	/* set_char is not implemented as custom chars are not
	 * supported by the device
	 */
	driver->icon = lcdm001_icon;
	driver->draw_frame = lcdm001_draw_frame;

	driver->getkey = lcdm001_getkey;
	driver->heartbeat = lcdm001_heartbeat;

	return fd;
}

/* Below here, you may use either lcd.framebuf or driver->framebuf..
 * lcd.framebuf will be set to the appropriate buffer before calling
 * your driver.
 */

static void
lcdm001_close ()
{
	char out[5];
	if (lcdm001->framebuf != NULL)
		free (lcdm001->framebuf);

	lcdm001->framebuf = NULL;

	if (oldframebuf != NULL)
		free (oldframebuf);

	oldframebuf = NULL;

	/*switch off all LEDs*/
	snprintf (out, sizeof(out), "%cL%c%c", 126, 0, 0);
	write (fd, out, 4);
	close (fd);

        report (RPT_INFO, "LCDM001: closed");
}

/**********************************************************
 * Clears the LCD screen
 */
static void
lcdm001_clear ()
{
        if (lcdm001->framebuf != NULL)
                memset (lcdm001->framebuf, ' ', (lcdm001->wid * lcdm001->hgt));

	debug (RPT_DEBUG, "LCDM001: screen will be cleared during next flush()");
}

/***********************************************************
 * Flushes all output to the lcd...
 */
void
lcdm001_flush ()
{
	lcdm001_draw_frame(lcdm001->framebuf);

        debug (RPT_DEBUG, "LCDM001: frame buffer flushed");
}

/***********************************************************
 * Send a rectangular area to the display.
 */

static void
lcdm001_flush_box (int lft, int top, int rgt, int bot)
{
	int y;
	char out[LCD_MAX_WIDTH];

	for (y = top; y <= bot; y++) {
		snprintf (out, sizeof(out), "%cP%1d%2d", 126, lft, y);
		write (fd, out, 4);
		write (fd, lcdm001->framebuf + (y * lcdm001->wid) + lft, rgt - lft + 1);
	}
	debug (RPT_DEBUG, "LCDM001: frame buffer box flushed");
}

/**********************************************************************
 * Prints a character on the lcd display, at position (x,y).  The
 * upper-left is (1,1), and the lower right should be (20,4).
 */
static void
lcdm001_chr (int x, int y, char c)
{
	char buf[64];
	int offset;

	ValidX(x);
	ValidY(y);

        if (c==0) {
                c = icon_char;   /*heartbeat workaround*/
        }

	/* write to frame buffer*/
	y--; x--; /* translate to 0-coords*/

	offset = (y * lcdm001->wid) + x;
	lcdm001->framebuf[offset] = c;

	snprintf(buf, sizeof(buf), "LCDM001: writing character %02X to position (%d,%d)", c, x, y);
	debug (RPT_DEBUG, buf);
}

/*****************************************************************
 * Prints a string on the lcd display, at position (x,y).  The
 * upper-left is (1,1), and the lower right should be (20,4).
 */
static void
lcdm001_string (int x, int y, char string[])
{
	int offset, siz;

	ValidX(x);
	ValidY(y);

	x--; y--; /* Convert 1-based coords to 0-based...*/
	offset = (y * lcdm001->wid) + x;
	siz = (lcdm001->wid * lcdm001->hgt) - offset;
	siz = siz > strlen(string) ? strlen(string) : siz;

	memcpy(lcdm001->framebuf + offset, string, siz);

	debug (RPT_DEBUG, "LCDM001: printed string at (%d,%d)", x, y);
}

/*****************************************************************
 * Controls LEDs
 * Actually one could connect other devices (via transistors or relais!)
 * to the device
 */
static void
lcdm001_output (int on)
{
	static int lcdm001_output_state = -1;

	char out[5];
	int one = 0, two = 0;

	if (lcdm001_output_state == on)
		return;

	lcdm001_output_state = on;

	/* The output value has to be split into two
	 * 8bit values, which are then sent to the device
	 */

	if (on<=255)
	{
		one=on;
		two=0;
	}
	else
	{
		one = on & 0xff;
		two = (on >> 8) & 0xff;
	}
        snprintf (out, sizeof(out), "~L%c%c",one,two);
        write(fd,out,4);

        debug (RPT_DEBUG, "LCDM001: current LED state: %d", on);
}

/***************************************************************
 * Draws a vertical bar, from the bottom of the screen up.
 */
static void
lcdm001_vbar(int x, int len)
{
   int y;
   char map[] = {'_','.',',',',','o','o','O','8'};

   y=lcdm001->hgt;

   debug (RPT_DEBUG , "LCDM001: vertical bar at %d set to %d", x, len);

   while (len >= lcdm001->cellhgt)
     {
       lcdm001_chr(x, y, 0xFF);
       len -= lcdm001->cellhgt;
       y--;
     }

   if(!len)
     return;
   else if (vbarworkaround != 0)
     {
       lcdm001_chr(x, y, map[len-1]);
     }
}

/*****************************************************************
 * Draws a horizontal bar to the right.
 */
static void
lcdm001_hbar(int x, int y, int len)
{

  ValidX(x);
  ValidY(y);

  debug (RPT_DEBUG, "LCDM001: horizontal bar at %d set to %d", x, len);

  /*TODO: Improve this function*/

  while((x <= lcdm001->wid) && (len > 0))
  {
    if((len < (int) lcdm001->cellwid / 2) && (hbarworkaround != 0))
      {
	lcdm001_chr(x, y, '-');
	break;
      }
    if((len < lcdm001->cellwid) && (hbarworkaround != 0))
      {
	lcdm001_chr(x, y, '=');
	break;
      }

    lcdm001_chr(x, y, 0xFF);
    len -= lcdm001->cellwid;
    x++;
  }

  return;
}

/**********************************************************************
 * Writes a big number.
 */
static void lcdm001_num (int x, int num)
{
	int y, dx;

	debug (RPT_DEBUG, "LCDM001: Writing big number \"%d\" at x = %d", num, x);

	/*This function uses an ASCII emulation of big numbers*/

	for (y = 1; y < 5; y++)
		for (dx = 0; dx < 3; dx++)
			lcdm001_chr (x + dx, y, num_icon[num][y-1][dx]);
}

/***********************************************************************
 * Sets character 0 to an icon...
 */
void
lcdm001_icon (int which, char dest)
{

	/* Heartbeat workaround:
	 * As custom chars are not supported LCDM001_OPEN_HEART
   	 * and LCDM001_FILLED_HEART are displayed instead.
         * You can change them in lcdm001.h
	 */

	if (dest == 0)
		switch (which) {
			case 0:
				icon_char = LCDM001_OPEN_HEART;
				break;
			case 1:
				icon_char = LCDM001_FILLED_HEART;
				break;
			default:
				icon_char = LCDM001_PAD;
				break;
		}
}

/*********************************************************************
 * Draws the framebuffer on the display.
 */
void
lcdm001_draw_frame (char *dat)
{
	char out[LCD_MAX_WIDTH];
	char *row, *oldrow;
	int i, written=-2;

	if (!dat)
		return;

	for (i = 0; i < lcdm001->hgt; i++) {

		row = dat + (lcdm001->wid * i);
		oldrow = oldframebuf + (lcdm001->wid * i);

		/* Backing-store implementation.  If it's already
		 * on the screen, don't put it there again
		 */
		if (memcmp(oldrow, row, lcdm001->wid) == 0)
		    continue;

        /* else, write out the entire row */
		memcpy(oldrow, row, lcdm001->wid);
		if ((!(i-1==written)) || i==0) {
			snprintf (out, sizeof(out), "%cP%1d00", 126, i);
			write (fd, out, 5);
		}
		write (fd, row, lcdm001->wid);
		written=i;
	}
}

/**********************************************************************
 * Tries to read a character from the device
 * Returns the values defined in input.h or
 * 0 for "nothing available".
 */
static char
lcdm001_getkey ()
{
        char in = 0;
        read (fd, &in, 1);
	if (in == pause_key) {
		in = INPUT_PAUSE_KEY;
	} else if (in == back_key) {
		in = INPUT_BACK_KEY;
	} else if (in == forward_key) {
		in = INPUT_FORWARD_KEY;
	} else if (in == main_menu_key) {
		in = INPUT_MAIN_MENU_KEY;
	}
	/*if(in) debug(RPT_DEBUG,"LCDM001: key: %c",in); */
        return in;
}

/************************************************************************
 * Does the heartbeat...
 */
static void
lcdm001_heartbeat (int type)
{
	static int timer = 0;
	int whichIcon;
	static int saved_type = HEARTBEAT_ON;

	if (type)
		saved_type = type;

	if (type == HEARTBEAT_ON) {
		/* Set this to pulsate like a real heart beat...*/
		whichIcon = (! ((timer + 4) & 5));

		/* This defines a custom character EVERY time...
		 * not efficient... is this necessary?
		 */
		lcdm001_icon (whichIcon, 0);

		/* Put character on screen...*/
		lcdm001_chr (lcdm001->wid, 1, 0);

		/* change display...*/
		lcdm001_flush ();
	}

	timer++;
	timer &= 0x0f;
}
