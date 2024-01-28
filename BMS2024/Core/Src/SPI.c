#include "SPI.h"

SPI_HandleTypeDef SPIHandle;

void SPI_Init(void)
{

  SPIHandle.Instance = SPI1;
  SPIHandle.Init.Mode = SPI_MODE_MASTER;
  SPIHandle.Init.Direction = SPI_DIRECTION_2LINES;
  SPIHandle.Init.DataSize = SPI_DATASIZE_8BIT;
  SPIHandle.Init.CLKPolarity = SPI_POLARITY_HIGH;
  SPIHandle.Init.CLKPhase = SPI_PHASE_2EDGE;
  SPIHandle.Init.NSS = SPI_NSS_HARD_OUTPUT;
  SPIHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // minimum SPI period is 1us, so minimum prescaler is 8
  SPIHandle.Init.FirstBit = SPI_FIRSTBIT_MSB;
  SPIHandle.Init.TIMode = SPI_TIMODE_DISABLE;
  SPIHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  SPIHandle.Init.CRCPolynomial = 10;

  HAL_SPI_Init(&SPIHandle);

}

bool SPIWrite(uint8_t *writeBuffer, uint8_t totalBytes) {

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET); //For debug only
	HAL_StatusTypeDef halReturnStatus;
	uint8_t readBuffer[totalBytes];

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	// HAL_Delay(1);
	halReturnStatus = HAL_SPI_TransmitReceive(&SPIHandle, writeBuffer, readBuffer, totalBytes, 1000);
	while( SPIHandle.State == HAL_SPI_STATE_BUSY );  // wait xmission complete
	// HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET); //For debug only

	return(halReturnStatus == HAL_OK);
};

bool SPIWriteRead(uint8_t *writeBuffer, uint8_t *readBuffer, uint8_t totalBytes) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET); //For debug only

	HAL_StatusTypeDef halReturnStatus;

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	// HAL_Delay(1);
	halReturnStatus = HAL_SPI_TransmitReceive(&SPIHandle, writeBuffer, readBuffer, totalBytes, 1000);
	while( SPIHandle.State == HAL_SPI_STATE_BUSY );  // wait xmission complete
	// HAL_Delay(1);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET); //For debug only

	return(halReturnStatus == HAL_OK);
};
