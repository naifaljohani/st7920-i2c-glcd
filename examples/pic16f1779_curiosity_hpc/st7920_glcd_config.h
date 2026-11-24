/*
 * st7920_glcd_config.h - per-project configuration for the st7920_glcd library
 *
 * Copy this file into your project folder and edit it.
 */
#ifndef ST7920_GLCD_CONFIG_H
#define ST7920_GLCD_CONFIG_H

/* ------------------------------------------------------------------ */
/* 1) I2C address of the expander (7-bit)                              */
/*    PCF8574 / PCA8574 : 0x20..0x27 (most boards 0x27)                */
/*    PCF8574A          : 0x38..0x3F (most boards 0x3F)                */
/*    Not sure? Run examples/i2c_address_finder                        */
/* ------------------------------------------------------------------ */
#define GLCD_I2C_ADDR   0x27

/* ------------------------------------------------------------------ */
/* 2) Expander pin mapping: which Px pin drives each ST7920 signal     */
/*    Use -1 for a signal that is not connected.                       */
/*    ST7920 R/W must be tied to GND, PSB tied to VCC (parallel mode). */
/* ------------------------------------------------------------------ */
#define GLCD_PIN_RS     0
#define GLCD_PIN_E      1
#define GLCD_PIN_RST    2       /* -1 if RST is tied high / RC circuit */
#define GLCD_PIN_D4     3
#define GLCD_PIN_D5     4
#define GLCD_PIN_D6     5
#define GLCD_PIN_D7     6
#define GLCD_PIN_BL     -1      /* backlight control pin, -1 if none */
#define GLCD_BL_ACTIVE_HIGH 1   /* 1: pin high = backlight on */

/* ------------------------------------------------------------------ */
/* 3) Features                                                         */
/*    Graphics needs ~1.2 KB RAM (1 KB frame buffer + 128 B tracking). */
/*    Set to 0 on small MCUs to use text mode only.                    */
/* ------------------------------------------------------------------ */
#define GLCD_USE_GRAPHICS   1
#define GLCD_USE_FONT5X7    1   /* 5x7 font for glcd_text() (~480 B flash) */

/* ------------------------------------------------------------------ */
/* 4) Port: connect the library to your I2C driver and delay functions */
/*    GLCD_I2C_START()  : send START condition                         */
/*    GLCD_I2C_WRITE(b) : send one byte (ACK result is ignored)        */
/*    GLCD_I2C_STOP()   : send STOP condition                          */
/*    GLCD_DELAY_US(n) / GLCD_DELAY_MS(n) : busy-wait delays. The      */
/*    library only passes compile-time constants, so XC8's             */
/*    __delay_us() / __delay_ms() can be used directly.                */
/* ------------------------------------------------------------------ */
#include "system.h"     /* must define _XTAL_FREQ for XC8 delays */
#include "i2c.h"

#define GLCD_I2C_START()    i2c_start()
#define GLCD_I2C_WRITE(b)   i2c_write(b)
#define GLCD_I2C_STOP()     i2c_stop()
#define GLCD_DELAY_US(n)    __delay_us(n)
#define GLCD_DELAY_MS(n)    __delay_ms(n)

#endif /* ST7920_GLCD_CONFIG_H */
