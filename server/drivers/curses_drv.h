/*  This is the LCDproc driver for ncurses.
    It displays an emulated LCD display at on a terminal screen
    using ncurses.

    Copyright (C) 1999, William Ferrell and Scott Scriven
		  1999, Björn Andersson
		  2001, Philip Pokorny
		  2001, David Douthitt
		  2001, Guilaume Filion
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

#ifndef LCD_CURSES_H
#define LCD_CURSES_H

extern lcd_logical_driver *curses_drv;

int curses_drv_init (struct lcd_logical_driver *driver, char *args);
void curses_drv_backlight (int on);
void curses_drv_close ();
void curses_drv_clear ();
void curses_drv_flush ();
void curses_drv_string (int x, int y, char string[]);
void curses_drv_chr (int x, int y, char c);
void curses_drv_vbar (int x, int len);
void curses_drv_hbar (int x, int y, int len);
void curses_drv_icon (int which, char dest);
void curses_drv_flush ();
void curses_drv_flush_box (int lft, int top, int rgt, int bot);
void curses_drv_draw_frame (char *dat);
char curses_drv_getkey ();
void curses_drv_init_num ();
void curses_drv_num (int x, int num);
void curses_drv_heartbeat (int type);

/*Default settings for config file parsing*/
#define CURSESDRV_DEF_FOREGR "blue"
#define CURSESDRV_DEF_BACKGR "cyan"
#define CURSESDRV_DEF_BACKLIGHT "red"
#define CURSESDRV_DEF_SIZE "20x4"
#define CURSESDRV_DEF_TOP_LEFT_X 7
#define CURSESDRV_DEF_TOP_LEFT_Y 7

#endif
