
#include <stdio.h>
#include <stdlib.h>
#include "type.h"

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

bool parseHexFile(FILE* stream, uint8_t* memory, size_t memorySize)
{
    char * line = NULL;
    size_t len = 0;
    int read;


    memset(memory, 0xff, memorySize);

    while ((read = getline(&line, &len, stream)) != -1) {

        int recordLen;
        int address;
        int fieldType;
        u_int8_t checksum = 0;
        int value;

        if(sscanf(line, ":%02x%04x%02x", &recordLen, &address, &fieldType) != 3){
            return false;
        }

        for(int i=0;i<4;i++){
            sscanf(&line[1+2*i], "%02x", &value);
            checksum += value;
        }

        if(fieldType == TYPE_DATA){
            for(int i=0;i<recordLen;i++){
                if(!sscanf(&line[9+2*i], "%02x", &value)) {
                    return false;
                }
                checksum += value;
                memory[address+i] = value;
            }

            if(!sscanf(&line[9+2*recordLen], "%02x", &value)) {
                return false;
            }

            checksum = (~checksum)+1;
            if(value != checksum){
                return false;
            }

        }
    }

    return true;
}