/*  This is the LCDproc driver for CrystalFontz 633 devices
    (get yours from http://crystalfontz.com)

    Copyright (C) 2002 David GLAUDE

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


/*
 * This is a maybe limited but working version of the CF633 driver.
 * The actual development is and will continue in CVS 0.5.
 *
 * Driver status
 * 04/04/2002: Working driver
 * 05/06/2002: KeyPad handeling are reading of return value
 *
 * THINGS NOT DONE:
 * + No checking if right hardware is connected (firmware/hardware)
 * + No BigNum (but screen is too small ???)
 * + No support for multiple instance (require private structure)
 * + No cache of custom char usage (like in MtxOrb)
 *
 * THINGS PARTIALY DONE:
 * + Checking of LCD response (message are only read for key detection) 
 *
 *
 * THINGS DONE:
 * + Stopping the live reporting (of temperature)
 * + Stopping the reporting of temp and fan (is it necessary after reboot)
 * + Use of library for hbar and vbar (good but library could be better)
 * + Support for keypad (Using a KeyRing)
 *
 * THINGS TO DO:
 * + Make the caching at least for heartbeat icon
 * + Backporting to 0.4.x
 * + Create and use the library (for custom char handeling)
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#define DEBUG

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "lcd.h"
#include "CFontz633.h"
#include "CFontz633io.h"
#include "render.h"
#include "input.h"
#include "shared/str.h"
#include "shared/report.h"
#include "server/configfile.h"


static int custom = 0;
typedef enum {
	hbar = 1,
	vbar = 2
} custom_type;

static int fd;
static int newfirmware = 0;
static char *backingstore = NULL;

static int brightness = CFONTZ633_DEF_BRIGHTNESS;
static int offbrightness = CFONTZ633_DEF_OFFBRIGHTNESS;

/* Internal functions */
static void CFontz633_icon (int which, char dest);
static void CFontz633_heartbeat (int type);
static void CFontz633_hidecursor ();
static void CFontz633_reboot ();
static void CFontz633_init_vbar ();
static void CFontz633_init_hbar ();
static void CFontz633_no_live_report ();
static void CFontz633_hardware_clear ();
static char CFontz633_getkey ();

lcd_logical_driver *CFontz633;

/*
 * Opens com port and sets baud correctly...
 */
