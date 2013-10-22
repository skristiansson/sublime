#ifndef _I2C_H_
#define _I2C_H_
#include <config.h>

#if (I2C_DRIVER == oci2c)
#define i2c_init	oci2c_init
#define i2c_write	oci2c_write
#define i2c_read	oci2c_read
#endif

extern void i2c_init(void);
extern void i2c_write(uint8_t address, uint8_t *buf, uint32_t len, int stop);
extern void i2c_read(uint8_t address, uint8_t *buf, uint32_t len, int stop);

#endif
