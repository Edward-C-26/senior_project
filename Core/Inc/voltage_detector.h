/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : voltage_detector.h
  * @brief          : Header for voltage detection module
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __VOLTAGE_DETECTOR_H
#define __VOLTAGE_DETECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* ADC Configuration */
#define ADC_CHANNEL              0
#define ADC_INSTANCE             ((ADC_TypeDef *)ADC1_BASE)
#define ADC_GPIO_PORT            GPIOA
#define ADC_GPIO_PIN             GPIO_PIN_0
#define ADC_CLK_ENABLE()         __HAL_RCC_ADC1_CLK_ENABLE()
#define GPIO_CLK_ENABLE()        __HAL_RCC_GPIOA_CLK_ENABLE()

/* Voltage calculation constants */
#define VREF                     3.3f
#define ADC_MAX_VALUE            4095.0f
#define VOLTAGE_SCALE            (VREF / ADC_MAX_VALUE)

typedef struct {
    uint32_t raw_value;
    float voltage;
    uint32_t sample_count;
} VoltageDetector_t;

void VoltageDetector_Init(VoltageDetector_t *detector);
void VoltageDetector_Start(VoltageDetector_t *detector);
uint32_t VoltageDetector_GetRawValue(VoltageDetector_t *detector);
float VoltageDetector_GetVoltage(VoltageDetector_t *detector);
void VoltageDetector_UpdateSampleCount(VoltageDetector_t *detector);

#ifdef __cplusplus
}
#endif

#endif /* __VOLTAGE_DETECTOR_H */
