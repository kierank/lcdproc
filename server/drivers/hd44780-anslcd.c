/*
 * Driver module for Hitachi HD44780 built into ANS (Apple Network Server)
 *
 * Copyright (c) 2002 Peter Castro <doctor@fruitbat.org> GNU Public License  
 *
 * History:
 *   Complete re-write, based on hd44780-picanlcd.[ch] by
 *   Peter Castro <doctor@fruitbat.org>
 *   Basically, the design of LCDproc changed such that the original driver
 *   code was nolonger applicable.  The new design allowed for sub-drivers
 *   to implemement just the interface specific calls needed while a generic
 *   HD44780 driver provides all the regular HD44780 commands and calls
 *   this subdriver to do the actual work.  A much better design, in my
 *   opinion :)
 *
 *   Original LCDproc driver based on LCDprod-0.4-pre9 by
 *   Richard Rognlie <rrognlie@gamerz.net> (1998)
 *
 * Copyright (c) 1998 Richard Rognlie       GNU Public License  
 *                    <rrognlie@gamerz.net>
 *
 * Large quantities of this code lifted (nearly verbatim) from
 * the lcd4.c module of lcdtext.  Copyright (C) 1997 Matthias Prinke
 * <m.prinke@trashcan.mcnet.de> and covered by GNU's GPL.
 * In particular, this program is free software and comes WITHOUT
 * ANY WARRANTY.
 *
 * Matthias stole (er, adapted) the code from the package lcdtime by
 * Benjamin Tse (blt@mundil.cs.mu.oz.au), August/October 1995
 * which uses the LCD-controller's 8 bit-mode.
 * References: port.h             by <damianf@wpi.edu>
 *             Data Sheet LTN211, Philips
 *             Various FAQs and TXTs about Hitachi's LCD Controller HD44780 -
 *                www.paranoia.com/~filipg is a good starting point  ???   
 *             which seems to have changed to www.repairfaq.org/~filipg
 *             (as of 5th Jan 2000.)
 *
 * Interfacing to the /dev/lcd device provided by Linux on the ANS by
 * Andras Kadinger <bandit@freeside.elte.hu>
 */

#include "hd44780-anslcd.h"
#include "hd44780.h"
#include "lcd.h"
#include "drv_base.h"

#include "shared/str.h"
#include "shared/report.h"
#include "configfile.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>

#include <errno.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define ANSLCD_SENDCTRL 0x02

// Generally, any function that accesses the LCD control lines needs to be
// implemented separately for each HW design. This is typically (but not
// restricted to):
// HD44780_senddata
// HD44780_readkeypad

#define DEFAULT_DEVICE		"/dev/lcd"
#define ANSLCD_SENDCTRL		0x02

/* TODO: replace DriverName with driver->name when that field exists. */
#define DriverName "HD44780"

static int anslcd_fd;
static int anslcd_debug=0;

static void (*driver_close)();

// initialise the driver
int
hd_init_anslcd (HD44780_functions * hd44780_functions, lcd_logical_driver * driver, char *args, unsigned int port)
{
	char device[256] = DEFAULT_DEVICE;

	/* Get debug level */
	anslcd_debug = config_get_int ( DriverName , "debug", 0 , 0 );
	/* Get device path to use */
	strncpy(device,
	  config_get_string ( DriverName , "device" , 0 , DEFAULT_DEVICE),
	  sizeof(device));
	device[sizeof(device)-1]=0;
	report (RPT_INFO,"HD44780: anslcd: Using device: %s", device);

	/* Set up io port correctly, and open it... */
	anslcd_fd = open(device,O_WRONLY);
	if (anslcd_fd == -1) {
		report(RPT_ERR, "HD44780: anslcd: could not open device %s (%s)", device, strerror(errno));
		return -1;
	}
	report (RPT_DEBUG,"HD44780: anslcd: init: fd=%d", anslcd_fd);

	// assign our driver specific functions
	hd44780_functions->senddata = anslcd_HD44780_senddata;
	hd44780_functions->scankeypad = NULL;

	// override a few generic driver functions
	driver_close = driver->close;
	driver->close = anslcd_HD44780_close;

	common_init ();

	return 0;
}