int
CFontz633_init (lcd_logical_driver * driver, char *args)
{
	struct termios portset;
	int tmp, w, h;
	int reboot = 0;

	int contrast = CFONTZ633_DEF_CONTRAST;
	char device[200] = CFONTZ633_DEF_DEVICE;
	int speed = CFONTZ633_DEF_SPEED;
	char size[200] = CFONTZ633_DEF_SIZE;

	CFontz633 = driver;

	debug(RPT_INFO, "CFontz633: init(%p,%s)", driver, args );

	#define DriverName "CFontz633"

	EmptyKeyRing();
	EmptyReceiveBuffer();

	/* Read config file */

	/* Which serial device should be used */
	strncpy(device, driver->config_get_string ( DriverName , "Device" , 0 , CFONTZ633_DEF_DEVICE),sizeof(device));
	device[sizeof(device)-1]=0;
	debug (RPT_INFO,"CFontz633: Using device: %s", device);

	/* Which size */
	strncpy(size, driver->config_get_string ( DriverName , "Size" , 0 , CFONTZ633_DEF_SIZE),sizeof(size));
	size[sizeof(size)-1]=0;
	if( sscanf(size , "%dx%d", &w, &h ) != 2
	|| (w <= 0) || (w > LCD_MAX_WIDTH)
	|| (h <= 0) || (h > LCD_MAX_HEIGHT)) {
		report (RPT_WARNING, "CFontz633_init: Cannot read size: %s. Using default value.\n", size);
		sscanf( CFONTZ633_DEF_SIZE , "%dx%d", &w, &h );
	}
	driver->wid = w;
	driver->hgt = h;

	/*Which contrast*/
	if (0<=config_get_int ( DriverName , "Contrast" , 0 , CFONTZ633_DEF_CONTRAST) && config_get_int ( DriverName , "Contrast" , 0 , CFONTZ633_DEF_CONTRAST) <= 50) {
		contrast = config_get_int ( DriverName , "Contrast" , 0 , CFONTZ633_DEF_CONTRAST);
	} else {
		report (RPT_WARNING, "CFontz633_init: Contrast must between 0 and 50. Using default value.\n");
	}

	/* Which backlight brightness */
	if (0<=config_get_int ( DriverName , "Brightness" , 0 , CFONTZ633_DEF_BRIGHTNESS) && config_get_int ( DriverName , "Brightness" , 0 , CFONTZ633_DEF_BRIGHTNESS) <= 100) {
		brightness = config_get_int ( DriverName , "Brightness" , 0 , CFONTZ633_DEF_BRIGHTNESS);
	} else {
		report (RPT_WARNING, "CFontz633_init: Brightness must between 0 and 100. Using default value.\n");
	}

	/* Which backlight-off "brightness" */
	if (0<=config_get_int ( DriverName , "OffBrightness" , 0 , CFONTZ633_DEF_OFFBRIGHTNESS) && config_get_int ( DriverName , "OffBrightness" , 0 , CFONTZ633_DEF_OFFBRIGHTNESS) <= 100) {
		offbrightness = config_get_int ( DriverName , "OffBrightness" , 0 , CFONTZ633_DEF_OFFBRIGHTNESS);
	} else {
		report (RPT_WARNING, "CFontz633_init: OffBrightness must between 0 and 100. Using default value.\n");
	}


	/* Which speed */
	tmp = config_get_int ( DriverName , "Speed" , 0 , CFONTZ633_DEF_SPEED);
	if (tmp == 1200) speed = B1200;
	else if (tmp == 2400) speed = B2400;
	else if (tmp == 9600) speed = B9600;
	else if (tmp == 19200) speed = B19200;
	else { report (RPT_WARNING, "CFontz633_init: Speed must be 1200, 2400, 9600 or 19200. Using default value.\n", speed);
	}

	/* New firmware version?
	 * I will try to behave differently for firmware 0.6 or above.
	 * Currently this is not in use.
	 */
	if(config_get_bool( DriverName , "NewFirmware" , 0 , 0)) {
		newfirmware = 1;
	}

	/* Reboot display? */
	if (config_get_bool( DriverName , "Reboot" , 0 , 0)) {
		report (RPT_INFO, "CFontz633: Rebooting CrystalFontz LCD...\n");
		reboot = 1;
	}

	/* End of config file parsing*/

        /* Allocate framebuffer memory*/
        /* You must use driver->framebuf here, but may use lcd.framebuf later.*/
        if (!driver->framebuf) {
                driver->framebuf = malloc (driver->wid * driver->hgt);
        }
        if (!backingstore) {
                backingstore = calloc (driver->wid * driver->hgt, 1);
        }

        if (!(driver->framebuf && backingstore)) {
                report(RPT_ERR, "CFontz633: Error: unable to create framebuffer.\n");
                return -1;
        }

	/* Set up io port correctly, and open it... */
	debug( RPT_DEBUG, "CFontz633: Opening serial device: %s", device);
	fd = open (device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1) {
		report (RPT_ERR, "CFontz633_init: failed (%s)\n", strerror (errno));
		return -1;
	}

	tcgetattr (fd, &portset);

	/* We use RAW mode */
#ifdef HAVE_CFMAKERAW
	/* The easy way */
	cfmakeraw( &portset );
#else
	/* The hard way */
	portset.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP
	                      | INLCR | IGNCR | ICRNL | IXON );
	portset.c_oflag &= ~OPOST;
	portset.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
	portset.c_cflag &= ~( CSIZE | PARENB | CRTSCTS );
	portset.c_cflag |= CS8 | CREAD | CLOCAL ;
#endif

	/* Set port speed */
	cfsetospeed (&portset, speed);
	cfsetispeed (&portset, B0);

	/* Do it... */
	tcsetattr (fd, TCSANOW, &portset);

	/* Set display-specific stuff.. */
	if (reboot) {
		CFontz633_reboot ();
		sleep (4);
		reboot = 0;
	}
	sleep (2);
	CFontz633_hidecursor ();
	CFontz633_no_live_report ();

        /* Set the functions the driver supports...*/

        driver->daemonize = 1; /* make the server daemonize after initialization*/

        driver->clear = CFontz633_clear;
        driver->string = CFontz633_string;
        driver->chr = CFontz633_chr;
        driver->vbar = CFontz633_vbar;
        driver->init_vbar = CFontz633_init_vbar;
        driver->hbar = CFontz633_hbar;
        driver->init_hbar = CFontz633_init_hbar;
        driver->num = CFontz633_num;

        driver->init = CFontz633_init;
        driver->close = CFontz633_close;
        driver->flush = CFontz633_flush;
        driver->flush_box = CFontz633_flush_box;
        driver->contrast = CFontz633_contrast;
        driver->backlight = CFontz633_backlight;
        driver->set_char = CFontz633_set_char;
        driver->icon = CFontz633_icon;
