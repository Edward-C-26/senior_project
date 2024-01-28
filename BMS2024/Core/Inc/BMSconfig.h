#include "stm32f1xx.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"

typedef struct {

	// General BMS configuration
	uint8_t numOfICs;
	uint8_t address[16];
	uint8_t addressesofICs[16];
	uint8_t numOfCellInputs;
	uint8_t numOfCellsPerIC;
	uint8_t numOfTempPerIC;
	uint8_t ADCConversionRate;
	uint16_t OV_threshold;
	uint16_t UV_threshold;
	uint16_t LUV_threshold;
	uint16_t HUV_threshold;
	uint16_t slowCharge_threshold;
	uint16_t stopCharge_threshold;
	uint16_t max_difference;
	uint16_t balancing_difference;
	uint8_t invalidPECcount;
	uint16_t dischargeTime;
	uint16_t start_scaling;
	uint16_t stop_scaling;
	uint16_t scale_to;
	uint16_t normalCurrent;
	uint16_t lowerCurrent;
	uint16_t chargerVoltage;

	// LTC configuration
	bool GPIO5PulldownOff;
	bool GPIO4PulldownOff;
	bool GPIO3PulldownOff;
	bool GPIO2PulldownOff;
	bool GPIO1PulldownOff;
	bool ReferenceOn;
	bool ADCModeOption;
	uint16_t UndervoltageComparisonVoltage;
	uint16_t OvervoltageComparisonVoltage;
	bool DischargeCell1;
	bool DischargeCell2;
	bool DischargeCell3;
	bool DischargeCell4;
	bool DischargeCell5;
	bool DischargeCell6;
	bool DischargeCell7;
	bool DischargeCell8;
	bool DischargeCell9;
	bool DischargeCell10;
	bool DischargeCell11;
	bool DischargeCell12;
	uint8_t DischargeTimeoutValue;
	uint8_t ADCMode;
	bool DischargePermitted;

} BMSconfigStructTypedef;

void loadConfig(BMSconfigStructTypedef* config);