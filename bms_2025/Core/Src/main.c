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
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define POLLING_ID 0x4E0
#define CELLVAL_ID 0x780
#define BMSSTAT_ID 0x781
#define BMSVINF_ID 0x782
#define BMSTINF_ID 0x783
#define PACKSTAT_ID 0x180	
#define CHARGER_OUT_ID 0x405
#define CELL_VOLTAGE_FAULTS 0x785
#define CELL_TEMP_FAULTS 0x786
#define CAN_RX_MAILBOX_SIZE 3

#define CHARGER_IN_ID 0x1806E5F4	// charger CAN ID is 0x1806E5F4
#define CHARGER_INFO_ID 0x700
#define CONSTANT_CAN_ENABLE 1


#define BALANCE_EN  0

// STM manual balancing
#define THRESHOLD_BALANCE_EN 0
#define DISCHARGE_THRESHOLD 65535 // well above cell max voltage LOOK AT ACTUAL FUNCTION CALL FOR THRESHOLD SETTING

// CAN Manual Balancing
#define MANUAL_BALANCING_ID 0x4E1

// CAN transmit timeout (ms)
// Assuming a 0% bus load 500kbit/s CAN bus, 5ms would be enough time to send ~23 8 byte CAN messages
// If we hit the 5ms timeout without transmitting any pending messages, we're either at an extremely high bus load
// and losing arbitration or there's some other issue (bus disconnected, no termination, etc.)
#define CAN_TX_TIMEOUT 5


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
SPI_HandleTypeDef *ltc_spi = &hspi1;

CAN_TxHeaderTypeDef tx_header;
CAN_RxHeaderTypeDef rx_header;
uint8_t tx_data[8];
uint8_t rx_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint32_t tx_mailbox;

CAN_TxHeaderTypeDef charger_tx_header;
uint8_t charger_tx_data[8];
uint8_t charge_rate = 2;
uint8_t global_error_count = 0;
uint8_t balance_counter = 0;
uint8_t charging_counter = 0;

bool bmsFault = 0;
bool CHARGE_EN = 0;

uint16_t *buff_2949;
bool ret_2949;

CellData bmsData[144];
uint8_t BMS_STATUS[6];
bool discharge[12][12];
bool full_discharge[12][12];

manual_balancing_t manual_balancing_config;

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
static void MX_CAN2_Init(void);
/* USER CODE BEGIN PFP */
static void setChargerTxData(BMSConfigStructTypedef cfg);
static bool transmit_can_message(CAN_HandleTypeDef* hcan, const CAN_TxHeaderTypeDef *tx_header, const uint8_t msg_data[]);
static void CELLVAL_message(CellData const bmsData[144]);
static void BMSSTAT_message(uint8_t const bmsStatus[6]);
static void BMSVINF_message(BMS_critical_info_t const *bms);
static void BMSTINF_message(BMS_critical_info_t const *bms, bool bmsFault);
// void PACKSTAT_message(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, uint8_t bmsData[144][6]); // old method 
static void PACKSTAT_message(BMS_critical_info_t const *bms); // draft :D

