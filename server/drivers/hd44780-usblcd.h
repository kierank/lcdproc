#ifndef HD_USBLCD_H
#define HD_USBLCD_H

#include "lcd.h"			  /* for lcd_logical_driver */
#include "hd44780-low.h"		  /* for HD44780_functions */

// initialise this particular driver, args is probably not used but keep
// for consistency
int hd_init_usblcd (HD44780_functions * hd44780_functions, lcd_logical_driver * driver, char *args, unsigned int port);

#endif