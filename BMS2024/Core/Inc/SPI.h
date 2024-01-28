#include "stm32f1xx.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"

void SPI_Init(void);
bool SPIWrite(uint8_t *writeBuffer, uint8_t totalBytes);
bool SPIWriteRead(uint8_t *writeBuffer, uint8_t *readBuffer, uint8_t totalBytes);
