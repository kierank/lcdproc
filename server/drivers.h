/*
 * drivers.h
 * This file is part of LCDd, the lcdproc server.
 *
 * This file is released under the GNU General Public License. Refer to the
 * COPYING file distributed with this package.
 *
 * Copyright (c) 2001, Joris Robijn
 *
 */

#ifndef DRIVERS_H
#define DRIVERS_H

#include "drivers/lcd.h"

typedef struct DisplayProps {
	int width, height;
	int cellwidth, cellheight;
} DisplayProps;

extern DisplayProps * display_props;

#define bool int


int
drivers_load_driver( char * name );
/* returns:
 * <0 error
 *  0 ok, driver is an input driver only
 *  1 ok, driver is an output driver
 *  2 ok, driver is an output driver that needs to run in the foreground
 */

int
drivers_unload_all();

char *
drivers_get_info();

void
drivers_clear();

void
drivers_flush();

void
drivers_string( int x, int y, char * string );

void
drivers_chr( int x, int y, char c );

void
drivers_init_vbar();	/* TO BE REMOVED */

void
drivers_init_hbar();	/* TO BE REMOVED */

void
drivers_init_num();	/* TO BE REMOVED */

void
drivers_vbar( int x, int y, int len, int promille, int pattern );

void
drivers_hbar( int x, int y, int len, int promille, int pattern );

void
drivers_num( int x, int num );

void
drivers_heartbeat( int state );

void
drivers_icon( int x, int y, int icon );

void
drivers_set_char( char ch, char *dat );

int
drivers_get_contrast();

void
drivers_set_contrast( int contrast );

void
drivers_backlight( int brightness );

void
drivers_output( int state );

char *
drivers_get_key();

char
drivers_getkey();

#endif
