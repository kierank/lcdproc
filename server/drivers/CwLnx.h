/*  This is the LCDproc driver for Cwlinux devices (http://www.cwlinux.com)

    Copyright (C) 2002, Andrew Ip
                  2002, David Glaude

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

#ifndef CWLNX_H
#define CWLNX_H

extern lcd_logical_driver *CwLnx;

int CwLnx_init(lcd_logical_driver * driver, char *device);
void CwLnx_close();
void CwLnx_flush();
void CwLnx_flush_box(int lft, int top, int rgt, int bot);
void CwLnx_chr(int x, int y, char c);
int CwLnx_contrast(int contrast);
void CwLnx_backlight(int on);
void CwLnx_init_vbar();
void CwLnx_init_hbar();
void CwLnx_vbar(int x, int len);
void CwLnx_hbar(int x, int y, int len);
void CwLnx_init_num();
void CwLnx_num(int x, int num);
void CwLnx_set_char(int n, char *dat);
void CwLnx_icon(int which, char dest);
void CwLnx_draw_frame(char *dat);
void CwLnx_clear(void);
void CwLnx_string(int x, int y, char string[]);


#define CWLNX_DEF_CELL_WIDTH 6
#define CWLNX_DEF_CELL_HEIGHT 8
#define CWLNX_DEF_DEVICE "/dev/lcd"
#define CWLNX_DEF_SPEED B19200
#define CWLNX_DEF_SIZE "20x4"

/* These are the keys for a (possibly) broken LK202-25...*/
/* NOTE: You should configure these settings in the configfile
 *       These defines are just used to get *some* defaults
 */
#define CWLNX_KEY_UP    'A'
#define CWLNX_KEY_DOWN  'B'
#define CWLNX_KEY_LEFT  'C'
#define CWLNX_KEY_RIGHT 'D'
#define CWLNX_KEY_YES   'E'
#define CWLNX_KEY_NO    'F'

#define CWLNX_DEF_PAUSE_KEY	CWLNX_KEY_UP
#define CWLNX_DEF_BACK_KEY	CWLNX_KEY_LEFT
#define CWLNX_DEF_FORWARD_KEY	CWLNX_KEY_RIGHT
#define CWLNX_DEF_MAIN_MENU_KEY	CWLNX_KEY_DOWN

#endif
