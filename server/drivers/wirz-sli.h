/*	wirz-sli.h -- Header file for LCDproc Wirz SLI driver
	Copyright (C) 1999 Horizon Technologies-http://horizon.pair.com/
	Written by Bryan Rittmeyer <bryanr@pair.com> - Released under GPL
			
        LCD info: http://www.wirz.com/sli/

	Modified for LCDproc 0.4.4 (C) 2002 by Rene Wagner
*/

#ifndef SLI_H
#define SLI_H

extern lcd_logical_driver *sli;

int sli_init (lcd_logical_driver * driver, char *device);

#endif