// Manual Charging/Balancing Functions
void CHARGER_message();
void resetChargerVariables();
void getLaptopCanMessage();
bool pollingFlag = false;
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
  MX_CAN2_Init();
  /* USER CODE BEGIN 2 */
    initPECTable();
    loadConfig(&BMSConfig);
    init_BMS_info(&BMSCriticalInfo);
    resetChargerVariables();

    HAL_CAN_Start(&hcan1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    	TIM2 -> CCR2 = 50;

        writeConfigAll(&BMSConfig);

        /** FUNCTION CALL OVERVIEW
         * First: Call read6811 -> Voltages, Temps on 6811
         * Second: Check for Faults
         * Third: Call balancing algorithm
         * Fourth : Remaining CAN messages
         * Last: :3 
         */
        readAllCellVoltages(bmsData);
        setCriticalVoltages(&BMSCriticalInfo, bmsData);


        // Read all Temps from LTC6811, store them in 144x6 array,
        // set the critical info struct, then send temp info over CAN
        readAllCellTemps(bmsData);

        setCriticalTemps(&BMSCriticalInfo, bmsData);

        BMSConfig.UV_threshold = (CHARGE_EN == 0) 
            ? BMSConfig.LUV_threshold : BMSConfig.HUV_threshold;


        /* DO THIS WHEN TESTING BMS FAULTS*/    
        //*NOTE* : need to figure out which pin is the BMS fault pin
         bool bmsFault = FAULT_check(&BMSCriticalInfo, BMS_STATUS);
         if (bmsFault == false) {
        	 global_error_count = 0;
        	 HAL_GPIO_WritePin(BMS_FLT_EN_GPIO_Port, BMS_FLT_EN_Pin, 
                     GPIO_PIN_RESET);
         }
         else {
         	global_error_count++;
         	if (global_error_count == 5) {
         		global_error_count = 0;
         		HAL_GPIO_WritePin(BMS_FLT_EN_GPIO_Port, BMS_FLT_EN_Pin,
                        GPIO_PIN_SET);
             }
         }

        // Finally, balance if charging and toggled 
        // TODO: make no balance when bms fault
        if(CHARGE_EN == 0 && BALANCE_EN == 1) {
            if(charge_rate != 0) {
                balance(&BMSConfig, &BMSCriticalInfo, bmsData, discharge,
                        full_discharge, balance_counter, &charge_rate);

                if(balance_counter == 12) {
                    balance_counter = 0;
                }
            }
            setChargerTxData(BMSConfig);

            if(charge_rate != 0) {
                dischargeCellGroups(&BMSConfig, discharge);
                HAL_Delay(BMSConfig.dischargeTime);
            } else {
                // checkDischarge(BMSConfig, full_discharge, bmsData);
                // not anywhere in past code??? WTF
                dischargeCellGroups(&BMSConfig, full_discharge);
                HAL_Delay(BMSConfig.dischargeTime);
            }
        }

        // poll for CAN1 Message from Laptop
        if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0) {
        	getLaptopCanMessage();

            int messagesDiscarded = 0;
			// Clear the receive FIFO0 by reading and discarding all messages
			while (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0
                    || messagesDiscarded >= CAN_RX_MAILBOX_SIZE)
			{
				// Retrieve and discard the message
				HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rx_header, rx_data);
                messagesDiscarded++;
			}
    	}

        // Discharge cells if charge message is valid AND discharge is enabled AND there is no BMS fault
        if((manual_balancing_config.valid_charge_message == true) &&
			(manual_balancing_config.discharge_balance_en == true) &&
			(bmsFault == false)) {

            thresholdBalance(&BMSConfig, &BMSCriticalInfo, bmsData, discharge, manual_balancing_config.discharge_threshold_voltage, manual_balancing_config.num_cells_discharged_per_secondary);
          
//            setChargerTxData(BMSConfig); // this is unsed for now
            dischargeCellGroups(&BMSConfig, discharge);
            HAL_Delay(BMSConfig.dischargeTime);

            dischargeCellGroups(&BMSConfig, discharge);
            HAL_Delay(BMSConfig.dischargeTime);
        }
        else {
        	thresholdBalance(&BMSConfig, &BMSCriticalInfo, bmsData, discharge, 43000, 0);	// default to an "off" state, cells should never go this high
        }

        // send CAN messages to charger
        // if charge message from laptop is valid AND charge is enabled AND there is no BMS fault
        if ((manual_balancing_config.valid_charge_message == true) &&
			(manual_balancing_config.charge_en == true) &&
			(!bmsFault == true)) {
        	CHARGER_message();
        }

        // stop messages from being sent if no message has been sent in 2 seconds
        if (charging_counter > 4) {
        	resetChargerVariables();
        }
        charging_counter++;

        // Send remaining CAN messages
        BMSVINF_message(&BMSCriticalInfo);    
        BMSTINF_message(&BMSCriticalInfo, bmsFault);
        PACKSTAT_message(&BMSCriticalInfo);
        BMSSTAT_message(BMS_STATUS);
        PACKSTAT_message(&BMSCriticalInfo);

      if (pollingFlag || CONSTANT_CAN_ENABLE){
        CELLVAL_message(bmsData);
        pollingFlag = false;
      }
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
  hcan1.Init.AutoBusOff = ENABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

	CAN_FilterTypeDef  sFilterConfig;
	sFilterConfig.FilterBank = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = 0x4E1 << 5;  // 0x4E1 shifted left by 5 bits
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0xFFE0;
	sFilterConfig.FilterMaskIdLow = 0x0000;
	sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	sFilterConfig.FilterActivation = ENABLE;
	sFilterConfig.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{
	  /* Filter configuration Error */
	  Error_Handler();
	}

  if (HAL_CAN_Start(&hcan1) != HAL_OK)
  {
	/* Start Error */
	Error_Handler();
  }

  // This ties the interupt to the rx_fifo0 msg bufer
