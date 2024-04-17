#ifndef PACK_CALCULATIONS
#define PACK_CALCULATIONS

#include "BMSconfig.h"
#include "LTC6811.h"
// #include "LTC2949.h"
#include "Fault.h"

void setCriticalVoltages(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, uint8_t bmsData[144][6]);
void setCriticalTemps(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, uint8_t bmsData[144][6]);
void balance(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, uint8_t bmsData[144][6], bool cellDischarge[12][12], bool fullDischarge[12][12], uint8_t balanceCounter, uint8_t chargeRate);

#endif  // PACK_CALCULATIONS