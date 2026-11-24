/*
 * i2c_address_finder - find the I2C address of a PCF8574 / PCF8574A /
 * PCA8574 expander and show it by blinking an LED. No display needed.
 *
 * Board : Microchip Curiosity HPC (DM164136), PIC16F1779
 * I2C   : MSSP1, SCL = RC3, SDA = RC4, 400 kHz
 * LED   : D5 on RA7 (change the LED_* macros for another pin)
 *
 * At start-up every PCF8574 (0x20..0x27) and PCF8574A (0x38..0x3F)
 * address is probed. Then:
 *
 *   expander found : the LED blinks N times, pauses 2 s, repeats.
 *                      N = 1..8   -> 0x20..0x27 (PCF8574 / PCA8574)
 *                      N = 9..16  -> 0x38..0x3F (PCF8574A)
 *                    Expander pin P0 also toggles once per cycle, so an
 *                    LED on P0 (VCC - resistor - LED - P0) blinks too.
 *   not found      : the LED blinks fast (100 ms). Check power, GND,
 *                    SDA/SCL, pull-ups and the A0..A2 pins.
 *
 * Put the address you find into GLCD_I2C_ADDR in st7920_glcd_config.h.
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

// Status LED (Curiosity HPC D5). RA7 has no analog function.
#define LED_INIT()      do { TRISAbits.TRISA7 = 0; LATAbits.LATA7 = 0; } while (0)
#define LED_SET(on)     (LATAbits.LATA7 = (on))
#define LED_TOGGLE()    (LATAbits.LATA7 ^= 1)

// Returns 1 if a device ACKs at this 7-bit address
static uint8_t i2c_probe(uint8_t addr)
{
    i2c_start();
    uint8_t nack = i2c_write((uint8_t)(addr << 1));
    i2c_stop();
    return (uint8_t)(nack == 0);
}

// Search both PCF8574 and PCF8574A address ranges, 0 = nothing found
static uint8_t find_expander(void)
{
    for (uint8_t a = 0x20; a <= 0x27; a++)
        if (i2c_probe(a)) return a;
    for (uint8_t a = 0x38; a <= 0x3F; a++)
        if (i2c_probe(a)) return a;
    return 0;
}

// Write all 8 expander pins
static void expander_write(uint8_t addr, uint8_t value)
{
    i2c_start();
    if (i2c_write((uint8_t)(addr << 1)) == 0)
        i2c_write(value);
    i2c_stop();
}

void main(void)
{
    OSCCON = 0x7A;              // IRCF = 1111 (16 MHz HF), SCS = 1x (internal osc)

    LED_INIT();
    i2c_init();                 // SCL = RC3, SDA = RC4
    __delay_ms(50);             // let the expander power up

    uint8_t addr = find_expander();

    if (addr == 0)
    {
        while (1)               // not found: fast blink
        {
            LED_TOGGLE();
            __delay_ms(100);
        }
    }

    // 0x20..0x27 -> 1..8 blinks, 0x38..0x3F -> 9..16 blinks
    uint8_t blinks = addr;
    blinks -= (addr < 0x38u) ? 0x1Fu : 0x2Fu;

    uint8_t p0 = 0;
    while (1)
    {
        p0 ^= 1;
        // P0 = p0, P1..P7 = 0 (with the default wiring E stays low,
        // so a connected ST7920 ignores the bus)
        expander_write(addr, p0);

        for (uint8_t i = 0; i < blinks; i++)
        {
            LED_SET(1);
            __delay_ms(300);
            LED_SET(0);
            __delay_ms(300);
        }
        __delay_ms(2000);       // pause, then repeat the count
    }
}
