
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <stdio.h>

#include "i2c.h"
#include "log.h"
#include "type.h"

int i2cbus3;

static const char* bus3name = "/dev/i2c-3";
static int PIC_i2c_address;
static FILE* updateFile;

static void printUsage()
{
	LOG("usage: picprogram updatefile\n\n");
}

static bool tryPicAddress(int address)
{
	LOG("trying PIC address 0x%x\n", address);

	uint8_t magicBuff[4];
	if(i2cRead(address, 1, &magicBuff[0], 2) != 2){
		return false;
	}
	if(i2cRead(address, 2, &magicBuff[2], 2) != 2){
		return false;
	}

	LOG("PIC magic: 0x%x 0x%x 0x%x 0x%x\n", 
		magicBuff[0], magicBuff[1], magicBuff[2], magicBuff[3]);

	return (magicBuff[0] == 0xEB && 
		magicBuff[1] == 0xBE &&
		magicBuff[2] == 0x15 &&
		magicBuff[3] == 0xA1);
}

#define BLOCK_SIZE 64

int readFileBlock(uint8_t* block)
{
	memset(block, 0, 64);
	return fread(block, sizeof(uint8_t), 64, updateFile);
}

bool enterBootloader()
{
	TRACE();
	uint8_t buff[2];
	buff[0] = 0x55;
	buff[1] = 0xAA;
	return i2cWrite(PIC_i2c_address, 0xA1, buff, 2) == 2;
}

bool setAddressPointer(int address)
{
	TRACE("address: 0x%x", address)
	uint8_t buff[2];
	buff[0] = (address & 0xff00) >> 8;
	buff[1] = address & 0xff;
	return i2cWrite(PIC_i2c_address, 0x1, buff, 2) == 2;
}

bool eraseFlash()
{
	TRACE()
	uint8_t buff[2];
	return i2cRead(PIC_i2c_address, 0x4, buff, 2) == 2;
}

bool uploadBlock(uint8_t* block)
{
	TRACE()
	return i2cWrite(PIC_i2c_address, 0x2, block, BLOCK_SIZE) == BLOCK_SIZE;
}

bool downloadBlock(uint8_t* block)
{
	TRACE()
	return i2cRead(PIC_i2c_address, 0x3, block, BLOCK_SIZE) == BLOCK_SIZE;
}

bool verifyBlock(uint8_t* a, uint8_t* b)
{
	for(int i=0;i<BLOCK_SIZE;i++) {
		if(a[i] != b[i]) {
			return false;
		}
	}
	return true;
}

bool writeFlash()
{
	TRACE()
	uint8_t buff[2];
	return i2cRead(PIC_i2c_address, 0x5, buff, 2) == 2;
}

bool exitBootloader()
{
	TRACE()
	uint8_t buff[2];
	return i2cRead(PIC_i2c_address, 0x6, buff, 2) == 2;
}

#define CMD(fun) if(!fun) {return -1;}

int main(int argv, char** argc)
{
	uint8_t block[BLOCK_SIZE];
	uint8_t rereadBlock[BLOCK_SIZE];

	LOG("Starting PIC Program\n");

	if(argv != 2){
		LOG("invalid args\n");
		printUsage();
		return -1;
	}

	
	LOG("using updatefile: %s\n", argc[1]);
	updateFile = fopen(argc[1], "r");
	if(NULL==updateFile){
		LOG("Could not open update file");
		return -1;
	}

	fseek(updateFile, 0L, SEEK_END);
	long sz = ftell(updateFile);
	const int numBlocks = (sz + (BLOCK_SIZE/2)) / BLOCK_SIZE;
	LOG("size: %ld bytes, %d blocks", sz, numBlocks);
	

	if((i2cbus3 = open(bus3name, O_RDWR)) < 0){
		LOG("could not open %s\n", bus3name);
		return -1;
	}

	PIC_i2c_address = 0x50;
	if(!tryPicAddress(PIC_i2c_address)) {
		PIC_i2c_address = 0x58;
		if(!tryPicAddress(PIC_i2c_address)) {
			LOG("could not find PIC");
			return -1;
		}
	}

	CMD(enterBootloader())
	CMD(setAddressPointer(0x3FFF))
	CMD(eraseFlash())
	
	CMD(setAddressPointer(0x200));
	for(int i=0;i<numBlocks;i++) {
		CMD(eraseFlash())
	}

	CMD(setAddressPointer(0x200));
	fseek(updateFile, 0, SEEK_SET);
	for(int i=0;i<numBlocks;i++){
		int bytesRead = readFileBlock(block);
		if(bytesRead == 0) {
			LOG("unexpected end of file\n");
			return -1;
		}
		CMD(uploadBlock(block))
		CMD(writeFlash())
	}

	CMD(setAddressPointer(0x200));
	fseek(updateFile, 0, SEEK_SET);
	for(int i=0;i<numBlocks;i++){
		int bytesRead = readFileBlock(block);
		if(bytesRead == 0) {
			LOG("unexpected end of file\n");
			return -1;
		}
		CMD(downloadBlock(rereadBlock))
		if(!verifyBlock(block, rereadBlock)){
			LOG("error verifying block: %d", i);
			return -1;
		}
	}


	memset(block, 0, BLOCK_SIZE);
	block[0] = 0x34;
	block[1] = 0x55;
	CMD(setAddressPointer(0x3FFF))
	CMD(uploadBlock(block))
	CMD(writeFlash())

	CMD(exitBootloader())

	LOG("flash success");
	return 0;
}