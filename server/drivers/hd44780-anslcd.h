/* Driver module for Hitachi HD44780 built into ANS (Apple Network Server)
 *  
 * Copyright (c) 2002 Peter Castro <doctor@fruitbat.org> GNU Public License  
 *
 * History:
 *   Complete re-write, based on hd44780-picanlcd.[ch] by
 *   Peter Castro <doctor@fruitbat.org>
 *   Basically, the design of LCDproc changed such that the original driver
 *   code was nolonger applicable.  The new design allowed for sub-drivers
 *   to implemement just the interface specific calls needed while a generic
 *   HD44780 driver provides all the regular HD44780 commands and calls
 *   this subdriver to do the actual work.  A much better design, in my
 *   opinion :)
 *
 *   Original LCDproc driver based on LCDprod-0.4-pre9 by
 *   Richard Rognlie <rrognlie@gamerz.net> (1998)
 *
 * Copyright (c) 1998 Richard Rognlie       GNU Public License  
 *                    <rrognlie@gamerz.net>
 *
 * Large quantities of this code lifted (nearly verbatim) from
 * the lcd4.c module of lcdtext.  Copyright (C) 1997 Matthias Prinke
 * <m.prinke@trashcan.mcnet.de> and covered by GNU's GPL.
 * In particular, this program is free software and comes WITHOUT
 * ANY WARRANTY.
 *
 * Matthias stole (er, adapted) the code from the package lcdtime by
 * Benjamin Tse (blt@mundil.cs.mu.oz.au), August/October 1995
 * which uses the LCD-controller's 8 bit-mode.
 * References: port.h             by <damianf@wpi.edu>
 *             Data Sheet LTN211, Philips
 *             Various FAQs and TXTs about Hitachi's LCD Controller HD44780 -
 *                www.paranoia.com/~filipg is a good starting point  ???   
 *             which seems to have changed to www.repairfaq.org/~filipg
 *             (as of 5th Jan 2000.)
 *
 * Interfacing to the /dev/lcd device provided by Linux on the ANS by
 * Andras Kadinger <bandit@freeside.elte.hu>
 */

#ifndef HD_ANSLCD_H
#define HD_ANSLCD_H

#include "lcd.h"			  /* for lcd_logical_driver */
#include "hd44780-low.h"		  /* for HD44780_functions */

int hd_init_anslcd (HD44780_functions * hd44780_functions, lcd_logical_driver * driver, char *args, unsigned int port);

void anslcd_HD44780_senddata (unsigned char displayID, unsigned char flags, unsigned char ch);
void anslcd_HD44780_close ();

#endif
