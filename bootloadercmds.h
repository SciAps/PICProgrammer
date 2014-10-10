
bool enterBootloader();
bool setAddressPointer(int address);
bool getAddressPointer(int* address);
bool eraseFlash();
bool uploadBlock(uint8_t* block);
bool downloadBlock(uint8_t* block);
bool verifyBlock(uint8_t* a, uint8_t* b);
bool writeFlash();
bool exitBootloader();

#define CMD(fun) if(!fun) {return -1;}
#define BLOCK_SIZE 64