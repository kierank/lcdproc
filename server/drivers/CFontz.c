/*  This is the LCDproc driver for CrystalFontz devices (http://crystalfontz.com)

    Copyright (C) 1999, William Ferrell and Scott Scriven
		  2001, Philip Pokorny
		  2001, David Douthitt
		  2001, David Glaude
		  2001, Joris Robijn
		  2001, Eddie Sheldrake
		  2001, Rene Wagner
		  2002, Mike Patnode
		  2002, Luis Llorente

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
#include "CFontz.h"
#include "render.h"
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

/* I had to choose a character almost never used.
 * Value 159 is uggly so I guess no one use it. ;-)
 */
#define DIRTY_CHAR 159

static int fd;
static int newfirmware = 0;
static char* backingstore = NULL;

static void CFontz_linewrap (int on);
static void CFontz_autoscroll (int on);
static void CFontz_hidecursor ();
static void CFontz_reboot ();
static void CFontz_heartbeat (int type);

// TODO:  Get rid of this variable?
lcd_logical_driver *CFontz;
// TODO:  Get the frame buffers working right

/////////////////////////////////////////////////////////////////
// Opens com port and sets baud correctly...
//
int
CFontz_init (lcd_logical_driver * driver, char *args)
{
	struct termios portset;
	int tmp, w, h;
	int reboot = 0;

	int contrast = CFONTZ_DEF_CONTRAST;
	char device[200] = CFONTZ_DEF_DEVICE;
	int speed = CFONTZ_DEF_SPEED;
	char size[200] = CFONTZ_DEF_SIZE;

	char buf[256] = "";

	CFontz = driver;

	debug(RPT_INFO, "CFontz: init(%p,%s)", driver, args );

	/* TODO: replace DriverName with driver->name when that field exists. */
	#define DriverName "CFontz"


	/*Read config file*/

	/*Which serial device should be used*/
	strncpy(device, config_get_string ( DriverName , "Device" , 0 , CFONTZ_DEF_DEVICE),sizeof(device));
	device[sizeof(device)-1]=0;
	report (RPT_INFO,"CFontz: Using device: %s", device);

	/*Which size*/
	strncpy(size, config_get_string ( DriverName , "Size" , 0 , CFONTZ_DEF_SIZE),sizeof(size));
	size[sizeof(size)-1]=0;
	if( sscanf(size , "%dx%d", &w, &h ) != 2
	|| (w <= 0) || (w > LCD_MAX_WIDTH)
	|| (h <= 0) || (h > LCD_MAX_HEIGHT)) {
		report (RPT_WARNING, "CFontz: Cannot read size: %s. Using default value.\n", size);
		sscanf( CFONTZ_DEF_SIZE , "%dx%d", &w, &h );
	}
	driver->wid = w;
	driver->hgt = h;

	/*Which contrast*/
	if (0<=config_get_int ( DriverName , "Contrast" , 0 , CFONTZ_DEF_CONTRAST) && config_get_int ( DriverName , "Contrast" , 0 , CFONTZ_DEF_CONTRAST) <= 255) {
		contrast = config_get_int ( DriverName , "Contrast" , 0 , CFONTZ_DEF_CONTRAST);
	} else {
		report (RPT_WARNING, "CFontz: Contrast must be between 0 and 255. Using default value.\n");
	}

	/*Which speed*/
	tmp = config_get_int ( DriverName , "Speed" , 0 , CFONTZ_DEF_SPEED);

	switch (tmp) {
		case 1200:
			speed = B1200;
			break;
		case 2400:
			speed = B2400;
			break;
		case 4800:
			speed = B4800;
			break;
		case 9600:
			speed = B9600;
			break;
		case 19200:
			speed = B19200;
			break;
		default:
			speed = CFONTZ_DEF_SPEED;
			switch (speed) {
				case B1200:
					strncpy(buf,"1200", sizeof(buf));
					break;
				case B2400:
					strncpy(buf,"2400", sizeof(buf));
					break;
				case B4800:
					strncpy(buf,"4800", sizeof(buf));
					break;
				case B9600:
					strncpy(buf,"9600", sizeof(buf));
					break;
				case B19200:
					strncpy(buf,"19200", sizeof(buf));
					break;
			}
			report (RPT_WARNING , "CFontz: Speed must be 1200, 2400, 4800, 9600, or 19200. Using default value of %s baud!", buf);
			strncpy(buf,"", sizeof(buf));
	}

	/*New firmware version?*/
	if(config_get_bool( DriverName , "NewFirmware" , 0 , 0)) {
		newfirmware = 1;
	}

	/*Reboot display?*/
	if (config_get_bool( DriverName , "Reboot" , 0 , 0)) {
		report (RPT_INFO, "CFontz: Rebooting CrystalFontz LCD...\n");
		reboot = 1;
	}

	/* End of config file parsing*/


	/* Allocate framebuffer memory*/
	/* You must use driver->framebuf here, but may use lcd.framebuf later.*/
	if (!driver->framebuf) {
		driver->framebuf = malloc (driver->wid * driver->hgt);
		backingstore = calloc (driver->wid * driver->hgt, 1);
	}

	if (!driver->framebuf) {
                report(RPT_ERR, "CFontz: Error: unable to create framebuffer.\n");
		return -1;
	}

	/* Set up io port correctly, and open it...*/
	debug( RPT_DEBUG, "CFontz: Opening serial device: %s", device);
	fd = open (device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1) {
		report (RPT_ERR, "CFontz: init() failed (%s)\n", strerror (errno));
		return -1;
	} else {
		report (RPT_INFO, "CFontz: Opened display on %s", device);
	}

	tcgetattr (fd, &portset);

	// We use RAW mode
#ifdef HAVE_CFMAKERAW
	// The easy way
	cfmakeraw( &portset );
#else
	// The hard way
	portset.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP
	                      | INLCR | IGNCR | ICRNL | IXON );
	portset.c_oflag &= ~OPOST;
	portset.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
	portset.c_cflag &= ~( CSIZE | PARENB | CRTSCTS );
	portset.c_cflag |= CS8 | CREAD | CLOCAL ;
