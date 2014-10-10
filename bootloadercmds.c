#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>


#include "i2c.h"
#include "log.h"
#include "type.h"
#include "bootloadercmds.h"

extern int PIC_i2c_address;

bool enterBootloader()
{
	TRACE();
	uint8_t buff[2];
	buff[0] = 0x55;
	buff[1] = 0xAA;
	return i2cWrite(PIC_i2c_address, 0xA1, buff, 2) == 3;
}

bool setAddressPointer(int address)
{
	TRACE("address: 0x%x\n", address);
	uint8_t buff[2];
	buff[0] = (address & 0xff00) >> 8;
	buff[1] = address & 0xff;
	return i2cWrite(PIC_i2c_address, 0x1, buff, 2) == 3;
}

bool getAddressPointer(int* address)
{
	TRACE();
	uint8_t buff[2];
	bool retval = i2cRead(PIC_i2c_address, 0x1, buff, 2) == 2;
	address[0] = (buff[0] << 8);
	address[0] |= buff[1] & 0xff;
	return retval;
}

bool eraseFlash()
{
	TRACE();
	uint8_t buff[1];
	bool retval = i2cRead(PIC_i2c_address, 0x4, buff, 1) == 1;
	retval &= buff[0] == 0;
	return retval;
}

bool uploadBlock(uint8_t* block)
{
	TRACE();
	return i2cWrite(PIC_i2c_address, 0x2, block, BLOCK_SIZE) == BLOCK_SIZE+1;
}

bool downloadBlock(uint8_t* block)
{
	TRACE();
	memset(block, 0, BLOCK_SIZE);
	return i2cRead(PIC_i2c_address, 0x3, block, BLOCK_SIZE) == BLOCK_SIZE;
}

bool verifyBlock(uint8_t* a, uint8_t* b)
{
	TRACE();

/*
	#ifdef DEBUG
	for(int i=0;i<BLOCK_SIZE;i++) {
		LOG("\ni: %d a: 0x%02x b: 0x%02x", i, a[i], b[i] );
	}
	#endif


	for(int i=0;i<BLOCK_SIZE;i++) {
		if( a[i] != b[i] ) {
			return false;
		}
	}
*/

	#ifdef DEBUG
	for(int i=0;i<BLOCK_SIZE;i+=2) {
		LOG("\ni: %d a: 0x%04x b: 0x%04x", i, 0x3fff & (a[i] << 8 | a[i+1]), 0x3fff & (b[i] << 8 | b[i+1]) );
	}
	#endif


	for(int i=0;i<BLOCK_SIZE;i+=2) {
		if( (0x3fff & (a[i] << 8 | a[i+1])) != (0x3fff & (b[i] << 8 | b[i+1])) ) {
			return false;
		}
	}


	return true;
}

bool writeFlash()
{
	TRACE();
	uint8_t buff[1];
	return i2cRead(PIC_i2c_address, 0x5, buff, 1) == 1;
}

bool exitBootloader()
{
	TRACE();
	uint8_t buff[1];
	return i2cRead(PIC_i2c_address, 0x6, buff, 1) == 1;
}