//  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
//   {
//     Error_Handler();
//   }
  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief CAN2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 1;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = ENABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN2_Init 2 */

  /* USER CODE END CAN2_Init 2 */

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
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
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
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
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
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
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
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SPI_UCOMM_CS_Pin|SPI_FERAM_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI_ADC_CS_GPIO_Port, SPI_ADC_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ADC_RST_GPIO_Port, ADC_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI_FERAM_WP_GPIO_Port, SPI_FERAM_WP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : BMS_FLT_EN_Pin SHUTDOWN_ACTIVE_Pin LV_PWR_MONITOR_Pin ADC_DRDY_Pin */
  GPIO_InitStruct.Pin = BMS_FLT_EN_Pin|SHUTDOWN_ACTIVE_Pin|LV_PWR_MONITOR_Pin|ADC_DRDY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PRECHARGE_COMPLETE_Pin CHARGE_EN_Pin */
  GPIO_InitStruct.Pin = PRECHARGE_COMPLETE_Pin|CHARGE_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DEBUG_LED_Pin SPI_UCOMM_CS_Pin SPI_FERAM_CS_Pin */
  GPIO_InitStruct.Pin = DEBUG_LED_Pin|SPI_UCOMM_CS_Pin|SPI_FERAM_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI_ADC_CS_Pin */
  GPIO_InitStruct.Pin = SPI_ADC_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI_ADC_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ADC_RST_Pin */
  GPIO_InitStruct.Pin = ADC_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ADC_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI_FERAM_WP_Pin */
  GPIO_InitStruct.Pin = SPI_FERAM_WP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI_FERAM_WP_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static bool transmit_can_message(CAN_HandleTypeDef* hcan, const CAN_TxHeaderTypeDef *tx_header, const uint8_t msg_data[]) {
    uint32_t can_error_code = HAL_CAN_GetError(hcan);

    // If we're in bus off or error passive, skip the 5ms timeout (but try to transmit anyway)
    if (!(can_error_code & (HAL_CAN_ERROR_BOF | HAL_CAN_ERROR_EPV))) {
        // Bus OK - wait for up to 5ms for space to become available in the transmit mailboxes
        uint32_t start_tick = HAL_GetTick();
        while(HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0 && HAL_GetTick() - start_tick < CAN_TX_TIMEOUT) {}
    }

    return HAL_CAN_AddTxMessage(hcan, tx_header, msg_data, &tx_mailbox) == HAL_OK;
}

