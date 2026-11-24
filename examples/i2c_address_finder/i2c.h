#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void i2c_init(void);                 // MSSP1 master, 400 kHz, SCL = RC3, SDA = RC4
void i2c_start(void);
void i2c_stop(void);
uint8_t i2c_write(uint8_t data);     // returns 0 on ACK, 1 on NACK

#endif
