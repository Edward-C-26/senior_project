/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include "BMSconfig.h"
#include "Fault.h"
// #include "LTC2949.h"
#include "LTC6811.h"
#include "PackCalculations.h"
#include "SPI.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CELLVAL_ID 0x780
#define BMSSTAT_ID 0x781
#define BMSVINF_ID 0x782
#define BMSTINF_ID 0x783
#define PACKSTAT_ID 0x180	
#define CHARGER_OUT_ID 0x405
#define CELL_VOLTAGE_FAULTS 0x785
#define CELL_TEMP_FAULTS 0x786

#define CHARGER_IN_ID 0x1806E5F4
#define CHARGER_INFO_ID 0x700


#define TEST_FAN 0
#define BALANCE_EN  0

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
SPI_HandleTypeDef *ltc_spi = &hspi1;

CAN_TxHeaderTypeDef TxHeader;
CAN_RxHeaderTypeDef RxHeader;
uint8_t TxData[8];
uint8_t RxData[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint32_t TxMailbox;

CAN_TxHeaderTypeDef ChargerTxHeader;
uint8_t ChargerTxData[8];
uint8_t charge_rate = 2;
uint8_t global_error_count = 0;
uint8_t balance_counter = 0;

bool CHARGE_EN = 1;
bool BMS_FAULT = 0;

int david = -2;
uint16_t adi = 0;
uint16_t *buff_2949;
bool ret_2949;

uint8_t BMS_DATA[144][6];
uint8_t BMS_STATUS[6];
uint8_t CELLVAL_DATA[6];
bool discharge[12][12];
bool full_discharge[12][12];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_SPI3_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
void setChargerTxData(BMSConfigStructTypedef cfg);
void CELLVAL_message(BMSConfigStructTypedef cfg, uint8_t bmsData[144][6]);
void BMSSTAT_message(BMSConfigStructTypedef cfg, uint8_t bmsStatus[6]);
void BMSVINF_message(BMSConfigStructTypedef cfg, BMS_critical_info_t bms);
void BMSTINF_message(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, bool BMS_FAULT);
// void PACKSTAT_message(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, uint8_t bmsData[144][6]); // old method 
void PACKSTAT_message(BMSConfigStructTypedef cfg, BMS_critical_info_t bms); // draft :D

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
    BMSConfigStructTypedef BMSConfig;
    BMS_critical_info_t BMSCriticalInfo;

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN1_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
    initPECTable();
    loadConfig(&BMSConfig);
    init_BMS_info(&BMSCriticalInfo, &BMSConfig);

    HAL_CAN_Start(&hcan1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        david++;

        if (TEST_FAN == 1) {
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
            TIM2->CCR2 = 0;
            HAL_Delay(1000);
            TIM2->CCR2 = 25;
            HAL_Delay(1000);
            TIM2->CCR2 = 50;
            HAL_Delay(1000);
            TIM2->CCR2 = 75;
            HAL_Delay(1000);
            TIM2->CCR2 = 100;
            HAL_Delay(1000);
        }

        writeConfigAll(&BMSConfig);
        HAL_Delay(50);	 // TODO: Why is this here?

        /** FUNCTION CALL OVERVIEW
         * First: Call read2949
         * Second: Call read6811 -> Voltages, Temps on 6811
         * Third: Check for Faults
         * Fourth: Call balancing algorithm
         * Last: Remaining CAN messages
         */

        readAllCellVoltages(BMSConfig, BMS_DATA);
        setCriticalVoltages(BMSConfig, BMSCriticalInfo, BMS_DATA);
        BMSVINF_message(BMSConfig, BMSCriticalInfo);

        // Read all Temps from LTC6811, store them in 144x6 array, set the critical info struct, then send temp info over CAN
        readAllCellTemps(BMSConfig, BMS_DATA);
        setCriticalTemps(BMSConfig, BMSCriticalInfo, BMS_DATA);
        BMSTINF_message(BMSConfig, BMSCriticalInfo, BMS_FAULT);

        // Check cell connections -> not sure if this works -_-
        checkAllCellConnections(BMSConfig, BMS_DATA);
        BMSConfig.UV_threshold = (CHARGE_EN == 0) ? BMSConfig.LUV_threshold : BMSConfig.HUV_threshold;

        /* DO THIS WHEN TESTING BMS FAULTS*/    //*NOTE* : need to figure out which pin is the BMS fault pin
         bool BMS_FAULT = FAULT_check(BMSConfig, BMSCriticalInfo, BMS_DATA, BMS_STATUS);
         if (BMS_FAULT == false) {
        	 global_error_count = 0;
        	 HAL_GPIO_WritePin(BMS_FLT_EN_GPIO_Port, BMS_FLT_EN_Pin, GPIO_PIN_RESET);
         }
         else {
         	global_error_count++;
         	if (global_error_count == 20) {
         		global_error_count = 0;
         		HAL_GPIO_WritePin(BMS_FLT_EN_GPIO_Port, BMS_FLT_EN_Pin, GPIO_PIN_SET);
             }
         }

        // Finally, balance if charging and toggled 
        // TODO: make no balance when bms fault
        if(CHARGE_EN == 0 && BALANCE_EN == 1) {
            if(charge_rate != 0) {
                balance(BMSConfig, BMSCriticalInfo, BMS_DATA, discharge, full_discharge, balance_counter, charge_rate);

                if(balance_counter == 12) {
                    balance_counter = 0;
                }
            }
            setChargerTxData(BMSConfig);

            if(charge_rate != 0) {
                dischargeCellGroups(&BMSConfig, discharge);
                HAL_Delay(BMSConfig.dischargeTime);
            } else {
                // checkDischarge(BMSConfig, full_discharge, BMS_DATA); // not anywhere in past code??? WTF
                dischargeCellGroups(&BMSConfig, full_discharge);
                HAL_Delay(BMSConfig.dischargeTime);
            }
        }

        // Send remaining CAN messages
        PACKSTAT_message(BMSConfig, BMSCriticalInfo);
        BMSSTAT_message(BMSConfig, BMS_STATUS);
        CELLVAL_message(BMSConfig, BMS_DATA);
        PACKSTAT_message(BMSConfig, BMSCriticalInfo);
    }

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 2;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  // This ties the interupt to the rx_fifo0 msg bufer
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
   {
     Error_Handler();
   }
  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 16;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|BMS_FLT_EN_Pin|EEPROM_WP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SPI1_6820_CS_Pin|SPI3_ADC_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI2_EEPROM_CS_GPIO_Port, SPI2_EEPROM_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC0 BMS_FLT_EN_Pin EEPROM_WP_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|BMS_FLT_EN_Pin|EEPROM_WP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : AIR_ACTIVE_Pin */
  GPIO_InitStruct.Pin = AIR_ACTIVE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(AIR_ACTIVE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_6820_CS_Pin SPI3_ADC_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_6820_CS_Pin|SPI3_ADC_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ADC_DRDY_Pin */
  GPIO_InitStruct.Pin = ADC_DRDY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ADC_DRDY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_EEPROM_CS_Pin */
  GPIO_InitStruct.Pin = SPI2_EEPROM_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI2_EEPROM_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CHARGE_EN_Pin */
  GPIO_InitStruct.Pin = CHARGE_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(CHARGE_EN_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void setChargerTxData(BMSConfigStructTypedef cfg) {
    ChargerTxHeader.StdId = CHARGER_OUT_ID;
    ChargerTxHeader.DLC = 8;

    /* voltage data (hex value of desired voltage (V) times 10)*/
    ChargerTxData[0] = (uint8_t)((cfg.chargerVoltage >> 8) & 0xFF);
    ChargerTxData[1] = (uint8_t)(cfg.chargerVoltage & 0xFF);

    /* set the current data (hex value of desired current (A) times 10) */
    switch (charge_rate) {
        case 1:
            /* lower current */
            ChargerTxData[2] = (uint8_t)((cfg.lowerCurrent >> 8) & 0xFF);
            ChargerTxData[3] = (uint8_t)(cfg.lowerCurrent & 0xFF);
            break;

        case 2:
            /* normal current */
            ChargerTxData[2] = (uint8_t)((cfg.normalCurrent >> 8) & 0xFF);
            ChargerTxData[3] = (uint8_t)(cfg.normalCurrent & 0xFF);
            break;

        default:
            /* no current */
            ChargerTxData[2] = 0x00;
            ChargerTxData[3] = 0x00;
    }

    /* these data bytes are not used */
    ChargerTxData[4] = 0x00;
    ChargerTxData[5] = 0x00;
    ChargerTxData[6] = 0x00;
    ChargerTxData[7] = 0x00;

    HAL_CAN_AddTxMessage(&hcan1, &ChargerTxHeader, ChargerTxData, &TxMailbox);
}

void CELLVAL_message(BMSConfigStructTypedef cfg, uint8_t bmsData[144][6]) {
    // canCounter1++;
    TxHeader.StdId = CELLVAL_ID;
    TxHeader.DLC = 6;


    //replace with memcopy?
    for (uint8_t cell = 0; cell < NUM_CELLS; cell++) {
        CELLVAL_DATA[0] = bmsData[cell][0];
        CELLVAL_DATA[1] = bmsData[cell][1];
        CELLVAL_DATA[2] = bmsData[cell][2];
        CELLVAL_DATA[3] = bmsData[cell][3];
        CELLVAL_DATA[4] = bmsData[cell][4];
        CELLVAL_DATA[5] = bmsData[cell][5];

        HAL_CAN_AddTxMessage(&hcan1, &TxHeader, CELLVAL_DATA, &TxMailbox);
        HAL_Delay(1);
    }
}

void BMSSTAT_message(BMSConfigStructTypedef cfg, uint8_t bmsStatus[6]) {
    // canCounter2++;

    TxHeader.StdId = BMSSTAT_ID;
    TxHeader.DLC = 6;

    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, bmsStatus, &TxMailbox);
}

void BMSVINF_message(BMSConfigStructTypedef cfg, BMS_critical_info_t bms) {
    uint16_t minV = bms.curr_min_voltage;
    uint8_t minCell = bms.min_volt_cell;
    uint16_t maxV = bms.curr_max_voltage;
    uint8_t maxCell = bms.max_volt_cell;


    TxHeader.StdId = BMSVINF_ID;
    TxHeader.DLC = 8;
    uint8_t BMSVINF_DATA[8];

    BMSVINF_DATA[0] = (uint8_t)((maxV >> 8) & 0xFF);
    BMSVINF_DATA[1] = (uint8_t)(maxV & 0xFF);
    BMSVINF_DATA[2] = maxCell;
    BMSVINF_DATA[3] = (uint8_t)((minV >> 8) & 0xFF);
    BMSVINF_DATA[4] = (uint8_t)(minV & 0xFF);
    BMSVINF_DATA[5] = minCell;
    // BMSVINF_DATA[6] = (uint8_t)((averageV >> 8) & 0xFF);
    // BMSVINF_DATA[7] = (uint8_t)(averageV & 0xFF);

    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, BMSVINF_DATA, &TxMailbox);  
}

void BMSTINF_message(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, bool BMS_FAULT) {
    uint16_t minT = bms.curr_min_temp;
    uint8_t minCell = bms.min_temp_cell;
    uint16_t maxT = bms.max_temp_cell;
    uint8_t maxCell = bms.curr_max_temp;

    // averageT = (uint16_t)(sum / (cfg.numOfICs * cfg.numOfTempPerIC)); //bug -> we only take 4 readings per board for temperatures
    // averageT = (uint16_t)(sum / (cfg.numOfICs * cfg.numOfTempPerIC));

    TxHeader.StdId = BMSTINF_ID;
    TxHeader.DLC = 8;
    uint8_t BMSTINF_DATA[8];

    BMSTINF_DATA[0] = (uint8_t)((maxT >> 8) & 0xFF);
    BMSTINF_DATA[1] = (uint8_t)(maxT & 0xFF);
    BMSTINF_DATA[2] = maxCell;
    BMSTINF_DATA[3] = (uint8_t)((minT >> 8) & 0xFF);
    BMSTINF_DATA[4] = (uint8_t)(minT & 0xFF);
    BMSTINF_DATA[5] = minCell;
    // BMSTINF_DATA[6] = (uint8_t)((averageT >> 8) & 0xFF);
    // BMSTINF_DATA[7] = (uint8_t)(averageT & 0xFF);

    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, BMSTINF_DATA, &TxMailbox);

    // Insert PWM code
    if ((maxT < 17400) || BMS_FAULT) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
        // HAL_GPIO_TogglePin(DEBUG_GPIO_Port, DEBUG_Pin);	// which PIN is this? 
    }
}