void setChargerTxData(BMSConfigStructTypedef cfg) {	// unused as of 10/15/24
    charger_tx_header.StdId = CHARGER_OUT_ID;
    charger_tx_header.DLC = 8;

    /* voltage data (hex value of desired voltage (V) times 10)*/
    charger_tx_data[0] = (uint8_t)((cfg.chargerVoltage >> 8) & 0xFF);
    charger_tx_data[1] = (uint8_t)(cfg.chargerVoltage & 0xFF);

    /* set the current data (hex value of desired current (A) times 10) */
    switch (charge_rate) {
        case 1:
            /* lower current */
            charger_tx_data[2] = (uint8_t)((cfg.lowerCurrent >> 8) & 0xFF);
            charger_tx_data[3] = (uint8_t)(cfg.lowerCurrent & 0xFF);
            break;

        case 2:
            /* normal current */
            charger_tx_data[2] = (uint8_t)((cfg.normalCurrent >> 8) & 0xFF);
            charger_tx_data[3] = (uint8_t)(cfg.normalCurrent & 0xFF);
            break;

        default:
            /* no current */
            charger_tx_data[2] = 0x00;
            charger_tx_data[3] = 0x00;
    }

    /* these data bytes are not used */
    charger_tx_data[4] = 0x00;
    charger_tx_data[5] = 0x00;
    charger_tx_data[6] = 0x00;
    charger_tx_data[7] = 0x00;

    transmit_can_message(&hcan1, &charger_tx_header, charger_tx_data);
}

static void CELLVAL_message(CellData const bmsData[144]) {
    // canCounter1++;
    tx_header.StdId = CELLVAL_ID;
    tx_header.DLC = 6;

    //Send dummy CAN message to wake up bus

    uint8_t CELLVAL_DATA[6];
    CELLVAL_DATA[0] = 0; // Should not  be a CELL_VAL message. Let's add a wake ID instead
    CELLVAL_DATA[1] = 0;
    CELLVAL_DATA[2] = 0;
    CELLVAL_DATA[3] = 0;
    CELLVAL_DATA[4] = 0;
    CELLVAL_DATA[5] = 0;


    transmit_can_message(&hcan1, &tx_header, CELLVAL_DATA);


    //replace with memcopy?
    for (uint8_t cell = 0; cell < NUM_CELLS; cell++) {
        CELLVAL_DATA[0] = cell;
        CELLVAL_DATA[1] = bmsData[cell].fault;
        CELLVAL_DATA[2] = (uint8_t) (bmsData[cell].voltage >> 8);
        CELLVAL_DATA[3] = (uint8_t)(bmsData[cell].voltage & 0xFF);
        CELLVAL_DATA[4] = (uint8_t) (bmsData[cell].temperature >> 8);
        CELLVAL_DATA[5] = (uint8_t)(bmsData[cell].temperature & 0xFF);

        transmit_can_message(&hcan1, &tx_header, CELLVAL_DATA);
    }
}

static void BMSSTAT_message(uint8_t const bmsStatus[6]) {
    // canCounter2++;

    tx_header.StdId = BMSSTAT_ID;
    tx_header.DLC = 6;

    transmit_can_message(&hcan1, &tx_header, bmsStatus);
}

static void BMSVINF_message(BMS_critical_info_t const *bms) {
    const uint16_t minV = bms->curr_min_voltage;
    const uint8_t minCell = bms->min_volt_cell;
    const uint16_t maxV = bms->curr_max_voltage;
    const uint8_t maxCell = bms->max_volt_cell;


    tx_header.StdId = BMSVINF_ID;
    tx_header.DLC = 8;
    uint8_t BMSVINF_DATA[8];

    BMSVINF_DATA[0] = (uint8_t)((maxV >> 8) & 0xFF);
    BMSVINF_DATA[1] = (uint8_t)(maxV & 0xFF);
    BMSVINF_DATA[2] = maxCell;
    BMSVINF_DATA[3] = (uint8_t)((minV >> 8) & 0xFF);
    BMSVINF_DATA[4] = (uint8_t)(minV & 0xFF);
    BMSVINF_DATA[5] = minCell;
    // BMSVINF_DATA[6] = (uint8_t)((averageV >> 8) & 0xFF);
    // BMSVINF_DATA[7] = (uint8_t)(averageV & 0xFF);

    transmit_can_message(&hcan1, &tx_header, BMSVINF_DATA);  
}

