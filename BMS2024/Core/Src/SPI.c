#include "SPI.h"

void SPI_Init(void) {
	/* This SPI handle is already initialized in main.c
	 * If some SPI settings needs to be modified,
	 * they should be modified from within the .ioc file
	 * for consistency and ease of use
	 */

	// ltc_spi->Instance = SPI1;
	// ltc_spi->Init.Mode = SPI_MODE_MASTER;
	// ltc_spi->Init.Direction = SPI_DIRECTION_2LINES;
	// ltc_spi->Init.DataSize = SPI_DATASIZE_8BIT;
	// ltc_spi->Init.CLKPolarity = SPI_POLARITY_HIGH;
	// ltc_spi->Init.CLKPhase = SPI_PHASE_2EDGE;
	// ltc_spi->Init.NSS = SPI_NSS_HARD_OUTPUT;
	// ltc_spi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  // minimum SPI period is 1us, so minimum prescaler is 8
	// ltc_spi->Init.FirstBit = SPI_FIRSTBIT_MSB;
	// ltc_spi->Init.TIMode = SPI_TIMODE_DISABLE;
	// ltc_spi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	// ltc_spi->Init.CRCPolynomial = 10;

	// HAL_SPI_Init(ltc_spi);
}

bool SPIWrite(uint8_t *writeBuffer, uint8_t totalBytes) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);	 // For debug only
	HAL_StatusTypeDef halReturnStatus;
	uint8_t readBuffer[totalBytes];

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	// HAL_Delay(1);
	halReturnStatus = HAL_SPI_TransmitReceive(ltc_spi, writeBuffer, readBuffer, totalBytes, 1000);
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
	halReturnStatus = HAL_SPI_TransmitReceive(ltc_spi, writeBuffer, readBuffer, totalBytes, 1000);
	while (ltc_spi->State == HAL_SPI_STATE_BUSY)
		;  // wait xmission complete
	// HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);  // For debug only

	return (halReturnStatus == HAL_OK);
};
