
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>


#include "i2c.h"
#include "log.h"
#include "type.h"

int i2cbus3;

static const char* bus3name = "/dev/i2c-3";
static int PIC_i2c_address;
static FILE* updateFile;

static void printUsage()
{
	LOG("usage: picprogram phase updatefile\n\n");
	LOG("\tphase=1: put the PIC in bootloader mode\n");
	LOG("\tphase=2: reflash the PIC \n");
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
	return i2cRead(PIC_i2c_address, 0x3, block, BLOCK_SIZE) == BLOCK_SIZE;
}

bool verifyBlock(uint8_t* a, uint8_t* b)
{
	TRACE();
	for(int i=0;i<BLOCK_SIZE;i++) {
		LOG("i: %d a: 0x%x b: 0x%x\n", i, a[i], b[i]);
		if(a[i] != b[i]) {
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

#define CMD(fun) if(!fun) {return -1;}

#define MEMORY_SIZE 0x8000
uint8_t memory[MEMORY_SIZE];
bool parseHexFile(FILE* stream, uint8_t* memory, size_t memorySize);

int main(int argv, char** argc)
{
	uint8_t block[BLOCK_SIZE];

	LOG("Starting PIC Program\n");

	if(argv != 3){
		LOG("invalid args\n");
		printUsage();
		return -1;
	}

	const int phase = atoi(argc[1]);
	
	LOG("using updatefile: %s\n", argc[2]);
	updateFile = fopen(argc[2], "r");
	if(NULL==updateFile){
		LOG("Could not open update file");
		return -1;
	}

	if(!parseHexFile(updateFile, memory, MEMORY_SIZE)){
		LOG("error parsing update hex file");
		return -1;
	}

	if((i2cbus3 = open(bus3name, O_RDWR)) < 0){
		LOG("could not open %s\n", bus3name);
		return -1;
	}

	
	if(phase == 1) {
		PIC_i2c_address = 0x50;
		if(!tryPicAddress(PIC_i2c_address)) {
			PIC_i2c_address = 0x58;
			if(!tryPicAddress(PIC_i2c_address)) {
				LOG("could not find PIC");
				return -1;
			}
		}

		CMD(enterBootloader())
		return 0;
	} else {
		PIC_i2c_address = 0x50;
	}

	sleep(1);

	int address;
	CMD(getAddressPointer(&address))

	//erase the app valid flag. Do this just incase flash goes wrong
	//it will stay in the bootloader
	CMD(setAddressPointer(0x3FFF - 31))
	CMD(eraseFlash())
	
#define START_BOOTLOADER 0x200
#define END 0x4000

	CMD(setAddressPointer(START_BOOTLOADER));
	for(address=START_BOOTLOADER;address<END;address+=32){
		CMD(downloadBlock(block))

		if(!verifyBlock(block, &memory[address*2])){
			LOG("block: %d differs. Flashing...\n", (address-START_BOOTLOADER) / 32);
			CMD(setAddressPointer(address));
			CMD(eraseFlash())
			CMD(setAddressPointer(address));
			CMD(uploadBlock(&memory[address*2]))
			CMD(writeFlash())

		} else {
			LOG("block: %d same\n", (address-START_BOOTLOADER) / 32);
		}
	}


	CMD(exitBootloader())

	LOG("flash success");
	return 0;
}