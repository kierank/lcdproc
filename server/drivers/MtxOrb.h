/*  This is the LCDproc driver for Matrix Orbital devices (http://www.matrix-orbital.com)

    For the Matrix Orbital LCD* LKD* VFD* and VKD* displays

    NOTE: GLK displays have a different driver.

    Copyright (C) 2001 ????

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

/* configfile support added by Rene Wagner (c) 2001*/
/* general cleanup by Rene Wagner (c) 2001*/

#ifndef MTXORB_H
#define MTXORB_H

extern lcd_logical_driver *MtxOrb;

int MtxOrb_init (lcd_logical_driver * driver, char *device);

/* The following values should be set in the configfile
 * but we need defaults ;)
 */
#define DEFAULT_BACKLIGHT	1
#define DEFAULT_CONTRAST	120
#define DEFAULT_DEVICE		"/dev/lcd"
#define DEFAULT_SPEED		B19200
#define DEFAULT_SIZE		"20x4"
#define DEFAULT_TYPE		"lcd"
/* End of configfile defaults */

#define DEFAULT_LINEWRAP	1
#define DEFAULT_AUTOSCROLL	1
#define DEFAULT_CURSORBLINK	0

/* These are the keys for a (possibly) broken LK202-25...*/
#define KEY_UP    'I'
#define KEY_DOWN  'F'
#define KEY_LEFT  'K'
#define KEY_RIGHT 'A'
#define KEY_F1    'N'
/* TODO: add more if you've got any more ;) or correct the settings
 * the actual translation is done in MtxOrb_getkey()
 */

#endif

