/**
 * @file SPI.c
 * Last Edited Spring 2024 : David Lacayo
*/

#include "SPI.h"
// MAX DELAY MAX_SPI_DELAY
bool SPIWrite(uint8_t *writeBuffer, uint8_t totalBytes) {
	START_CRITICAL_SECTION;
//	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);	 // For debug only
	HAL_StatusTypeDef halReturnStatus;
	uint8_t readBuffer[totalBytes];

	HAL_GPIO_WritePin(SPI_UCOMM_CS_GPIO_Port, SPI_UCOMM_CS_Pin, GPIO_PIN_RESET);
	 HAL_Delay(2);
	halReturnStatus = HAL_SPI_TransmitReceive(ltc_spi, writeBuffer, readBuffer, totalBytes, MAX_SPI_DELAY);//1000);
	while (ltc_spi->State == HAL_SPI_STATE_BUSY)
		;  // wait xmission complete
	 HAL_Delay(2);
	HAL_GPIO_WritePin(SPI_UCOMM_CS_GPIO_Port, SPI_UCOMM_CS_Pin, GPIO_PIN_SET);

//	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);  // For debug only
	END_CRITICAL_SECTION;
	return (halReturnStatus == HAL_OK);
};

bool SPIWriteRead(uint8_t *writeBuffer, uint8_t *readBuffer, uint8_t totalBytes) {
	START_CRITICAL_SECTION;
//	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);	 // For debug only

	HAL_StatusTypeDef halReturnStatus;

	HAL_GPIO_WritePin(SPI_UCOMM_CS_GPIO_Port, SPI_UCOMM_CS_Pin, GPIO_PIN_RESET);
	 HAL_Delay(2);
	halReturnStatus = HAL_SPI_TransmitReceive(ltc_spi, writeBuffer, readBuffer, totalBytes,  MAX_SPI_DELAY);//1000);
	while (ltc_spi->State == HAL_SPI_STATE_BUSY)
		;  // wait xmission complete
	 HAL_Delay(2);
	HAL_GPIO_WritePin(SPI_UCOMM_CS_GPIO_Port, SPI_UCOMM_CS_Pin, GPIO_PIN_SET);

//	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);  // For debug only
	END_CRITICAL_SECTION;

	return (halReturnStatus == HAL_OK);
};