/*        driver->draw_frame = CFontz633_draw_frame;	*/

        driver->getkey = CFontz633_getkey; 

        CFontz633_contrast (contrast);

        driver->cellwid = CFONTZ633_DEF_CELL_WIDTH;
        driver->cellhgt = CFONTZ633_DEF_CELL_HEIGHT;

        driver->heartbeat = CFontz633_heartbeat;

	report (RPT_DEBUG, "CFontz633_init: done\n");

	return 0;
}

/*
 * Clean-up
 */
void
CFontz633_close ()
{
	debug(RPT_INFO, "CFontz633: close" );
	close (fd);

	if (CFontz633->framebuf)
		free (CFontz633->framebuf);

	if (backingstore)
		free (backingstore);

	CFontz633->framebuf = NULL;
	backingstore = NULL;
}


/*
 * Flushes all output to the lcd...
 */
void
CFontz633_flush ()
{
	int i;
	char *xp, *xq;

/*
 * We don't use delta update yet.
 * It should be possible but since we can only update one line at a time.
 */

debug(RPT_INFO, "CFontz633: flush" );

xp = CFontz633->framebuf;
xq = backingstore;

for (i=0; i<CFontz633->wid; i++) {
	if (*xp != *xq) {
send_bytes_message(fd, 16, CF633_Set_LCD_Contents_Line_One, &CFontz633->framebuf[0]);
memcpy(backingstore, CFontz633->framebuf, CFontz633->wid);
		break;
		}
	xp++; xq++;
	}

xp = &CFontz633->framebuf[CFontz633->wid];
xq = &backingstore[CFontz633->wid];

for (i=0; i<CFontz633->wid; i++) {
	if (*xp != *xq) {
send_bytes_message(fd, 16, CF633_Set_LCD_Contents_Line_Two, &CFontz633->framebuf[CFontz633->wid]);
memcpy(&backingstore[CFontz633->wid], &CFontz633->framebuf[CFontz633->wid], CFontz633->wid);
		break;
		}
	xp++; xq++;
	}

}

/* TODO: NOT NEEDED or DUMMY */
/*
 * Flushes all output to the lcd...
 */
void
CFontz633_flush_box (int lft, int top, int rgt, int bot)
{
debug(RPT_INFO, "CFontz633: flush_box" );
CFontz633_flush ();
}


/*
 * Prints a character on the lcd display, at position (x,y).
 * The upper-left is (1,1), and the lower right should be (16,2).
 */
void
CFontz633_chr (int x, int y, char c)
{
	debug(RPT_INFO, "CFontz633: chr" );

	y--;
	x--;

	CFontz633->framebuf[(y * CFontz633->wid) + x] = c;
}

/*
 *  Changes screen contrast (valid hardware value: 0-50)
 */
int
CFontz633_contrast (int contrast)
{
	static int status = CFONTZ633_DEF_CONTRAST;

	debug(RPT_INFO, "CFontz633: contrast" );

	/* Check it */
	if((contrast >= 0) && (contrast <= 50)) {
		status = contrast;
		send_onebyte_message(fd, CF633_Set_LCD_Contrast, contrast);
	}

	return status;
}

/* TODO: Backlight is not implemented yet */
/*
 * Sets the backlight on or off.
 * The hardware support any value between 0 and 100.
 * Need to find out if we have support for intermediate value.
 */
void
CFontz633_backlight (int on)
{
	debug(RPT_INFO, "CFontz633: backlight" );
	if (on) {
send_onebyte_message(fd, CF633_Set_LCD_And_Keypad_Backlight, brightness);
	} else {
send_onebyte_message(fd, CF633_Set_LCD_And_Keypad_Backlight, offbrightness);
	}
}

/*
 * Get rid of the blinking curson
 */
void
CFontz633_hidecursor ()
{
	debug(RPT_INFO, "CFontz633: hidecursor" );
	send_onebyte_message(fd, CF633_Set_LCD_Cursor_Style, 0);
}


