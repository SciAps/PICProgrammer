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


int PIC_i2c_address;
int i2cbus3;
static const char* bus3name = "/dev/i2c-3";

#define LINE_SIZE 16
static void formatBinary(unsigned char* data, char* dataout, size_t size)
{
    int pos = 0;
    for(size_t i=0;i<LINE_SIZE;i++){
        if(i >= size){
            pos += sprintf(&dataout[pos], "     ");
        } else {
            pos += sprintf(&dataout[pos], "0x%02x ", data[i]);
        }
    }
    pos += sprintf(&dataout[pos], "| ");
    for(size_t i=0;i<LINE_SIZE;i++){
        char b;
        if(i >= size) {
            pos += sprintf(&dataout[pos], " ");
        } else {
            b = data[i] >= 32 && data[i] <= 126 ? data[i] : '?';
            pos += sprintf(&dataout[pos], "%c", b);
        }
    }
}

int main(int argv, char** argc)
{

	uint8_t block[2*BLOCK_SIZE];

	if((i2cbus3 = open(bus3name, O_RDWR)) < 0){
		LOG("could not open %s\n", bus3name);
		return -1;
	}

	PIC_i2c_address = 0x50;

#define START_BOOTLOADER 0x200
#define END 0x4000


	int address = START_BOOTLOADER;
	int readAddress;
	//CMD(setAddressPointer(START_BOOTLOADER));
	for(address=START_BOOTLOADER;address<END;address+=32){

		CMD(setAddressPointer(address))
		CMD(downloadBlock(block))

		LOG("address: 0x%04x\n", address);
		char buffer[1024];
		for(int i=0;i<BLOCK_SIZE;i+=LINE_SIZE){
			formatBinary(&block[i], buffer, BLOCK_SIZE-i);
			LOG("%s\n", buffer);
		}		

		//CMD(getAddressPointer(&readAddress))
		//LOG("read address: 0x%04x\n", readAddress);
	}

}
