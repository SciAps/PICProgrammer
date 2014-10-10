
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

int i2cbus3;

static const char* bus3name = "/dev/i2c-3";
int PIC_i2c_address;
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

int readFileBlock(uint8_t* block)
{
	memset(block, 0, 64);
	return fread(block, sizeof(uint8_t), 64, updateFile);
}



#define MEMORY_SIZE 0x18000
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
#define END 0x3F80
//#define END 0x4000

	for(address=START_BOOTLOADER;address<END;address+=32){
		CMD(setAddressPointer(address));
		CMD(downloadBlock(block))

		if(!verifyBlock(block, &memory[address*2])){
			LOG("block: %d differs. Flashing...\n", (address-START_BOOTLOADER) / 32);
			CMD(setAddressPointer(address));
			CMD(eraseFlash())

			CMD(setAddressPointer(address));
			CMD(uploadBlock(&memory[address*2]))
			CMD(writeFlash())

			CMD(setAddressPointer(address));
			CMD(downloadBlock(block))
			CMD(verifyBlock(block, &memory[address*2]))

		} else {
			LOG("block: %d same\n", (address-START_BOOTLOADER) / 32);
		}
		
		sleep(1);
	}


	CMD(exitBootloader())

	LOG("flash success");
	return 0;
}