static void BMSTINF_message(BMS_critical_info_t const *bms, bool bmsFault) {
    const uint16_t minT = bms->curr_min_temp;
    const uint8_t minCell = bms->min_temp_cell;
    const uint16_t maxT = bms->curr_max_temp;
    const uint8_t maxCell = bms->max_temp_cell;

    // averageT = (uint16_t)(sum / (cfg.numOfICs * cfg.numOfTempPerIC)); //bug -> we only take 4 readings per board for temperatures
    // averageT = (uint16_t)(sum / (cfg.numOfICs * cfg.numOfTempPerIC));

    tx_header.StdId = BMSTINF_ID;
    tx_header.DLC = 8;
    uint8_t BMSTINF_DATA[8];

    BMSTINF_DATA[0] = (uint8_t)((maxT >> 8) & 0xFF);
    BMSTINF_DATA[1] = (uint8_t)(maxT & 0xFF);
    BMSTINF_DATA[2] = maxCell;
    BMSTINF_DATA[3] = (uint8_t)((minT >> 8) & 0xFF);
    BMSTINF_DATA[4] = (uint8_t)(minT & 0xFF);
    BMSTINF_DATA[5] = minCell;
    // BMSTINF_DATA[6] = (uint8_t)((averageT >> 8) & 0xFF);
    // BMSTINF_DATA[7] = (uint8_t)(averageT & 0xFF);

    transmit_can_message(&hcan1, &tx_header, BMSTINF_DATA);

    // Insert PWM code
    if ((maxT < 17400) || bmsFault) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
        // HAL_GPIO_TogglePin(DEBUG_GPIO_Port, DEBUG_Pin);	// which PIN is this? 
    }
}

static void PACKSTAT_message(BMS_critical_info_t const *bms) {
    tx_header.StdId = PACKSTAT_ID;
    tx_header.DLC = 6;
    uint8_t PACKSTAT_DATA[6];

    const uint16_t pack_voltage = bms->packVoltage;
    const uint16_t pack_current = bms->packCurrent;
    const uint16_t pack_power = bms->packPower;

    PACKSTAT_DATA[0] = (uint8_t)((pack_voltage >> 8) & 0xFF);
    PACKSTAT_DATA[1] = (uint8_t)(pack_voltage & 0xFF);
    PACKSTAT_DATA[2] = (uint8_t)((pack_current >> 8) & 0xFF);
    PACKSTAT_DATA[3] = (uint8_t)(pack_current & 0xFF);
    PACKSTAT_DATA[4] = (uint8_t)((pack_power >> 8) & 0xFF);
    PACKSTAT_DATA[5] = (uint8_t)(pack_power & 0xFF);

    transmit_can_message(&hcan1, &tx_header, PACKSTAT_DATA);
}

void CHARGER_message() {
	tx_header.StdId = 0;  // Set to 0 for extended IDs
	tx_header.ExtId = CHARGER_IN_ID; // Set your 29-bit ID
	tx_header.DLC = 8;				// set command length
	tx_header.IDE = CAN_ID_EXT;       // Set to extended ID

	uint8_t CHARGER_DATA[8];

	if (manual_balancing_config.charge_en == true && manual_balancing_config.valid_charge_message == true) {
		CHARGER_DATA[0] = (uint8_t)((manual_balancing_config.charge_voltage >> 8) & 0xFF);
		CHARGER_DATA[1] = (uint8_t)(manual_balancing_config.charge_voltage & 0xFF);
		CHARGER_DATA[2] = (uint8_t)((manual_balancing_config.charge_current >> 8) & 0xFF);
		CHARGER_DATA[3] = (uint8_t)(manual_balancing_config.charge_current & 0xFF);
	}
	else {
		return;	// abort send
	}

    /* these data bytes are not used */
	CHARGER_DATA[4] = 0x00;
	CHARGER_DATA[5] = 0x00;
	CHARGER_DATA[6] = 0x00;
	CHARGER_DATA[7] = 0x00;

    transmit_can_message(&hcan1, &tx_header, CHARGER_DATA);

    // reset to normal
	tx_header.StdId = 0;  // Set to 0 for extended IDs
	tx_header.ExtId = 0; // Set your 29-bit ID
	tx_header.DLC = 8;				// set command length
	tx_header.IDE = CAN_ID_STD;       // Set to extended ID
}

