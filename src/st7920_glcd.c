/*
 * st7920_glcd - ST7920 128x64 graphic LCD driver over an I2C I/O expander
 *               (PCF8574 / PCF8574A / PCA8574), 4-bit parallel mode.
 *
 * Portable C99. All hardware access goes through st7920_glcd_config.h.
 * See README.md for wiring, configuration and porting.
 *
 * SPDX-License-Identifier: MIT
 */
#include "st7920_glcd.h"
#if GLCD_USE_GRAPHICS && GLCD_USE_FONT5X7
#include "st7920_font5x7.h"
#endif

/* ------------------------------------------------------------------ */
/* configuration checks                                                */
/* ------------------------------------------------------------------ */
#if !defined(GLCD_I2C_ADDR) || !defined(GLCD_PIN_RS) || !defined(GLCD_PIN_E) || \
    !defined(GLCD_PIN_D4) || !defined(GLCD_PIN_D5) || !defined(GLCD_PIN_D6) || !defined(GLCD_PIN_D7)
#error "st7920_glcd: missing settings in st7920_glcd_config.h"
#endif
#ifndef GLCD_PIN_RST
#define GLCD_PIN_RST -1
#endif
#ifndef GLCD_PIN_BL
#define GLCD_PIN_BL -1
#endif
#ifndef GLCD_BL_ACTIVE_HIGH
#define GLCD_BL_ACTIVE_HIGH 1
#endif

/* ------------------------------------------------------------------ */
/* expander bit masks                                                  */
/* ------------------------------------------------------------------ */
#define BIT(p)      ((uint8_t)(1u << (p)))
#define M_RS        BIT(GLCD_PIN_RS)
#define M_E         BIT(GLCD_PIN_E)
#if GLCD_PIN_RST >= 0
#define M_RST       BIT(GLCD_PIN_RST)
#else
#define M_RST       0u
#endif

/* nibble value -> expander data-pin bits, for any D4..D7 mapping */
#define NIB(n) (uint8_t)( ((((n) >> 0) & 1u) << GLCD_PIN_D4) | \
                          ((((n) >> 1) & 1u) << GLCD_PIN_D5) | \
                          ((((n) >> 2) & 1u) << GLCD_PIN_D6) | \
                          ((((n) >> 3) & 1u) << GLCD_PIN_D7) )
static const uint8_t nib_map[16] = {
    NIB(0),  NIB(1),  NIB(2),  NIB(3),  NIB(4),  NIB(5),  NIB(6),  NIB(7),
    NIB(8),  NIB(9),  NIB(10), NIB(11), NIB(12), NIB(13), NIB(14), NIB(15)
};

/* bits that are always present on the expander outputs (RST high, backlight) */
static uint8_t ctrl_bits = M_RST
#if GLCD_PIN_BL >= 0
#if GLCD_BL_ACTIVE_HIGH
    | BIT(GLCD_PIN_BL)
#endif
#endif
    ;

/* ------------------------------------------------------------------ */
/* low level bus                                                       */
/* ------------------------------------------------------------------ */
static void bus_begin(void)
{
    GLCD_I2C_START();
    GLCD_I2C_WRITE((uint8_t)(GLCD_I2C_ADDR << 1));
}

static void bus_end(void)
{
    GLCD_I2C_STOP();
}

static void expander_write(uint8_t data)
{
    bus_begin();
    GLCD_I2C_WRITE(data);
    bus_end();
}

/* One nibble with an E pulse, inside an open I2C transfer. The expander
   updates its outputs after every byte, so one transfer carries many pulses. */
static void bus_nibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (uint8_t)(nib_map[nibble & 0x0Fu] | rs | ctrl_bits);
    GLCD_I2C_WRITE((uint8_t)(data | M_E));      /* E high */
    GLCD_I2C_WRITE(data);                       /* E low -> ST7920 latches */
}

/* One byte = 4 I2C bytes (~90 us at 400 kHz, ~360 us at 100 kHz), longer
   than the 72 us the ST7920 needs per byte, so streamed data needs no delay. */