/*
 * Stop live reporting of temperature.
 */
void
CFontz633_no_live_report ()
{
char out[2]= {0, 0};

debug(RPT_INFO, "CFontz633: no_live_report" );
for (out[0]=0; out[0]<8; out[0]++)
    send_bytes_message(fd, 2, CF633_Set_Up_Live_Fan_or_Temperature_Display , out);
}

/*
 * Stop the reporting of any fan.
 */
void
CFontz633_no_fan_report ()
{
debug(RPT_INFO, "CFontz633: no_fan_report" );
send_onebyte_message(fd, CF633_Set_Up_Fan_Reporting, 0);
}

/*
 * Stop the reporting of any temperature.
 */
void
CFontz633_no_temp_report ()
{
char out[4]= {0, 0, 0, 0};

debug(RPT_INFO, "CFontz633: no_temp_report" );
send_bytes_message(fd, 4, CF633_Set_Up_Temperature_Reporting, out);
}

/*
 * Reset the display bios
 */
void
CFontz633_reboot ()
{
char out[3]= {8, 18, 99};

debug(RPT_INFO, "CFontz633: reboot" );
send_bytes_message(fd, 3, CF633_Reboot, out);
}

/*
 * Return one char from the KeyRing
 */
static char
CFontz633_getkey ()
{
	unsigned char akey;

	if ((akey = GetKeyFromKeyRing()))
	{
	if (akey==1) {
		akey = INPUT_PAUSE_KEY;
	} else if (akey==3) {
		akey = INPUT_BACK_KEY;
	} else if (akey==4) {
		akey = INPUT_FORWARD_KEY;
	} else if (akey==2) {
		akey = INPUT_MAIN_MENU_KEY;
	} 
/*
 * Currently only 4 keys are defined.
 */
	else { akey=('A'-1+akey); }
	}

	return akey;
}

/*
 * Sets up for vertical bars.
 */
static void
CFontz633_init_vbar ()
{
	char a[] = {
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1,
	};
	char b[] = {
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
	};
	char c[] = {
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
	};
	char d[] = {
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
	};
	char e[] = {
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
	};
	char f[] = {
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
	};
	char g[] = {
		0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
		0, 1, 1, 1, 1, 1,
	};

	if (custom != vbar) {
		CFontz633_set_char (1, a);
		CFontz633_set_char (2, b);
		CFontz633_set_char (3, c);
		CFontz633_set_char (4, d);
		CFontz633_set_char (5, e);
		CFontz633_set_char (6, f);
		CFontz633_set_char (7, g);
		custom = vbar;
	}
}

/*
 * Inits horizontal bars...
 */
static void
CFontz633_init_hbar ()
{

	char a[] = {
		1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0,
	};
	char b[] = {
		1, 1, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0,
		1, 1, 0, 0, 0, 0,
	};
	char c[] = {
		1, 1, 1, 0, 0, 0,
		1, 1, 1, 0, 0, 0,
		1, 1, 1, 0, 0, 0,
		1, 1, 1, 0, 0, 0,
		1, 1, 1, 0, 0, 0,
		1, 1, 1, 0, 0, 0,
		1, 1, 1, 0, 0, 0,
		1, 1, 1, 0, 0, 0,
	};
	char d[] = {
		1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 0, 0,
		1, 1, 1, 1, 0, 0,
	};
	char e[] = {
		1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 0,
		1, 1, 1, 1, 1, 0,
	};
	char f[] = {
		1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1,
	};

	if (custom != hbar) {
		CFontz633_set_char (1, a);
		CFontz633_set_char (2, b);
		CFontz633_set_char (3, c);
		CFontz633_set_char (4, d);
		CFontz633_set_char (5, e);
		CFontz633_set_char (6, f);
		custom = hbar;
	}
}


/*
 * Draws a vertical bar...
 */
void
CFontz633_vbar (int x, int len)
{
        char map[9] = { 32, 1, 2, 3, 4, 5, 6, 7, 255 };

        int y;
        for (y = CFontz633->hgt; y > 0 && len > 0; y--) {
                if (len >= CFontz633->cellhgt)
                        CFontz633_chr (x, y, 255);
                else
                        CFontz633_chr (x, y, map[len]);

                len -= CFontz633->cellhgt;
        }

}


/*
 * Draws a horizontal bar to the right.
 */
