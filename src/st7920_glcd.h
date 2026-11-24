/*
 * st7920_glcd - ST7920 128x64 graphic LCD driver over an I2C I/O expander
 *               (PCF8574 / PCF8574A / PCA8574), 4-bit parallel mode.
 *
 * Portable C99, no MCU-specific code. All hardware access goes through the
 * macros in st7920_glcd_config.h (I2C start/write/stop and delays).
 *
 * https://opensource.org/licenses/MIT  -  see LICENSE
 */
#ifndef ST7920_GLCD_H
#define ST7920_GLCD_H

#include <stdint.h>
#include "st7920_glcd_config.h"

#define ST7920_GLCD_VERSION "1.0.2"

#define GLCD_WIDTH   128
#define GLCD_HEIGHT  64

/* draw colours for glcd_color() */
#define GLCD_COLOR_OFF     0    /* clear pixels  */
#define GLCD_COLOR_ON      1    /* set pixels    */
#define GLCD_COLOR_INVERT  2    /* toggle pixels */

/* ---------------------------------------------------------------- */
/* Init and text mode (built-in ST7920 font: 4 lines x 16 chars)    */
/* ---------------------------------------------------------------- */
void glcd_init(void);                            /* call after your I2C is initialised */
void glcd_clear(void);                           /* clear the text layer */
void glcd_set_cursor(uint8_t col, uint8_t row);  /* col 0..15 (rounded to even), row 0..3 */
void glcd_putc(char c);
void glcd_puts(const char *s);

#if GLCD_PIN_BL >= 0
void glcd_backlight(uint8_t on);
#endif

/* ---------------------------------------------------------------- */
/* Graphics mode (1 KB frame buffer in MCU RAM)                     */
/*   Draw into the buffer, then call glcd_update(). Only the parts  */
/*   that changed since the last update are sent to the display.    */
/* ---------------------------------------------------------------- */
#if GLCD_USE_GRAPHICS
void glcd_gfx_begin(void);                       /* switch display to graphics mode */
void glcd_gfx_end(void);                         /* back to text mode */
void glcd_fb_clear(void);                        /* clear the whole frame buffer */
void glcd_update(void);                          /* send changed areas to the display */
void glcd_update_all(void);                      /* force a full-screen refresh */

void glcd_color(uint8_t color);                  /* GLCD_COLOR_ON / _OFF / _INVERT */
void glcd_pixel(uint8_t x, uint8_t y, uint8_t color);
uint8_t glcd_get_pixel(uint8_t x, uint8_t y);
void glcd_hline(uint8_t x, uint8_t y, uint8_t w);
void glcd_vline(uint8_t x, uint8_t y, uint8_t h);
void glcd_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void glcd_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void glcd_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void glcd_circle(uint8_t cx, uint8_t cy, uint8_t r);
void glcd_fill_circle(uint8_t cx, uint8_t cy, uint8_t r);

/* Bitmap: w x h pixels, rows top to bottom, each row (w+7)/8 bytes, MSB = leftmost.
   Set bits are drawn in the current colour, clear bits are skipped (transparent). */
void glcd_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bmp);

#if GLCD_USE_FONT5X7
void glcd_char(uint8_t x, uint8_t y, char c);   /* 5x7 font, cell 6 x 8 px */
void glcd_text(uint8_t x, uint8_t y, const char *s);
#endif
#endif /* GLCD_USE_GRAPHICS */

#endif /* ST7920_GLCD_H */
