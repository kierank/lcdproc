/*  This is the LCDproc driver for LIRC infrared devices (http://www.lirc.org)

    Copyright (C) 2000, Harald Klein
		  2002, Rene Wagner
    [Merged some stuff from a different lircin driver, so:]
		  1999, David Glaude

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

static int lircin_fd;

static struct lirc_config *lircin_irconfig;


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
	lirc_freeconfig (lircin_irconfig);
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
		if ( (lirc_code2char(lircin_irconfig,code,&cmd)==0) && (cmd!=NULL) ) {
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
	char s[200]="";
	char *lircrc, *prog;

	lircrc=NULL;
	prog=NULL;

	lircin = driver;

	debug( RPT_INFO, "lircin: init(%p,%s)", driver, args );

	/* TODO: replace DriverName with driver->name when that field exists.*/
	#define DriverName "lircin"

	/* READ CONFIG FILE:*/

	/* Get location of lircrc to be used */
	if (config_get_string ( DriverName , "lircrc" , 0 , NULL) != NULL)
		strncpy(s, config_get_string ( DriverName , "lircrc" , 0 , ""), sizeof(s));

	if (strcmp(s, "") != 0) {
		s[sizeof(s)-1]=0;
		lircrc=malloc(strlen(s)+1);
		strcpy(lircrc,s);
		report  (RPT_INFO,"lircin: Using lircrc: %s", lircrc);
	}
	else {
		report (RPT_INFO,"lircin: Using default lircrc: ~/.lircrc");
	}

	/* Get program identifier "prog=..." to be used */
	if (config_get_string ( DriverName , "prog" , 0 , NULL) != NULL)
		strncpy(s, config_get_string ( DriverName , "prog" , 0 , ""), sizeof(s));

	if (strcmp(s, "") != 0) {
		s[sizeof(s)-1]=0;
		report  (RPT_INFO,"lircin: Using prog: %s", prog);
	}
	else {
		strcpy(s, LIRCIN_DEF_PROG);
	}
	prog=malloc(strlen(s)+1);
	strcpy(prog,s);
	report  (RPT_INFO,"lircin: Using prog: %s", prog);

	/* End of config file parsing */


/* open socket to lirc */

	if (-1 == (lircin_fd = lirc_init (prog, LIRCIN_VERBOSELY))) {
		report( RPT_ERR, "lircin: Could not connect to lircd." );
		return -1;
	}

	if (0 != lirc_readconfig (lircrc, &lircin_irconfig, NULL)) {
		lirc_deinit ();
		report( RPT_ERR, "lircin: lirc_readconfig() failed." );
		return -1;
	}

/* socket shouldn't block lcdd */

	if (fcntl(lircin_fd, F_SETFL, O_NONBLOCK) < 0){
                report(RPT_ERR, "Unable to change lircin_fd(%d) to O_NONBLOCK: %s",
			lircin_fd, strerror(errno));
		return -1;
		}
	fcntl (lircin_fd, F_SETFD, FD_CLOEXEC);

/* assign functions */

	driver->daemonize=1; /* make the server daemonize after initialisation*/

	driver->close = lircin_close;
	driver->getkey = lircin_getkey;

	return 1;						  // 200 is arbitrary.  (must be 1 or more)
}