#endif

	// Set port speed
	cfsetospeed (&portset, speed);
	cfsetispeed (&portset, B0);

	// Do it...
	tcsetattr (fd, TCSANOW, &portset);

	/* Set display-specific stuff..*/
	if (reboot) {
		CFontz_reboot ();
		sleep (4);
		reboot = 0;
	}
	sleep (1);
	CFontz_hidecursor ();
	CFontz_linewrap (1);
	CFontz_autoscroll (0);
	CFontz_backlight (0);

	/* Set the functions the driver supports...*/

	driver->daemonize = 1; /* make the server daemonize after initialization*/

	driver->clear = CFontz_clear;
	driver->string = CFontz_string;
	driver->chr = CFontz_chr;
	driver->vbar = CFontz_vbar;
	driver->init_vbar = CFontz_init_vbar;
	driver->hbar = CFontz_hbar;
	driver->init_hbar = CFontz_init_hbar;
	driver->num = CFontz_num;

	driver->init = CFontz_init;
	driver->close = CFontz_close;
	driver->flush = CFontz_flush;
	driver->flush_box = CFontz_flush_box;
	driver->contrast = CFontz_contrast;
	driver->backlight = CFontz_backlight;
	driver->set_char = CFontz_set_char;
	driver->icon = CFontz_icon;
	driver->draw_frame = CFontz_draw_frame;

	CFontz_contrast (contrast);

	driver->cellwid = CFONTZ_DEF_CELL_WIDTH;
	driver->cellhgt = CFONTZ_DEF_CELL_HEIGHT;

	driver->heartbeat = CFontz_heartbeat;

	report (RPT_DEBUG, "CFontz_init: done\n");

	return fd;
}

/////////////////////////////////////////////////////////////////
// Clean-up
//
void
CFontz_close ()
{
	close (fd);

	if (CFontz->framebuf)
		free (CFontz->framebuf);

	if (backingstore)
		free (backingstore);

	CFontz->framebuf = NULL;
	backingstore = NULL;
}

void
CFontz_flush ()
{
	CFontz_draw_frame (CFontz->framebuf);
}

void
CFontz_flush_box (int lft, int top, int rgt, int bot)
{
	int y;
	char out[LCD_MAX_WIDTH];

	debug (RPT_DEBUG, "CFontz: flush_box (%i,%i)-(%i,%i)\n", lft, top, rgt, bot);

	for (y = top; y <= bot; y++) {
		snprintf (out, sizeof(out), "%c%c%c", 17, lft, y);
		write (fd, out, 4);
		write (fd, CFontz->framebuf + (y * CFontz->wid) + lft, rgt - lft + 1);

	}

}

