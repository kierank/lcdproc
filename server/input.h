/*
 * input.h
 * This file is part of LCDd, the lcdproc server.
 *
 * This file is released under the GNU General Public License. Refer to the
 * COPYING file distributed with this package.
 *
 * Copyright 	(c) 1999, William Ferrell, Scott Scriven
 *		(c) 2001, Rene Wagner
 *
 */

#ifndef INPUT_H
#define INPUT_H

/* Accepts and uses keypad input while displaying screens...*/
int handle_input ();

/* These defines should be used by drivers for version 0.4.3 of LCDproc
 * as return values for _getkey().
 */
#define PAUSE_KEY	'A'
#define BACK_KEY	'B'
#define FORWARD_KEY	'C'
#define MAIN_MENU_KEY	'D'

#endif