void PACKSTAT_message(BMSConfigStructTypedef cfg, BMS_critical_info_t bms) {
    TxHeader.StdId = PACKSTAT_ID;
    TxHeader.DLC = 6;
    uint8_t PACKSTAT_DATA[6];

    uint16_t pack_voltage = bms.packVoltage;
    uint16_t pack_current = bms.packCurrent;
    uint16_t pack_power = bms.packPower;

    PACKSTAT_DATA[0] = (uint8_t)((pack_voltage >> 8) & 0xFF);
    PACKSTAT_DATA[1] = (uint8_t)(pack_voltage & 0xFF);
    PACKSTAT_DATA[2] = (uint8_t)((pack_current >> 8) & 0xFF);
    PACKSTAT_DATA[3] = (uint8_t)(pack_current & 0xFF);
    PACKSTAT_DATA[4] = (uint8_t)((pack_power >> 8) & 0xFF);
    PACKSTAT_DATA[5] = (uint8_t)(pack_power & 0xFF);

    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, PACKSTAT_DATA, &TxMailbox);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan){
	//TODO: use HAL_CAN_GetRxMessage() to get the message off the rx fifo
}

// This will not work if we aren't using ADC
/*
void PACKSTAT_message(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, uint8_t bmsData[144][6]) {
    // canCounter3++;
    uint16_t channel1;
    uint16_t channel2;
    uint16_t pack_voltage;

    // +/- 20A (high res, channel 1)
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    // HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    channel1 = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    // +/- 500A (full range, channel 2)
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    // HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    channel2 = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    pack_voltage = (uint16_t)((bms.packVoltage / 10000) * 100);	// mV to V

    TxHeader.StdId = PACKSTAT_ID;
    TxHeader.DLC = 7;
    uint8_t PACKSTAT_DATA[7];

    PACKSTAT_DATA[0] = (uint8_t)((pack_voltage >> 8) & 0xFF);
    PACKSTAT_DATA[1] = (uint8_t)(pack_voltage & 0xFF);
    PACKSTAT_DATA[2] = (uint8_t)((channel1 >> 8) & 0xFF);
    PACKSTAT_DATA[3] = (uint8_t)(channel1 & 0xFF);
    PACKSTAT_DATA[4] = (uint8_t)((channel2 >> 8) & 0xFF);
    PACKSTAT_DATA[5] = (uint8_t)(channel2 & 0xFF);
    PACKSTAT_DATA[6] = 0x00;
    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, PACKSTAT_DATA, &TxMailbox);
    // returns message with cell voltage data
}
*/

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
