/* USBLCD driver module.
 *
 * Copyright (C)  2002 Adams IT Services <info@usblcd.de>
 * This driver is based on hd44780-picanlcd.c . See this file
 * for additional Copyrights.
 *
 * This file is released under the GNU General Public License. Refer to the
 * COPYING file distributed with this package.
 *
 * See http://www.usblcd.de for hardware and documentation.
 *
 */

#include "hd44780-usblcd.h"
#include "hd44780.h"

#include "shared/str.h"
#include "shared/report.h"
#include "configfile.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <errno.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define DEFAULT_DEVICE  "/dev/usb/lcd"

#define GET_HARD_VERSION	1
#define GET_DRV_VERSION		2

void usblcd_HD44780_senddata (unsigned char displayID, unsigned char flags, unsigned char ch);

void usblcd_HD44780_backlight (unsigned char state);

static int fd;

// initialise the driver
int
hd_init_usblcd (HD44780_functions * hd44780_functions, lcd_logical_driver * driver, char *args, unsigned int port)
{
	char device[256] = DEFAULT_DEVICE;
	char buf[128];
        int major,minor;
	/* TODO: replace DriverName with driver->name when that field exists. */
	#define DriverName "HD44780"

	/* Get serial device to use */
	strncpy(device, config_get_string ( DriverName , "device" , 0 , DEFAULT_DEVICE),sizeof(device));
	device[sizeof(device)-1]=0;
	report (RPT_INFO,"HD44780: USBLCD: Using device: %s", device);

	// open it...
	fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1) {
		report(RPT_ERR, "HD44780: USBLCD: could not open device %s (%s)\n", device, strerror(errno));
		return -1;
	}

	memset(buf,0,128);

	if( ioctl(fd,GET_DRV_VERSION, buf)!=0) {
	  report(RPT_ERR,"IOCTL failed, could not get Driver Version!\n");
	  return -2;
	} ;
	
	report(RPT_INFO,"Driver Version: %s\n",buf);

        if( sscanf(buf,"USBLCD Driver Version %d.%d",&major,&minor)!=2) {
	  report(RPT_ERR,"Could not read Driver Version!\n");
	  return -4;
	};

	if(major!=1) {
	  report(RPT_ERR,"Driver Version not supported!\n");
          return -4;
	}

	memset(buf,0,128);

	if( ioctl(fd,GET_HARD_VERSION, buf)!=0) {
	  report(RPT_ERR,"IOCTL failed, could not get Hardware Version!\n");
          return -3;
	};
  
        report(RPT_INFO,"Hardware Version: %s\n",buf);

        if( sscanf(buf,"%d.%d",&major,&minor)!=2) {
	  report(RPT_ERR,"Could not read Hardware Version!\n");
	  return -4;
	};

	if(major!=1) {
	  report(RPT_ERR,"Hardware Version not supported!\n");
          return -4;
	}

	hd44780_functions->senddata = usblcd_HD44780_senddata;
	hd44780_functions->backlight = usblcd_HD44780_backlight;

	common_init ();
	return 0;
}

// usblcd_HD44780_senddata
void
usblcd_HD44780_senddata (unsigned char displayID, unsigned char flags, unsigned char ch)
{
    static const char instr_byte=0;
            
    if( flags==RS_DATA ) {
	if( ch==0 ) write( fd, &ch, 1 );
        write( fd, &ch, 1 );
    } else {
	write( fd, &instr_byte, 1 );
	write( fd, &ch, 1 );
    }
}

void
usblcd_HD44780_backlight (unsigned char state)
{
    static const char instr_byte=0;
    static const char bl_on =0x21;
    static const char bl_off=0x20;
    
    write( fd, &instr_byte, 1 );
    
    if( state==0 ) {
	write( fd, &bl_off, 1 );
    } else {
	write( fd, &bl_on , 1 );
    }
}