/////////////////////////////////////////////////////////////////
// Prints a character on the lcd display, at position (x,y).  The
// upper-left is (1,1), and the lower right should be (20,4).
//
void
CFontz_chr (int x, int y, char c)
{
	y--;
	x--;

	if (c < 32 && c >= 0)
		c += 128;

	/*
	 * For V2 of the firmware to get the block to display right
	*/
	if (newfirmware && c==-1) {
	c=214;
	}

	CFontz->framebuf[(y * CFontz->wid) + x] = c;
}

/////////////////////////////////////////////////////////////////
// Changes screen contrast (0-255; 140 seems good)
//
int
CFontz_contrast (int contrast)
{
	int realcontrast = -1;
	char out[4];
	static int status = 140;

	if (contrast > 0) {
		status = contrast;
		realcontrast = (int) (status * 100 / 255);
		snprintf (out, sizeof(out), "%c%c", 15, realcontrast);
		write (fd, out, 3);
	}

	return status;
}

/////////////////////////////////////////////////////////////////
// Sets the backlight brightness
//
void
CFontz_backlight (int on)
{
	static int current = -1;
	int realbacklight = -1;
	char out[4];

	if (on == current)
		return;

	/* validate backlight value */
	if (on > 255)
		on = 255;
	if (on < 0)
		on = 0;

	current = on;

	realbacklight = (int) (current * 100 / 255);

	snprintf (out, sizeof(out), "%c%c", 14, realbacklight);
	write (fd, out, 3);
}

/////////////////////////////////////////////////////////////////
// Toggle the built-in linewrapping feature
//
static void
CFontz_linewrap (int on)
{
	char out[4];
	if (on)
		snprintf (out, sizeof(out), "%c", 23);
	else
		snprintf (out, sizeof(out), "%c", 24);
	write (fd, out, 1);
}

/////////////////////////////////////////////////////////////////
// Toggle the built-in automatic scrolling feature
//
static void
CFontz_autoscroll (int on)
{
	char out[4];
	if (on)
		snprintf (out, sizeof(out), "%c", 19);
	else
		snprintf (out, sizeof(out), "%c", 20);
	write (fd, out, 1);
}

/////////////////////////////////////////////////////////////////
// Get rid of the blinking curson
//
static void
CFontz_hidecursor ()
{
	char out[4];
	snprintf (out, sizeof(out), "%c", 4);
	write (fd, out, 1);
}

/////////////////////////////////////////////////////////////////
// Reset the display bios
//
static void
CFontz_reboot ()
{
	char out[4];
	snprintf (out, sizeof(out), "%c", 26);
	write (fd, out, 1);
}

/////////////////////////////////////////////////////////////////
// Sets up for vertical bars.  Call before CFontz->vbar()
//
void
CFontz_init_vbar ()
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
		CFontz_set_char (1, a);
		CFontz_set_char (2, b);
		CFontz_set_char (3, c);
		CFontz_set_char (4, d);
		CFontz_set_char (5, e);
		CFontz_set_char (6, f);
		CFontz_set_char (7, g);
		custom = vbar;
	}
}

/////////////////////////////////////////////////////////////////
// Inits horizontal bars...
//
void
CFontz_init_hbar ()
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
		CFontz_set_char (1, a);
		CFontz_set_char (2, b);
		CFontz_set_char (3, c);
		CFontz_set_char (4, d);
		CFontz_set_char (5, e);
		CFontz_set_char (6, f);
		custom = hbar;
	}
}

/////////////////////////////////////////////////////////////////
// Draws a vertical bar...
//
void
CFontz_vbar (int x, int len)
{
	char map[9] = { 32, 1, 2, 3, 4, 5, 6, 7, 255 };

	int y;
	for (y = CFontz->hgt; y > 0 && len > 0; y--) {
		if (len >= CFontz->cellhgt)
			CFontz_chr (x, y, 255);
		else
			CFontz_chr (x, y, map[len]);

		len -= CFontz->cellhgt;
	}

}

/////////////////////////////////////////////////////////////////
// Draws a horizontal bar to the right.
//
void
CFontz_hbar (int x, int y, int len)
{
	char map[7] = { 32, 1, 2, 3, 4, 5, 6 };

	for (; x <= CFontz->wid && len > 0; x++) {
		if (len >= CFontz->cellwid)
			CFontz_chr (x, y, map[6]);
		else
			CFontz_chr (x, y, map[len]);

		len -= CFontz->cellwid;

	}

}


/////////////////////////////////////////////////////////////////
// Writes a big number.
//
void
CFontz_num (int x, int num)
{
	int y, dx;
	char out[5];
	custom = bign;
	snprintf (out, sizeof(out), "%c%c%c%c", 28, 0,  x-1, num+48);
	write (fd, out, 4);

/* Make this space dirty as far as frame buffer knows.*/
	for (y = 1; y < 5; y++)
		for (dx = 0; dx < 3; dx++)
			CFontz_chr (x + dx, y, DIRTY_CHAR);

}

