# I2C address finder

If the display stays blank, use this small program to check whether the PIC can reach the I/O
expander at all, and to find out which address it uses. It doesn't need the display or the
library, only the I2C driver, and it shows the result by blinking an LED.

## What it does

When it starts, it tries every PCF8574 / PCA8574 address (0x20–0x27) and every PCF8574A
address (0x38–0x3F). It reports the first one that answers.

| LED (D5 / RA7) | Address | Chip |
|---|---|---|
| 1–8 blinks, a 2 s pause, then repeats | 0x20–0x27 | PCF8574 / PCA8574 |
| 9–16 blinks, a 2 s pause, then repeats | 0x38–0x3F | PCF8574A |
| Fast blinking (100 ms) | none found | Check power, GND, SDA/SCL, the pull-up resistors and the A0–A2 pins |

Expander pin **P0** also changes state once per cycle. If you connect an LED as
**VCC → resistor → LED → P0**, you can watch the expander's output working. The expander can
sink current but sources very little, so an LED wired from P0 to GND will stay dark.

Put the address you find into `GLCD_I2C_ADDR` in `st7920_glcd_config.h`.

## Using it

1. Create a new MPLAB X project for your PIC and add `main.c`, `i2c.c`, `i2c.h` and
   `system.h` from this folder.
2. Build and program the board, then count the blinks.

These files are written for the PIC16F1779 on the Curiosity HPC (SCL on RC3, SDA on RC4, LED on
RA7). On another board, change the `LED_*` macros in `main.c` and the pin setup in `i2c.c`.
