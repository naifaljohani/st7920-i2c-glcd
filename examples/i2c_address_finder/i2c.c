#include "system.h"
#include "i2c.h"

static void i2c_wait(void)
{
    // wait until no transfer is in progress and START/STOP/RESTART/RCV/ACK are idle
    while ((SSP1STAT & 0x04) || (SSP1CON2 & 0x1F))
        ;
}

void i2c_init(void)
{
    // RC3 / RC4 digital inputs (MSSP drives them open-drain)
    ANSELCbits.ANSC3 = 0;
    ANSELCbits.ANSC4 = 0;
    TRISCbits.TRISC3 = 1;
    TRISCbits.TRISC4 = 1;

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
