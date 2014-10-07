
#ifndef _I2C_H_
#define _I2C_H_


int i2cRead(size_t address, size_t reg, void* buffer, size_t len);
int i2cWrite(size_t address, size_t reg, void* buffer, size_t len);

#endif /* _I2C_H_ */
