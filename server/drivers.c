/*
 * driver.c
 * This file is part of LCDd, the lcdproc server.
 *
 * This file is released under the GNU General Public License. Refer to the
 * COPYING file distributed with this package.
 *
 * Copyright (c) 1999, William Ferrell, Scott Scriven
 *               2001, Joris Robijn
 *               2002, Rene Wagner
 *
 *
 * This code does all driver handling, loading, initializing, unloading.
 *
 */

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "shared/LL.h"
#include "shared/report.h"
#include "configfile.h"

#include "drivers/lcd.h"
/* lcd.h is used for the driver API definition */


LinkedList * loaded_drivers = NULL;

int empty_int_function() { return 0; }
void empty_void_function() { return;}
char empty_char_function() { return 0; }
char * empty_char_pointer_function() { return ""; }

void empty_icon_function (int which, char dest) { return; }
void empty_chr_function (int x, int y, char c) { return; };

static int fill_driver_functions( lcd_logical_driver * driver );

static int store_private_ptr(struct lcd_logical_driver * driver, void * private_data);

int
load_driver ( char * name, char * filename, char * args )
{
	int res;
	int (*driver_init)();
	lcd_logical_driver * driver;

	report( RPT_INFO, "load_driver(%s,%s,%s)", name, filename, args );

	/* First driver ?*/
	if( !loaded_drivers ) {
		/* Create linked list*/
		loaded_drivers = LL_new ();
		if( !loaded_drivers ) {
			report( RPT_ERR, "Error allocating driver list." );
			return -1;
		}
	}


	/* Find the driver in the array of driver types*/
	if ((driver_init = (void *) lcd_find_init(name)) == NULL) {
		/* Driver not found*/
		report( RPT_ERR, "invalid driver: %s", name);
		return -1;
	}


	/* Allocate memory for new driver struct*/
	driver = malloc( sizeof( lcd_logical_driver ));
	/*memset( driver, 0, sizeof (lcd_logical_driver ));*/

	fill_driver_functions( driver );


	/* Rebind the init function and call it*/
	driver->init = driver_init;

	res = driver->init( driver, args );
	if( res < 0 ) {
		report( RPT_ERR, "Driver load failed, return code < 0" );
		/* driver load failed, don't add driver to list*/
		return -1;
	}

	/* Add driver to list*/
	LL_Push( loaded_drivers, driver );

	/* Set lcd_ptr (which is used for output) to driver, if it's
	 * the first output driver in the list (lcd_ptr==NULL)*/
	if ((lcd_ptr == NULL) && (driver->chr != empty_chr_function))
		lcd_ptr = driver;

	/* Check the driver type*/
	if( !driver->daemonize ) {
		return 2; /* it should be run in foreground */
	}

	/* Does it do output ? */
	return ( driver->chr != empty_chr_function ) ? 1 : 0;
}


int
unload_all_drivers ()
{
	lcd_logical_driver * driver;

	report( RPT_INFO, "unload_all_driver()");

	while( (driver = LL_Pop( loaded_drivers )) != NULL ) {
		debug( RPT_DEBUG, "driver->close %p", driver );
		driver->close();
	}

	return 0;
}


static int
fill_driver_functions( struct lcd_logical_driver * driver )
{
	driver->wid = LCD_STD_WIDTH;
	driver->hgt = LCD_STD_HEIGHT;

	driver->cellwid = LCD_STD_CELL_WIDTH;
	driver->cellhgt = LCD_STD_CELL_HEIGHT;

	driver->framebuf = NULL;
	driver->nextkey = NULL;

	driver->daemonize = 1;

	/* Set pointers to empty function*/

	/* Basic functions*/
	driver->init = empty_int_function;
	driver->close = empty_void_function;

	driver->getinfo = empty_char_pointer_function;
	/* and don't forget other get_* functions later...*/

	driver->clear = empty_void_function;
	driver->flush = empty_void_function;
	driver->string = empty_void_function;
	driver->chr = empty_chr_function;

	/* Extended functions*/
	driver->init_vbar = empty_void_function;
	driver->vbar = empty_void_function;
	driver->init_hbar = empty_void_function;
	driver->hbar = empty_void_function;
	driver->init_num = empty_void_function;
	driver->num = empty_void_function;
	driver->heartbeat = empty_void_function;

	/* Hardware functions*/
	driver->contrast = empty_int_function;
	driver->backlight = empty_void_function;
	driver->output = empty_void_function;

	/* Userdef character functions*/
	driver->set_char = empty_void_function;
	driver->icon = empty_icon_function;

	/* Key functions*/
	driver->getkey = empty_char_function;

	/* Ancient functions*/
	driver->flush_box = empty_void_function;
	driver->draw_frame = empty_void_function;

	/* Config file functions*/
	driver->config_get_bool		= config_get_bool;
	driver->config_get_int		= config_get_int;
	driver->config_get_float	= config_get_float;
	driver->config_get_string	= config_get_string;
	driver->config_has_section	= config_has_section;
	driver->config_has_key		= config_has_key;

	/* Driver private data*/
	driver->store_private_ptr	= store_private_ptr;

	return 0;
}

char
drivers_getkey()
{
	lcd_logical_driver * driver;
	char ch = 0;

	report( RPT_INFO, "drivers_getkey()");

	for( driver = LL_GetFirst(loaded_drivers);
	  driver != NULL && ch == 0;
	  driver = LL_GetNext(loaded_drivers) ) {
		debug( RPT_DEBUG, "driver->getkey %p", driver );
		ch = driver->getkey();
	}
	debug( RPT_DEBUG, "driver_getkey returns char %d ('%c')", ch, ch );
	return ch;
}

static int
store_private_ptr(struct lcd_logical_driver * driver, void * private_data)
{
	driver->private_data = private_data;
	return 0;
}