static void bus_byte(uint8_t value, uint8_t rs)
{
    bus_nibble((uint8_t)(value >> 4), rs);
    bus_nibble((uint8_t)(value & 0x0Fu), rs);
}

static void lcd_send(uint8_t value, uint8_t rs)
{
    bus_begin();
    bus_byte(value, rs);
    bus_end();
    GLCD_DELAY_US(80);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_send(cmd, 0);
}

/* ------------------------------------------------------------------ */
/* init and text mode                                                  */
/* ------------------------------------------------------------------ */
void glcd_init(void)
{
    GLCD_DELAY_MS(50);                          /* power-up */
#if GLCD_PIN_RST >= 0
    expander_write((uint8_t)(ctrl_bits & (uint8_t)(M_RST ^ 0xFFu)));   /* RST low */
    GLCD_DELAY_MS(10);
#endif
    expander_write(ctrl_bits);                  /* RST high */
    GLCD_DELAY_MS(50);                          /* > 40 ms after reset */

    bus_begin();
    bus_nibble(0x02, 0);                        /* switch to 4-bit interface */
    bus_end();
    GLCD_DELAY_US(100);
    lcd_cmd(0x20);                              /* function set: 4-bit, basic */
    GLCD_DELAY_US(100);
    lcd_cmd(0x20);                              /* repeated, as in the datasheet */
    GLCD_DELAY_US(100);
    lcd_cmd(0x0C);                              /* display on, cursor off */
    glcd_clear();
    lcd_cmd(0x06);                              /* entry mode: increment */
}

void glcd_clear(void)
{
    lcd_cmd(0x01);
    GLCD_DELAY_MS(2);                           /* clear takes 1.6 ms */
}

void glcd_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_addr[4] = { 0x80, 0x90, 0x88, 0x98 };
    uint8_t r = (uint8_t)(row & 0x03u);
    uint8_t c = (uint8_t)((col & 0x0Fu) >> 1);  /* 2 characters per address */
    lcd_cmd((uint8_t)(row_addr[r] + c));
}

void glcd_putc(char c)
{
    lcd_send((uint8_t)c, M_RS);
}

void glcd_puts(const char *s)
{
    while (*s)
        glcd_putc(*s++);
}

#if GLCD_PIN_BL >= 0
void glcd_backlight(uint8_t on)
{
#if GLCD_BL_ACTIVE_HIGH
    uint8_t lit = on;
#else
    uint8_t lit = (uint8_t)!on;
#endif
    if (lit)
        ctrl_bits = (uint8_t)(ctrl_bits | BIT(GLCD_PIN_BL));
    else
        ctrl_bits = (uint8_t)(ctrl_bits & (uint8_t)(BIT(GLCD_PIN_BL) ^ 0xFFu));
    expander_write(ctrl_bits);
}
#endif

/* ------------------------------------------------------------------ */
/* graphics mode                                                       */
/* ------------------------------------------------------------------ */
#if GLCD_USE_GRAPHICS

#define FB_STRIDE   (GLCD_WIDTH / 8u)           /* 16 bytes per row */

static uint8_t fb[FB_STRIDE * GLCD_HEIGHT];     /* MSB = leftmost pixel */

/* per row: first/last changed 16-px word since last update, 0xFF = clean */
static uint8_t dlo[GLCD_HEIGHT];
static uint8_t dhi[GLCD_HEIGHT];

static uint8_t draw_color = GLCD_COLOR_ON;

static void mark_all_dirty(void)
{
    for (uint8_t y = 0; y < GLCD_HEIGHT; y++)
    {
        dlo[y] = 0;
        dhi[y] = 7;
    }
}

void glcd_gfx_begin(void)
{
    glcd_clear();                               /* text layer overlays graphics */
    lcd_cmd(0x24);                              /* extended instruction set */
    lcd_cmd(0x26);                              /* graphic display on */
    mark_all_dirty();                           /* GDRAM is random after power-up */
}

