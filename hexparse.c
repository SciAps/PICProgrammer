
#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "log.h"

ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
    char *ptr;

    ptr = fgetln(stream, n);

    if (ptr == NULL) {
        return -1;
    }

    /* Free the original ptr */
    if (*lineptr != NULL) free(*lineptr);

    /* Add one more space for '\0' */
    size_t len = n[0] + 1;

    /* Update the length */
    n[0] = len;

    /* Allocate a new buffer */
    *lineptr = malloc(len);

    /* Copy over the string */
    memcpy(*lineptr, ptr, len-1);

    /* Write the NULL character */
    (*lineptr)[len-1] = '\0';

    /* Return the length of the new buffer */
    return len;
}

#define TYPE_DATA 0
#define TYPE_EXTEND 4

bool parseHexFile(FILE* stream, uint8_t* memory, size_t memorySize)
{
    char * line = NULL;
    size_t len = 0;
    int read;
    uint32_t extendedAddress = 0;


    for(size_t i=0;i<memorySize;i+=2){
    	memory[i] = 0xff;
    	memory[i+1] = 0x3f;
    }

    memset(memory, 0xff, memorySize);

    while ((read = getline(&line, &len, stream)) != -1) {

        int recordLen;
        uint32_t address;
        int fieldType;
        u_int8_t checksum = 0;
        int value;

        if(sscanf(line, ":%02x%04x%02x", &recordLen, &address, &fieldType) != 3){
            return false;
        }

        address = extendedAddress + address;

        for(int i=0;i<4;i++){
            sscanf(&line[1+2*i], "%02x", &value);
            checksum += value;
        }

        if(fieldType == TYPE_DATA){
            for(int i=0;i<recordLen;i+=2){
            	int lowbyte, highbyte;
                if(!sscanf(&line[9+2*i], "%02x%02x", &lowbyte, &highbyte)) {
                    return false;
                }
                checksum += lowbyte;
                checksum += highbyte;

                if(address+i+1 >= memorySize) {
                	LOG("error: address 0x%x is out of bounds", address+i);
                	return false;
                } else {
                	memory[address+i] &= highbyte;
                	memory[address+i+1] &= lowbyte;
            	}
            }

            if(!sscanf(&line[9+2*recordLen], "%02x", &value)) {
                return false;
            }

            checksum = (~checksum)+1;
            if(value != checksum){
                return false;
            }

        } else if(fieldType == TYPE_EXTEND) {
            if (!sscanf(&line[9], "%04x", &value)) {
                return false;
            }
            extendedAddress = ((uint32_t)value << 16);

        }
    }

    return true;
}