#ifndef PACK_CALCULATIONS
#define PACK_CALCULATIONS

#include "BMSconfig.h"
#include "LTC6811.h"
// #include "LTC2949.h"
#include "Fault.h"

#define MAXINT16 65535

extern uint8_t balance_counter;
void setCriticalVoltages(BMS_critical_info_t *bms,
        CellData const bmsData[144]);
void setCriticalTemps(BMS_critical_info_t *bms, CellData const bmsData[144]);
void balance(BMSConfigStructTypedef const *cfg, BMS_critical_info_t *bms,
        CellData bmsData[144], bool cellDischarge[12][12],
        bool fullDischarge[12][12], uint8_t balanceCounter,
        uint8_t *chargeRate);
void thresholdBalance(BMSConfigStructTypedef *cfg, BMS_critical_info_t *bms,
        CellData bmsData[144], bool cell_discharge[12][12],
        uint16_t cell_discharge_threshold,
        uint8_t num_cells_discharge_per_secondary);

#endif  // PACK_CALCULATIONS
