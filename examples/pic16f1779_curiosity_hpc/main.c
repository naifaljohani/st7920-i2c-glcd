/*
 * st7920_glcd example - animation demo on ST7920 GLCD (via I2C expander)
 * Board : Microchip Curiosity HPC (DM164136), PIC16F1779
 * I2C   : MSSP1, SCL = RC3, SDA = RC4, 400 kHz
 *
 *   left : radar sweep (rotating line in a circle)
 *   right: bouncing ball in a box
 *   top  : frame counter
 * LED on RA7 toggles every frame.
 * Device: PIC16F1779, XC8, internal oscillator 16 MHz
 */

// CONFIG1
#pragma config FOSC = INTOSC    // Internal oscillator, I/O on CLKIN pin
#pragma config WDTE = OFF       // Watchdog timer disabled
#pragma config PWRTE = ON       // Power-up timer enabled
#pragma config MCLRE = ON       // MCLR pin is MCLR
#pragma config CP = OFF         // Code protection off
#pragma config BOREN = ON       // Brown-out reset enabled
#pragma config CLKOUTEN = OFF   // CLKOUT disabled
#pragma config IESO = OFF
#pragma config FCMEN = OFF

// CONFIG2
#pragma config WRT = OFF
#pragma config PPS1WAY = OFF
#pragma config ZCD = OFF
#pragma config PLLEN = OFF
#pragma config STVREN = ON
#pragma config BORV = LO
#pragma config LPBOR = OFF
#pragma config LVP = ON         // Low-voltage programming (needed by on-board programmers)

#include "system.h"
#include "i2c.h"
#include "st7920_glcd.h"

// sin(i * 360/32 degrees) * 127, i = 0..31
static const int8_t sin32[32] = {
       0,   25,   49,   71,   90,  106,  117,  125,
     127,  125,  117,  106,   90,   71,   49,   25,
       0,  -25,  -49,  -71,  -90, -106, -117, -125,
    -127, -125, -117, -106,  -90,  -71,  -49,  -25
};

// radar
#define RADAR_X     22
#define RADAR_Y     38
#define RADAR_R     19

// ball box (outline), ball must stay inside
#define BOX_X       46
#define BOX_Y       14
#define BOX_W       81
#define BOX_H       49
#define BALL_R      4

static void radar_hand(uint8_t angle, uint8_t on)
{
    int16_t dx = ((int16_t)(RADAR_R - 2) * sin32[(uint8_t)((angle + 8u) & 31u)]) / 128;  // cos
    int16_t dy = ((int16_t)(RADAR_R - 2) * sin32[angle & 31u]) / 128;                    // sin
    glcd_color(on);
    glcd_line(RADAR_X, RADAR_Y, (uint8_t)(RADAR_X + dx), (uint8_t)(RADAR_Y + dy));
}

static void u16_to_str(uint16_t v, char *buf)       // 5 digits, leading zeros
{
    for (int8_t i = 4; i >= 0; i--)
    {
        buf[i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    buf[5] = '\0';
}

void main(void)
{
    OSCCON = 0x7A;              // IRCF = 1111 (16 MHz HF), SCS = 1x (internal osc)

    TRISAbits.TRISA7 = 0;       // RA7 output (RA7 has no analog function)
    LATAbits.LATA7 = 0;

    i2c_init();                 // SCL = RC3, SDA = RC4
    glcd_init();
    glcd_gfx_begin();

    // ---- static parts of the screen ----
    glcd_fb_clear();
    glcd_color(1);
    glcd_rect(0, 0, 128, 64);                   // screen border
    glcd_text(4, 3, "Animation");
    glcd_line(0, 12, 127, 12);
    glcd_circle(RADAR_X, RADAR_Y, RADAR_R);
    glcd_rect(BOX_X, BOX_Y, BOX_W, BOX_H);
    glcd_update();

    // ---- animation state ----
    uint8_t angle = 0;
    int8_t  bx = BOX_X + 20, by = BOX_Y + 15;   // ball centre
    int8_t  vx = 2, vy = 1;                     // ball speed (pixels per frame)
    uint16_t frame = 0;
    char num[6];

    while (1)
    {
        // 1) erase the old moving objects
        radar_hand(angle, 0);
        glcd_color(0);
        glcd_fill_circle((uint8_t)bx, (uint8_t)by, BALL_R);
        glcd_fill_rect(92, 3, 30, 7);           // old counter text

        // 2) move
        angle = (uint8_t)((angle + 1u) & 31u);

        bx += vx;
        by += vy;
        if (bx - BALL_R <= BOX_X + 1 || bx + BALL_R >= BOX_X + BOX_W - 2) { vx = -vx; bx += vx; }
        if (by - BALL_R <= BOX_Y + 1 || by + BALL_R >= BOX_Y + BOX_H - 2) { vy = -vy; by += vy; }

        frame++;

        // 3) draw them in the new position
        radar_hand(angle, 1);
        glcd_color(1);
        glcd_fill_circle((uint8_t)bx, (uint8_t)by, BALL_R);
        u16_to_str(frame, num);
        glcd_text(92, 3, num);

        // 4) send only what changed
        glcd_update();

        LATAbits.LATA7 ^= 1;    // LED toggles once per frame
    }
}
