
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <stdio.h>

#include "i2c-dev.h"
#include "log.h"

extern int i2cbus3;


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int i2cRead(size_t address, size_t reg, void* buffer, size_t len)
{
	TRACE("address: 0x%x, reg: %d, len: %d\n", address, reg, len);
	unsigned int sleeptime = 50000;

	if(ioctl(i2cbus3, I2C_SLAVE, address) < 0){
		LOG("failed to acquire bus address: 0x%x\n", address);
		return -1;
	}

	size_t bytesRead = 0;
	char temp[1];
	temp[0] = reg;
	while((bytesRead = write(i2cbus3, temp, 1)) != 1){
		LOG("failed to write reg to i2c bus. error: %d\n", bytesRead);
		usleep(sleeptime);
		sleeptime += 50000;
		//return -1;
	}

	uint8_t* buf = (uint8_t*)buffer;
	int i = 0;
	bytesRead = 0;
	while(bytesRead < len){
		int err = read(i2cbus3, &buf[i], len-bytesRead);
		if(err < 0){
			LOG("error reading from i2c bus: %d\n", err);
			return -1;
		} else {
			bytesRead += err;
			i += err;
		}
	}

	return bytesRead;
}

int i2cWrite(size_t address, size_t reg, void* buffer, size_t len)
{
	TRACE("address: 0x%x, reg: %d, len: %d\n", address, reg, len);
	unsigned int sleeptime = 50000;

#define MAX_WRITE_SIZE 2048
	if(len > MAX_WRITE_SIZE) {
		LOG("request to write %d bytes is too big. max size is %d\n", len, MAX_WRITE_SIZE);
		return -1;
	}

	if(ioctl(i2cbus3, I2C_SLAVE, address) < 0){
		LOG("failed to acquire bus address: 0x%x\n", address);
		return -1;
	}

	uint8_t writeBuffer[MAX_WRITE_SIZE+1];
	writeBuffer[0] = reg;
	memcpy(&writeBuffer[1], buffer, MIN(len,MAX_WRITE_SIZE));

	size_t bytesWritten = 0;
	int i = 0;
	while(bytesWritten < len){
		int err = write(i2cbus3, &writeBuffer[i], (len-bytesWritten) + 1);
		if(err < 0){
			LOG("error writing to i2c bus: %d %s\n", err, strerror( errno ));
			usleep(sleeptime);
			sleeptime += 50000;
			//return -1;
		} else {
			bytesWritten += err;
			i += err;
		}
	}

	return bytesWritten;
}