/*  This is the LCDproc driver for Matrix Orbital devices (http://www.matrixorbital.com)

    For the Matrix Orbital LCD* LKD* VFD* and VKD* displays

    NOTE: GLK displays have a different driver.

    Copyright (C) 1999, William Ferrell and Scott Scriven
		  2001, André Breiler
		  2001, Philip Pokorny
		  2001, David Douthitt
		  2001, David Glaude
		  2001, Joris Robijn
		  2001, Rene Wagner

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

#ifndef MTXORB_H
#define MTXORB_H

extern lcd_logical_driver *MtxOrb;

int MtxOrb_init (lcd_logical_driver * driver, char *device);

/* The following values should be set in the configfile
 * but we need defaults ;)
 */
#define MTXORB_DEF_BACKLIGHT	1
#define MTXORB_DEF_CONTRAST	120
#define MTXORB_DEF_DEVICE	"/dev/lcd"
#define MTXORB_DEF_SPEED	B19200
#define MTXORB_DEF_SIZE		"20x4"
#define MTXORB_DEF_TYPE		"lcd"
/* End of configfile defaults */

#define MTXORB_DEF_LINEWRAP	1
#define MTXORB_DEF_AUTOSCROLL	1
#define MTXORB_DEF_CURSORBLINK	0

/* These are the keys for a (possibly) broken LK202-25...*/
/* NOTE: You should configure these settings in the configfile
 *       These defines are just used to get *some* defaults
 */
#define MTXORB_KEY_UP    'I'
#define MTXORB_KEY_DOWN  'F'
#define MTXORB_KEY_LEFT  'K'
#define MTXORB_KEY_RIGHT 'A'
#define MTXORB_KEY_F1    'N'

#define MTXORB_DEF_PAUSE_KEY		MTXORB_KEY_F1
#define MTXORB_DEF_BACK_KEY		MTXORB_KEY_LEFT
#define MTXORB_DEF_FORWARD_KEY		MTXORB_KEY_RIGHT
#define MTXORB_DEF_MAIN_MENU_KEY	MTXORB_KEY_DOWN


#endif

