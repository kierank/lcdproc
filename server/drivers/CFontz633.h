#ifndef CFONTZ633_H
#define CFONTZ633_H

#include "lcd.h"

extern lcd_logical_driver *CFontz633;

#define CFONTZ633_DEF_CELL_WIDTH 6
#define CFONTZ633_DEF_CELL_WIDTH 6
#define CFONTZ633_DEF_CELL_WIDTH 6
#define CFONTZ633_DEF_CELL_HEIGHT 8
#define CFONTZ633_DEF_CONTRAST 16
#define CFONTZ633_DEF_DEVICE "/dev/lcd"
#define CFONTZ633_DEF_SPEED B19200
#define CFONTZ633_DEF_BRIGHTNESS 100
#define CFONTZ633_DEF_OFFBRIGHTNESS 0
#define CFONTZ633_DEF_SIZE "16x2"

int  CFontz633_init (lcd_logical_driver * driver, char *device);
void CFontz633_close ();
void CFontz633_flush ();
void CFontz633_flush_box (int lft, int top, int rgt, int bot);
void CFontz633_chr (int x, int y, char c);
int  CFontz633_contrast (int contrast);
void CFontz633_backlight (int on);
void CFontz633_vbar (int x, int len);
void CFontz633_hbar (int x, int y, int len);
void CFontz633_init_num ();
void CFontz633_num (int x, int num);
void CFontz633_set_char (int n, char *dat);
void CFontz633_draw_frame(char *dat);
int  CFontz633_width ();
int  CFontz633_height ();
void CFontz633_clear ();
void CFontz633_string (int x, int y, char string[]);




#endif