void
CFontz633_hbar (int x, int y, int len)
{
        char map[7] = { 32, 1, 2, 3, 4, 5, 6 };

        for (; x <= CFontz633->wid && len > 0; x++) {
                if (len >= CFontz633->cellwid)
                        CFontz633_chr (x, y, map[6]);
                else
                        CFontz633_chr (x, y, map[len]);

                len -= CFontz633->cellwid;

        }

}

/* TODO: Find what to do if bignum are not available/possible */
/*
 * Writes a big number.
 * This is not supported on 633 because we only have 2 lines...
 */
void
CFontz633_num (int x, int num)
{
}

/*
 * Sets a custom character from 0-7...
 *
 * For input, values > 0 mean "on" and values <= 0 are "off".
 *
 * The input is just an array of characters...
 */
void
CFontz633_set_char (int n, char *dat)
{
	char out[9];
	int row, col;
	int letter;

	if (n < 0 || n > 7)
		return;
	if (!dat)
		return;

	out[0]=n;	/* Custom char to define. xxx */

	for (row = 0; row < CFontz633->cellhgt; row++) {
		letter = 0;
		for (col = 0; col < CFontz633->cellwid; col++) {
			letter <<= 1;
			letter |= (dat[(row * CFontz633->cellwid) + col] > 0);
		}
		out[row+1]=letter;
	}
send_bytes_message(fd, 9, CF633_Set_LCD_Special_Character_Data , out);
}

/*
 * Places an icon on screen
 */
static void
CFontz633_icon (int which, char dest)
{
	char icons[3][6 * 8] = {
		{
		 1, 1, 1, 1, 1, 1,		  /* Empty Heart */
		 1, 1, 0, 1, 0, 1,
		 1, 0, 0, 0, 0, 0,
		 1, 0, 0, 0, 0, 0,
		 1, 0, 0, 0, 0, 0,
		 1, 1, 0, 0, 0, 1,
		 1, 1, 1, 0, 1, 1,
		 1, 1, 1, 1, 1, 1,
		 },

		{
		 1, 1, 1, 1, 1, 1,		  /* Filled Heart */
		 1, 1, 0, 1, 0, 1,
		 1, 0, 1, 0, 1, 0,
		 1, 0, 1, 1, 1, 0,
		 1, 0, 1, 1, 1, 0,
		 1, 1, 0, 1, 0, 1,
		 1, 1, 1, 0, 1, 1,
		 1, 1, 1, 1, 1, 1,
		 },

		{
		 0, 0, 0, 0, 0, 0,		  /* Ellipsis */
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 1, 0, 1, 0, 1, 0,
		 },

	};

        CFontz633_set_char (dest, &icons[which][0]);
}


/*
 * Clears the LCD screen
 */
void
CFontz633_clear ()
{
	memset (CFontz633->framebuf, ' ', CFontz633->wid * CFontz633->hgt);
}

/*
 * Hardware clears the LCD screen
 */
void
CFontz633_hardware_clear ()
{
	send_zerobyte_message(fd, CF633_Clear_LCD_Screen);
}

/*
 * Prints a string on the lcd display, at position (x,y).  The
 * upper-left is (1,1), and the lower right should be (16,2).
 */
void
CFontz633_string (int x, int y, char string[])
{
	int i;

	/* Convert 1-based coords to 0-based... */
	x -= 1;
	y -= 1;

	for (i = 0; string[i]; i++) {

		/* Check for buffer overflows... */
		if ((y * CFontz633->wid) + x + i > (CFontz633->wid * CFontz633->hgt))
			break;
		CFontz633->framebuf[(y * CFontz633->wid) + x + i] = string[i];
	}
}

/*
 * Does the heartbeat thing
 */
static void
CFontz633_heartbeat (int type)
{
        static int timer = 0;
        int whichIcon;
        static int saved_type = HEARTBEAT_ON;

        if (type)
                saved_type = type;

        if (type == HEARTBEAT_ON) {
                // Set this to pulsate like a real heart beat...
                whichIcon = (! ((timer + 4) & 5));

                // This defines a custom character EVERY time...
                // not efficient... is this necessary?
                CFontz633_icon (whichIcon, 0);

                // Put character on screen...
                CFontz633_chr (CFontz633->wid, 1, 0);

                // change display...
                CFontz633_flush ();
       }

       timer++;
       timer &= 0x0f;
}


