#ifndef PACK_CALCULATIONS
#define PACK_CALCULATIONS

#include "BMSconfig.h"
#include "LTC6811.h"
// #include "LTC2949.h"
#include "Fault.h"

#define MAXINT16 65535

void setCriticalVoltages(BMSConfigStructTypedef *cfg, BMS_critical_info_t *bms, CellData bmsData[144]);
void setCriticalTemps(BMSConfigStructTypedef *cfg, BMS_critical_info_t *bms, CellData bmsData[144]);
void balance(BMSConfigStructTypedef *cfg, BMS_critical_info_t *bms, CellData bmsData[144], bool cellDischarge[12][12], bool fullDischarge[12][12], uint8_t balanceCounter, uint8_t chargeRate);

#endif  // PACK_CALCULATIONS
