/*  This is the LCDproc driver for MatrixOrbital GLK Graphic Displays
                                         http://www.matrixorbital.com

    Copyright (C) 2001, Philip Pokorny
		  2001, David Douthitt
		  2001, David Glaude
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


#ifndef GLK_H
#define GLK_H

extern lcd_logical_driver *glk;

int glk_init(struct lcd_logical_driver *driver, char *args);

#define GLK_DEF_DEVICE "/dev/lcd"
#define GLK_DEF_CONTRAST 140
#define GLK_DEF_SPEED B19200

#endif
