/*  This is the LCDproc driver for LIRC infrared devices (http://www.lirc.org)

    Copyright (C) 2000, Harald Klein
		  2002, Rene Wagner

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


#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <lirc/lirc_client.h>

#define __u32 unsigned int
#define __u8 unsigned char

#define NAME_LENGTH 128

#include "lcd.h"
#include "lircin.h"

#include "shared/str.h"
#include "shared/report.h"
#include "configfile.h"

static void lircin_close ();
static char lircin_getkey ();

char *progname = "lircin";

static int fd;
char buf[256];
struct sockaddr_un addr;

static struct lirc_config *config;


lcd_logical_driver *lircin;

//void sigterm(int sig)
//{
//  ir_free_commands();
//  ir_finish();
//  raise(sig);
//}

static void
lircin_close ()
{
	lirc_freeconfig (config);
	lirc_deinit ();
}

//////////////////////////////////////////////////////////////////////
// Tries to read a character from an input device...
//
// Return 0 for "nothing available".
//
static char
lircin_getkey ()
{
	char key=0;
	char *code=NULL, *cmd=NULL;

	if ( (lirc_nextcode(&code)==0) && (code!=NULL) ) {
		if ( (lirc_code2char(config,code,&cmd)==0) && (cmd!=NULL) ) {
			report (RPT_DEBUG, "lircin: \"%s\"", cmd);
			sscanf (cmd, "%c", &key);
		}
		free (code);
	}

	return key;
}

////////////////////////////////////////////////////////////
// init() should set up any device-specific stuff, and
// point all the function pointers.
int
lircin_init (struct lcd_logical_driver *driver, char *args)
{

/* assign funktions */

	lircin = driver;

	driver->getkey = lircin_getkey;
	driver->close = lircin_close;

/* open socket to lirc */

	if (-1 == (fd = lirc_init ("lcdd", LIRCIN_VERBOSELY))) {
		report( RPT_ERR, "lircin: Could not connect to lircd." );
		return -1;
	}

	if (0 != lirc_readconfig (NULL, &config, NULL)) {
		lirc_deinit ();
		report( RPT_ERR, "lircin: lirc_readconfig() failed." );
		return -1;
	}
	fcntl (fd, F_SETFL, O_NONBLOCK);
	fcntl (fd, F_SETFD, FD_CLOEXEC);

/* socket shouldn block lcdd */

	fcntl (fd, F_SETFL, O_NONBLOCK);
	fcntl (fd, F_SETFD, FD_CLOEXEC);

	return 1;						  // 200 is arbitrary.  (must be 1 or more)
}
