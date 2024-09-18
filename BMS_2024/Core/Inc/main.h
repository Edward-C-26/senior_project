/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern SPI_HandleTypeDef* ltc_spi;
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define AIR_ACTIVE_Pin GPIO_PIN_3
#define AIR_ACTIVE_GPIO_Port GPIOC
#define SPI1_6820_CS_Pin GPIO_PIN_4
#define SPI1_6820_CS_GPIO_Port GPIOA
#define SPI1_6820_SCK_Pin GPIO_PIN_5
#define SPI1_6820_SCK_GPIO_Port GPIOA
#define SPI1_6820_MISO_Pin GPIO_PIN_6
#define SPI1_6820_MISO_GPIO_Port GPIOA
#define SPI1_6820_MOSI_Pin GPIO_PIN_7
#define SPI1_6820_MOSI_GPIO_Port GPIOA
#define BMS_FLT_EN_Pin GPIO_PIN_4
#define BMS_FLT_EN_GPIO_Port GPIOC
#define ADC_CLKIN_Pin GPIO_PIN_0
#define ADC_CLKIN_GPIO_Port GPIOB
#define ADC_DRDY_Pin GPIO_PIN_1
#define ADC_DRDY_GPIO_Port GPIOB
#define SPI2_EEPROM_CS_Pin GPIO_PIN_12
#define SPI2_EEPROM_CS_GPIO_Port GPIOB
#define SPI2_EEPROM_SCK_Pin GPIO_PIN_13
#define SPI2_EEPROM_SCK_GPIO_Port GPIOB
#define SPI2_EEPROM_MISO_Pin GPIO_PIN_14
#define SPI2_EEPROM_MISO_GPIO_Port GPIOB
#define SPI2_EEPROM_MOSI_Pin GPIO_PIN_15
#define SPI2_EEPROM_MOSI_GPIO_Port GPIOB
#define EEPROM_WP_Pin GPIO_PIN_8
#define EEPROM_WP_GPIO_Port GPIOC
#define CHARGE_EN_Pin GPIO_PIN_8
#define CHARGE_EN_GPIO_Port GPIOA
#define SPI3_ADC_CS_Pin GPIO_PIN_15
#define SPI3_ADC_CS_GPIO_Port GPIOA
#define SPI3_ADC_SCK_Pin GPIO_PIN_10
#define SPI3_ADC_SCK_GPIO_Port GPIOC
#define SPI3_ADC_MISO_Pin GPIO_PIN_11
#define SPI3_ADC_MISO_GPIO_Port GPIOC
#define SPI3_ADC_MOSI_Pin GPIO_PIN_12
#define SPI3_ADC_MOSI_GPIO_Port GPIOC
#define FAN_PWM_Pin GPIO_PIN_3
#define FAN_PWM_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

// This struct holds the configuration variables required for manual balancing
// https://confluence.illinielectricmotorsports.com/display/SOF/Manual+Balancing+Implementation
typedef struct {
	bool charge_en;				// defaults to false
	uint16_t charge_voltage;	// defaults to 600V | multiplied by 10 | 600V -> 6000 in the variable
	uint16_t charge_current;	// defaults to 0A	| multiplied by 10 | 10A -> 100 in the variable

	bool discharge_balance_en;	// defaults to false
	uint8_t num_cells_discharged_per_secondary;	// defaults to 0, must be set between 1 to 12 when discharge_balance_en == true
	uint16_t discharge_threshold_voltage;	// defaults to 4.16V | multiplied by 10,000 | 4.16V -> 41600 in the variable

	bool valid_charge_message;

} manual_balancing_t;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
