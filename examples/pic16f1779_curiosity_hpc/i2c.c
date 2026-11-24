#include "system.h"
#include "i2c.h"

static void i2c_wait(void)
{
    // wait until no transfer is in progress and START/STOP/RESTART/RCV/ACK are idle
    while ((SSP1STAT & 0x04) || (SSP1CON2 & 0x1F))
        ;
}

// I2C bus recovery. After a reset that is not a power-up (for example the
// MCLR reset at the end of programming) the expander may still be in the
// middle of a byte and hold SDA low, which blocks the bus until power is
// removed. Clock SCL until SDA is released, then send a STOP.
// The pins are driven open-drain style: TRIS = 1 releases the line (the
// pull-up makes it high), TRIS = 0 with LAT = 0 pulls it low.
static void i2c_bus_recover(void)
{
    SSP1CON1 = 0x00;                // MSSP off
    RC3PPS   = 0x00;                // PPS outputs survive an MCLR reset:
    RC4PPS   = 0x00;                // give RC3/RC4 back to LATC
    LATCbits.LATC3 = 0;
    LATCbits.LATC4 = 0;
    TRISCbits.TRISC3 = 1;           // SCL released
    TRISCbits.TRISC4 = 1;           // SDA released
    __delay_us(10);

    for (uint8_t i = 0; i < 9u && !PORTCbits.RC4; i++)
    {
        TRISCbits.TRISC3 = 0;       // SCL low
        __delay_us(10);
        TRISCbits.TRISC3 = 1;       // SCL high
        __delay_us(10);
    }

    // STOP: SDA goes low -> high while SCL is high
    TRISCbits.TRISC3 = 0;  __delay_us(10);
    TRISCbits.TRISC4 = 0;  __delay_us(10);
    TRISCbits.TRISC3 = 1;  __delay_us(10);
    TRISCbits.TRISC4 = 1;  __delay_us(10);
}

void i2c_init(void)
{
    // RC3 / RC4 digital inputs (MSSP drives them open-drain)
    ANSELCbits.ANSC3 = 0;
    ANSELCbits.ANSC4 = 0;
    TRISCbits.TRISC3 = 1;
    TRISCbits.TRISC4 = 1;

    i2c_bus_recover();              // free the bus if a reset hit mid-transfer

    // Peripheral Pin Select: route MSSP1 I2C to RC3 (SCL) and RC4 (SDA)
    SSPCLKPPS = 0x13;   // SCL input  <- RC3
    SSPDATPPS = 0x14;   // SDA input  <- RC4
    RC3PPS    = 0x21;   // RC3 output <- SCL
    RC4PPS    = 0x22;   // RC4 output <- SDA

    SSP1STAT = 0x00;    // slew rate control on (needed for 400 kHz)
    SSP1ADD  = (uint8_t)((_XTAL_FREQ / (4UL * 400000UL)) - 1);  // 9 -> 400 kHz
    SSP1CON2 = 0x00;
    SSP1CON3 = 0x00;
    SSP1CON1 = 0x28;    // SSPEN = 1, I2C master mode
}

void i2c_start(void)
{
    i2c_wait();
    SSP1CON2bits.SEN = 1;
    while (SSP1CON2bits.SEN)
        ;
}

void i2c_stop(void)
{
    i2c_wait();
    SSP1CON2bits.PEN = 1;
    while (SSP1CON2bits.PEN)
        ;
}

uint8_t i2c_write(uint8_t data)
{
    i2c_wait();
    PIR1bits.SSP1IF = 0;
    SSP1BUF = data;
    while (!PIR1bits.SSP1IF)    // set after the 9th clock (ACK bit)
        ;
    PIR1bits.SSP1IF = 0;
    return SSP1CON2bits.ACKSTAT;
}
