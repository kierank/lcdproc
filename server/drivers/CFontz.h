/*  This is the LCDproc driver for CrystalFontz devices (http://crystalfontz.com)

    Copyright (C) 1999, William Ferrell and Scott Scriven
    		  2001, Philip Pokorny
		  2001, David Douthitt
		  2001, David Glaude
		  2001, Joris Robijn
		  2001, Eddie Sheldrake
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

#ifndef CFONTZ_H
#define CFONTZ_H

extern lcd_logical_driver *CFontz;

int CFontz_init (lcd_logical_driver * driver, char *device);
void CFontz_close ();
void CFontz_flush ();
void CFontz_flush_box (int lft, int top, int rgt, int bot);
void CFontz_chr (int x, int y, char c);
int CFontz_contrast (int contrast);
void CFontz_backlight (int on);
void CFontz_init_vbar ();
void CFontz_init_hbar ();
void CFontz_vbar (int x, int len);
void CFontz_hbar (int x, int y, int len);
void CFontz_init_num ();
void CFontz_num (int x, int num);
void CFontz_set_char (int n, char *dat);
void CFontz_icon (int which, char dest);
void CFontz_draw_frame (char *dat);
void CFontz_clear (void);
void CFontz_string (int x, int y, char string[]);

#define CFONTZ_DEF_CELL_WIDTH 6
#define CFONTZ_DEF_CELL_HEIGHT 8
#define CFONTZ_DEF_CONTRAST 140
#define CFONTZ_DEF_DEVICE "/dev/lcd"
#define CFONTZ_DEF_SPEED B19200
#define CFONTZ_DEF_BRIGHTNESS 60
#define CFONTZ_DEF_OFFBRIGHTNESS 0
#define CFONTZ_DEF_SIZE "20x4"

#endif
