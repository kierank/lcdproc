#ifndef MTXORB_H
#define MTXORB_H

extern lcd_logical_driver *MtxOrb;

int MtxOrb_init (lcd_logical_driver * driver, char *device);

/* The following values should be set in the configfile
 * but we need defaults ;)
 */
#define DEFAULT_BACKLIGHT	1
#define DEFAULT_CONTRAST	120
#define DEFAULT_DEVICE		"/dev/lcd"
#define DEFAULT_SPEED		B19200
#define DEFAULT_SIZE		"20x4"
#define DEFAULT_TYPE		"lcd"
/* End of configfile defaults */

#define DEFAULT_LINEWRAP	1
#define DEFAULT_AUTOSCROLL	1
#define DEFAULT_CURSORBLINK	0

/* These are the keys for a (possibly) broken LK202-25...*/
#define KEY_UP    'I'
#define KEY_DOWN  'F'
#define KEY_LEFT  'K'
#define KEY_RIGHT 'A'
#define KEY_F1    'N'
/* TODO: add more if you've got any more ;) or correct the settings
 * the actual translation is done in MtxOrb_getkey()
 */

#endif