// reset all variables to default
void resetChargerVariables() {
	manual_balancing_config.charge_en = false;		// disable charge
	manual_balancing_config.charge_voltage = 6000;	// 600V
	manual_balancing_config.charge_current = 0;		// 0A

	manual_balancing_config.discharge_balance_en = false;	// disable discharge balance
	manual_balancing_config.num_cells_discharged_per_secondary = 0;	// no cells to be discharged
	manual_balancing_config.discharge_threshold_voltage = 41600;	// 4.16V

	manual_balancing_config.valid_charge_message = false;	// assume invalid message
}

void getLaptopCanMessage(){
  HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rx_header, rx_data);

  if (rx_header.StdId == POLLING_ID) {
    pollingFlag = true;
  }

  switch (rx_header.StdId) {
	  case MANUAL_BALANCING_ID: // insert laptop to charger can id here
		  manual_balancing_config.charge_en = (rx_data[0] == 0xFF);
		  manual_balancing_config.charge_voltage = ((rx_data[1] << 8) | (rx_data[2]));
		  manual_balancing_config.charge_current = ((rx_data[3] << 8) | (rx_data[4]));

		  manual_balancing_config.discharge_balance_en = (rx_data[5] & 0xF0) == 0xF0;
		  manual_balancing_config.num_cells_discharged_per_secondary = (rx_data[5] & 0x0F);
		  manual_balancing_config.discharge_threshold_voltage = ((rx_data[6] << 8) | (rx_data[7]));

		  // check for validity

		  // if both charge and balance are enabled, invalid message
		  if ((manual_balancing_config.charge_en == true) && (manual_balancing_config.discharge_balance_en == true)) {
			  manual_balancing_config.valid_charge_message = false;
			  resetChargerVariables();
			  break;
		  }

		  // if charge is enabled AND if charge voltage is out of bounds [500-600][V]
		  // multiplied by 10 -> [5000 - 6000]
		  if ((manual_balancing_config.charge_en == true) && ((manual_balancing_config.charge_voltage < 5000) || (manual_balancing_config.charge_voltage > 6000))) {
			  manual_balancing_config.valid_charge_message = false;
			  resetChargerVariables();
			  break;
		  }

		  // if charge is enabled AND if charge current is out of bounds [0-10][A]
		  // multiplied by 10 -> [0 - 100]
		  if ((manual_balancing_config.charge_en == true) && (manual_balancing_config.charge_current > 100)) {
			  manual_balancing_config.valid_charge_message = false;
			  resetChargerVariables();
			  break;
		  }

		  // if cell discharge count is out of bounds
		  if ((manual_balancing_config.discharge_balance_en == true) && (manual_balancing_config.num_cells_discharged_per_secondary > 12)) {
			  manual_balancing_config.valid_charge_message = false;
			  resetChargerVariables();
			  break;
		  }

		  // if discharge is enabled AND if discharge cell threshold is out of bounds [3.2-4.2][V]
		  // multiplied by 10,000 -> [32000 - 42000]
		  if ((manual_balancing_config.discharge_balance_en == true) && ((manual_balancing_config.discharge_threshold_voltage < 32000) || (manual_balancing_config.discharge_threshold_voltage > 42000))) {
			  manual_balancing_config.valid_charge_message = false;
			  resetChargerVariables();
			  break;
		  }

		  // if all conditions passed, charge message is true
		  manual_balancing_config.valid_charge_message = true;
		  charging_counter++;
		  break;

	  default:
		  break;
  }

}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    (void)hcan;
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
