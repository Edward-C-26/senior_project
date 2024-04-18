/**
 * Last Edited Spring 2024 : David Lacayo
*/

#include "SPI.h"

bool SPIWrite(uint8_t *writeBuffer, uint8_t totalBytes) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);	 // For debug only
	HAL_StatusTypeDef halReturnStatus;
	uint8_t readBuffer[totalBytes];

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	// HAL_Delay(1);
	halReturnStatus = HAL_SPI_TransmitReceive(ltc_spi, writeBuffer, readBuffer, totalBytes, HAL_MAX_DELAY);//1000);
	while (ltc_spi->State == HAL_SPI_STATE_BUSY)
		;  // wait xmission complete
	// HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);  // For debug only

	return (halReturnStatus == HAL_OK);
};

bool SPIWriteRead(uint8_t *writeBuffer, uint8_t *readBuffer, uint8_t totalBytes) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);	 // For debug only

	HAL_StatusTypeDef halReturnStatus;

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	// HAL_Delay(1);
	halReturnStatus = HAL_SPI_TransmitReceive(ltc_spi, writeBuffer, readBuffer, totalBytes,  HAL_MAX_DELAY);//1000);
	while (ltc_spi->State == HAL_SPI_STATE_BUSY)
		;  // wait xmission complete
	// HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);  // For debug only

	return (halReturnStatus == HAL_OK);
};