void glcd_gfx_end(void)
{
    lcd_cmd(0x24);                              /* graphic display off */
    lcd_cmd(0x20);                              /* basic instruction set */
    glcd_clear();
}

void glcd_fb_clear(void)
{
    for (uint16_t i = 0; i < sizeof(fb); i++)
        fb[i] = 0;
    mark_all_dirty();
}

void glcd_color(uint8_t color)
{
    draw_color = color;
}

void glcd_pixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= GLCD_WIDTH || y >= GLCD_HEIGHT)
        return;
    uint16_t idx = (uint16_t)((uint16_t)y * FB_STRIDE + (uint8_t)(x >> 3));
    uint8_t mask = (uint8_t)(0x80u >> (x & 7u));
    uint8_t old = fb[idx];
    uint8_t val;
    if (color == GLCD_COLOR_INVERT)
        val = (uint8_t)(old ^ mask);
    else if (color)
        val = (uint8_t)(old | mask);
    else
        val = (uint8_t)(old & (uint8_t)(mask ^ 0xFFu));
    if (val == old)
        return;                                 /* unchanged: nothing to send */
    fb[idx] = val;

    uint8_t w = (uint8_t)(x >> 4);              /* 16-px word 0..7 */
    if (dlo[y] == 0xFF)
    {
        dlo[y] = w;
        dhi[y] = w;
    }
    else
    {
        if (w < dlo[y]) dlo[y] = w;
        if (w > dhi[y]) dhi[y] = w;
    }
}

uint8_t glcd_get_pixel(uint8_t x, uint8_t y)
{
    if (x >= GLCD_WIDTH || y >= GLCD_HEIGHT)
        return 0;
    uint16_t idx = (uint16_t)((uint16_t)y * FB_STRIDE + (uint8_t)(x >> 3));
    return (uint8_t)((fb[idx] & (uint8_t)(0x80u >> (x & 7u))) ? 1u : 0u);
}

/* signed, clipped plot in the current colour */
static void plot(int16_t x, int16_t y)
{
    if (x < 0 || y < 0 || x >= GLCD_WIDTH || y >= GLCD_HEIGHT)
        return;
    glcd_pixel((uint8_t)x, (uint8_t)y, draw_color);
}

static void span(int16_t x0, int16_t x1, int16_t y)
{
    for (int16_t x = x0; x <= x1; x++)
        plot(x, y);
}

void glcd_hline(uint8_t x, uint8_t y, uint8_t w)
{
    if (w)
        span(x, (int16_t)x + w - 1, y);
}

void glcd_vline(uint8_t x, uint8_t y, uint8_t h)
{
    for (uint8_t j = 0; j < h; j++)
        plot(x, (int16_t)y + j);
}

void glcd_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    int16_t x = x0, y = y0;                     /* Bresenham */
    int16_t dx = (int16_t)x1 - x0;
    int16_t dy = (int16_t)y1 - y0;
    int8_t sx = dx < 0 ? -1 : 1;
    int8_t sy = dy < 0 ? -1 : 1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    int16_t err = dx - dy;

    while (1)
    {
        plot(x, y);
        if (x == x1 && y == y1)
            break;
        int16_t e2 = err * 2;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }
    }
}

void glcd_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    if (w == 0 || h == 0)
        return;
    glcd_hline(x, y, w);
    if (h > 1)
        glcd_hline(x, (uint8_t)(y + h - 1), w);
    if (h > 2)
    {
        glcd_vline(x, (uint8_t)(y + 1), (uint8_t)(h - 2));
        if (w > 1)
            glcd_vline((uint8_t)(x + w - 1), (uint8_t)(y + 1), (uint8_t)(h - 2));
    }
}

void glcd_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t j = 0; j < h; j++)
        glcd_hline(x, (uint8_t)(y + j), w);
}

