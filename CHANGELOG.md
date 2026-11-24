# Changelog

## Unreleased
- New example: `examples/i2c_address_finder` probes every PCF8574 and PCF8574A address
  and blinks the one it finds on an LED, so no display is needed.
- README: an address table for each chip and A0–A2 setting, and demo GIF/video.

## 1.0.0 - 2026-11-22
- First release.
- Text mode using the ST7920 built-in font.
- Graphics mode: frame buffer, partial updates, pixels, lines, rectangles, circles,
  filled shapes, bitmaps, 5x7 font, and on/off/invert colours.
- Configurable expander pin mapping, with optional reset and backlight pins.
- Example: PIC16F1779 on the Curiosity HPC, MSSP I2C at 400 kHz.