/////////////////////////////////////////////////////////////////
// Sets a custom character from 0-7...
//
// For input, values > 0 mean "on" and values <= 0 are "off".
//
// The input is just an array of characters...
//
void
CFontz_set_char (int n, char *dat)
{
	char out[4];
	int row, col;
	int letter;

	if (n < 0 || n > 7)
		return;
	if (!dat)
		return;

	snprintf (out, sizeof(out), "%c%c", 25, n);
	write (fd, out, 2);

	for (row = 0; row < CFontz->cellhgt; row++) {
		letter = 0;
		for (col = 0; col < CFontz->cellwid; col++) {
			letter <<= 1;
			letter |= (dat[(row * CFontz->cellwid) + col] > 0);
		}
		write (fd, &letter, 1);
	}
}

void
CFontz_icon (int which, char dest)
{
	char icons[3][6 * 8] = {
		{
		 1, 1, 1, 1, 1, 1,		  // Empty Heart
		 1, 0, 1, 0, 1, 1,
		 0, 0, 0, 0, 0, 1,
		 0, 0, 0, 0, 0, 1,
		 0, 0, 0, 0, 0, 1,
		 1, 0, 0, 0, 1, 1,
		 1, 1, 0, 1, 1, 1,
		 1, 1, 1, 1, 1, 1,
		 },

		{
		 1, 1, 1, 1, 1, 1,		  // Filled Heart
		 1, 0, 1, 0, 1, 1,
		 0, 1, 0, 1, 0, 1,
		 0, 1, 1, 1, 0, 1,
		 0, 1, 1, 1, 0, 1,
		 1, 0, 1, 0, 1, 1,
		 1, 1, 0, 1, 1, 1,
		 1, 1, 1, 1, 1, 1,
		 },

		{
		 0, 0, 0, 0, 0, 0,		  // Ellipsis
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0,
		 1, 0, 1, 0, 1, 0,
		 },

	};

	if (custom == bign)
		custom = beat;
	CFontz_set_char (dest, &icons[which][0]);
}

/////////////////////////////////////////////////////////////
// Blasts a single frame onscreen, to the lcd...
//
// Input is a character array, sized CFontz->wid*CFontz->hgt
//
void
CFontz_draw_frame (char *dat)
{
	char out[LCD_MAX_WIDTH * LCD_MAX_HEIGHT];
	char *row, *b_row;
	int i;

	if (!dat)
		return;

	for (i = 0; i < CFontz->hgt; i++) {

		row = dat + (CFontz->wid * i);
		b_row = backingstore + (CFontz->wid * i);

		/* Backing-store implementation.  If it's already
		 * on the screen, don't put it there again
		 */
		if (memcmp(b_row, row, CFontz->wid) == 0)
		    continue;

        /* else, write out the entire row */
		memcpy(b_row, row, CFontz->wid);
		snprintf (out, sizeof(out), "%c%c%c", 17, 0, i);
		write (fd, out, 3);
		write (fd, row, CFontz->wid);
	}

}

/////////////////////////////////////////////////////////////////
// Clears the LCD screen
//
void
CFontz_clear ()
{
	memset (CFontz->framebuf, ' ', CFontz->wid * CFontz->hgt);

}

/////////////////////////////////////////////////////////////////
// Prints a string on the lcd display, at position (x,y).  The
// upper-left is (1,1), and the lower right should be (20,4).
//
void
CFontz_string (int x, int y, char string[])
{
	int i;

	x -= 1;							  // Convert 1-based coords to 0-based...
	y -= 1;

	for (i = 0; string[i]; i++) {

		
		// For V2 of the firmware to get the block to display right
		if (newfirmware && string[i]==-1) {
		string[i]=214;
		}


		// Check for buffer overflows...
		if ((y * CFontz->wid) + x + i > (CFontz->wid * CFontz->hgt))
			break;
		CFontz->framebuf[(y * CFontz->wid) + x + i] = string[i];
	}
}

/////////////////////////////////////////////////////////////
// Does the heartbeat...
//
static void
CFontz_heartbeat (int type)
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
		CFontz_icon (whichIcon, 0);

		// Put character on screen...
		CFontz_chr (CFontz->wid, 1, 0);

		// change display...
		CFontz_flush ();
	}

	timer++;
	timer &= 0x0f;
}