void glcd_circle(uint8_t cx, uint8_t cy, uint8_t r)
{
    int16_t x = r, y = 0;                       /* midpoint circle */
    int16_t err = 1 - x;

    /* every pixel is plotted exactly once, so GLCD_COLOR_INVERT works */
    while (x >= y)
    {
        if (y == 0)
        {
            plot(cx + x, cy);
            plot(cx - x, cy);
            plot(cx, cy + x);
            plot(cx, cy - x);
        }
        else if (x == y)
        {
            plot(cx + x, cy + y);
            plot(cx - x, cy + y);
            plot(cx + x, cy - y);
            plot(cx - x, cy - y);
        }
        else
        {
            plot(cx + x, cy + y);
            plot(cx - x, cy + y);
            plot(cx + x, cy - y);
            plot(cx - x, cy - y);
            plot(cx + y, cy + x);
            plot(cx - y, cy + x);
            plot(cx + y, cy - x);
            plot(cx - y, cy - x);
        }
        y++;
        if (err < 0)
            err += 2 * y + 1;
        else
        {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void glcd_fill_circle(uint8_t cx, uint8_t cy, uint8_t r)
{
    /* one span per row, so GLCD_COLOR_INVERT does not toggle a pixel twice */
    if (r > 127)
        r = 127;                                /* keeps the maths inside int16 */
    int16_t rr = (int16_t)r * r + r;            /* r^2 + r gives a rounder shape */
    int16_t x = r;
    for (int16_t dy = 0; dy <= r; dy++)
    {
        while (x > 0 && x * x + dy * dy > rr)
            x--;
        span(cx - x, cx + x, cy + dy);
        if (dy)
            span(cx - x, cx + x, cy - dy);
    }
}

void glcd_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bmp)
{
    uint8_t stride = (uint8_t)((w + 7u) >> 3);
    for (uint8_t j = 0; j < h; j++)
    {
        const uint8_t *row = bmp + (uint16_t)j * stride;
        for (uint8_t i = 0; i < w; i++)
        {
            if (row[i >> 3] & (uint8_t)(0x80u >> (i & 7u)))
                plot((int16_t)x + i, (int16_t)y + j);
        }
    }
}

#if GLCD_USE_FONT5X7
void glcd_char(uint8_t x, uint8_t y, char c)
{
    if (c < 0x20 || c > 0x7E)
        c = '?';
    const uint8_t *glyph = glcd_font5x7[(uint8_t)(c - 0x20)];
    for (uint8_t col = 0; col < 5; col++)
    {
        uint8_t bits = glyph[col];
        for (uint8_t row = 0; row < 7; row++)
        {
            if (bits & (uint8_t)(1u << row))
                plot((int16_t)x + col, (int16_t)y + row);
        }
    }
}

void glcd_text(uint8_t x, uint8_t y, const char *s)
{
    while (*s)
    {
        glcd_char(x, y, *s++);
        x = (uint8_t)(x + 6u);
    }
}
#endif /* GLCD_USE_FONT5X7 */

void glcd_update(void)
{
    /* ST7920 GDRAM is 32 rows x 256 px: screen rows 32..63 are the right
       half of GDRAM rows 0..31 (horizontal address + 8). */
    for (uint8_t y = 0; y < GLCD_HEIGHT; y++)
    {
        uint8_t lo = dlo[y];
        if (lo == 0xFF)
            continue;                           /* row unchanged */
        uint8_t hi = dhi[y];
        dlo[y] = 0xFF;

        uint8_t haddr = (uint8_t)(((y < 32u) ? 0u : 8u) + lo);
        lcd_cmd((uint8_t)(0x80u | (y & 0x1Fu)));    /* vertical address */
        lcd_cmd((uint8_t)(0x80u | haddr));          /* horizontal address */

        const uint8_t *p = &fb[(uint16_t)y * FB_STRIDE + (uint8_t)(lo << 1)];
        uint8_t n = (uint8_t)(hi - lo);
        n = (uint8_t)((uint8_t)(n + 1u) << 1);      /* 2 bytes per word */
        bus_begin();
        while (n--)
            bus_byte(*p++, M_RS);
        bus_end();
    }
}

void glcd_update_all(void)
{
    mark_all_dirty();
    glcd_update();
}

#endif /* GLCD_USE_GRAPHICS */
