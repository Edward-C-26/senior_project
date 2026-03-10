/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : voltage_detector.c
  * @brief          : Voltage detection implementation using direct register access
  ******************************************************************************
  */
/* USER CODE END Header */

#include "voltage_detector.h"

/* External ADC handle from main.c */
extern ADC_HandleTypeDef hadc1;

/* STM32F401 Register definitions */
#define RCC_APB2ENR  (*(volatile uint32_t *)(0x40023800 + 0x44))
#define RCC_AHB1ENR  (*(volatile uint32_t *)(0x40023800 + 0x30))

#define ADC1_CR2     (*(volatile uint32_t *)(0x40012000 + 0x08))
#define ADC1_SMPR2   (*(volatile uint32_t *)(0x40012000 + 0x10))
#define ADC1_SQR3    (*(volatile uint32_t *)(0x40012000 + 0x34))
#define ADC1_DR      (*(volatile uint32_t *)(0x40012000 + 0x4C))

#define GPIOA_MODER  (*(volatile uint32_t *)(0x40020000 + 0x00))

void VoltageDetector_Init(VoltageDetector_t *detector)
{
    /* Enable GPIOA and ADC1 clocks */
    RCC_AHB1ENR |= (1 << 0);  /* Enable GPIOA clock */
    RCC_APB2ENR |= (1 << 8);  /* Enable ADC1 clock */
    
    /* Configure PA0 as analog input */
    GPIOA_MODER &= ~(3 << 0);  /* Clear PA0 mode bits */
    GPIOA_MODER |= (3 << 0);   /* Set PA0 to analog mode (11) */
    
    /* Configure ADC1 */
    /* Set sampling time for channel 0 to 84 cycles */
    ADC1_SMPR2 &= ~(7 << 0);    /* Clear channel 0 sampling bits */
    ADC1_SMPR2 |= (6 << 0);     /* Set to 84 cycles (110 = 84 cycles) */
    
    /* Set channel 0 in regular sequence */
    ADC1_SQR3 &= ~(0x1F << 0);  /* Clear channel selection bits */
    ADC1_SQR3 |= (0 << 0);       /* Select channel 0 */
    
    /* Enable ADC1 */
    ADC1_CR2 |= (1 << 0);  /* ADON: ADC ON */
    
    /* Initialize structure */
    detector->raw_value = 0;
    detector->voltage = 0.0f;
    detector->sample_count = 0;
}

void VoltageDetector_Start(VoltageDetector_t *detector)
{
    /* Start conversion */
    ADC1_CR2 |= (1 << 30);  /* SWSTART: Start conversion */
}

uint32_t VoltageDetector_GetRawValue(VoltageDetector_t *detector)
{
    /* Use HAL to start and read ADC */
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        detector->raw_value = HAL_ADC_GetValue(&hadc1);
    } else {
        printf("ADC conversion timeout\r\n");
        detector->raw_value = 0;
    }
    HAL_ADC_Stop(&hadc1);
    
    /* Calculate voltage */
    detector->voltage = detector->raw_value * VOLTAGE_SCALE;
    
    return detector->raw_value;
}

float VoltageDetector_GetVoltage(VoltageDetector_t *detector)
{
    return detector->voltage;
}

void VoltageDetector_UpdateSampleCount(VoltageDetector_t *detector)
{
    detector->sample_count++;

}