static
void
anslcd_HD44780_decodeioctl(unsigned char ch)
{
  char buf[100];
  buf[0]='\0';
  if (ch & POSITION)
  {
    report(RPT_DEBUG, "HD44780: anslcd: POSITION: 0x%02x",(ch & ~POSITION));
  }
  else
  if (ch & SETCHAR)
  {
    report(RPT_DEBUG, "HD44780: anslcd: SETCHAR: 0x%02x",(ch & ~SETCHAR));
  }
  else
  if (ch & FUNCSET)
  {
    if (ch & IF_8BIT) strcat(buf,"IF_8BIT,"); else strcat(buf,"IF_4BIT,");
    if (ch & TWOLINE) strcat(buf,"TWOLINE,"); else strcat(buf,"ONELINE,");
    if (ch & LARGECHAR) strcat(buf,"LARGECHAR"); else strcat(buf,"SMALLCHAR");
    report(RPT_DEBUG, "HD44780: anslcd: FUNCSET: %s",buf);
  }
  else
  if (ch & CURSORSHIFT)
  {
    if (ch & SCROLLDISP) strcat(buf,"SCROLLDISP,"); else strcat(buf,"MOVECURSOR,");
    if (ch & MOVERIGHT) strcat(buf,"MOVERIGHT"); else strcat(buf,"MOVELEFT");
    report(RPT_DEBUG, "HD44780: anslcd: CURSORSHIFT: %s",buf);
  }
  else
  if (ch & ONOFFCTRL)
  {
    if (ch & DISPON) strcat(buf,"DISPON,"); else strcat(buf,"DISPOFF,");
    if (ch & CURSORON) strcat(buf,"CURSORON,"); else strcat(buf,"CURSOROFF,");
    if (ch & CURSORBLINK) strcat(buf,"CURSORBLINK"); else strcat(buf,"CURSORNOBLINK");
    report(RPT_DEBUG, "HD44780: anslcd: ONOFFCTRL: %s",buf);
  }
  else
  if (ch & ENTRYMODE)
  {
    if (ch & E_MOVERIGHT) strcat(buf,"E_MOVERIGHT,"); else strcat(buf,"E_MOVELEFT,");
    if (ch & EDGESCROLL) strcat(buf,"EDGESCROLL"); else strcat(buf,"NOSCROLL");
    report(RPT_DEBUG, "HD44780: anslcd: ENTRYMODE: %s",buf);
  }
  else
  if (ch & HOMECURSOR)
  {
    report(RPT_DEBUG, "HD44780: anslcd: HOMECURSOR");
  }
  else
  if (ch & CLEAR)
  {
    report(RPT_DEBUG, "HD44780: anslcd: CLEAR");
  }
  else
    report(RPT_DEBUG, "HD44780: anslcd: unknown command");
  return;
}

// anslcd_HD44780_senddata
void
anslcd_HD44780_senddata (unsigned char displayID, unsigned char flags, unsigned char ch)
{
	char str[2];
	int rc;

	if (anslcd_debug) report(RPT_DEBUG, "HD44780: anslcd:senddata: id=%d, flags=%d, [0x%02x]",displayID,flags,ch);
	if (flags == RS_DATA) {
		rc=write(anslcd_fd,&ch,1);
		if (anslcd_debug) report(RPT_DEBUG, "HD44780: anslcd:senddata: write(fd=%d,ch=[0x%02x],len=%d) -> %d",anslcd_fd,ch,1,rc);
	} else {
		str[0]=ch;
		str[1]=0;
		if (anslcd_debug) anslcd_HD44780_decodeioctl(ch);
		rc=ioctl(anslcd_fd,ANSLCD_SENDCTRL,&str);
		if (anslcd_debug) report(RPT_DEBUG, "HD44780: anslcd:senddata: ioctl(SENDCTRL): [0x%02x] -> %d",ch,rc);
	}
}

// anslcd_HD44780_close
void 
anslcd_HD44780_close()
{
	if (anslcd_fd >= 0)
	{
		report (RPT_DEBUG,"HD44780: anslcd: close: fd=%d", anslcd_fd);
		close(anslcd_fd);
		anslcd_fd=-1;
	}
	// call generic close
	driver_close();
}
