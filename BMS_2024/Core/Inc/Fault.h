#ifndef FAULT_H
#define FAULT_H

// Need to account for this is main or when we get 2949 data so we don't have to worry here
#define INVALID_VOLTAGE_UPPER_THRESHOLD   45000
#define INVALID_VOLTAGE_LOWER_THRESHOLD   25000

#include "BMSconfig.h"

typedef struct {
    // 6811 readings
    uint16_t curr_max_voltage;
    uint8_t max_volt_cell;
    uint16_t curr_min_voltage;
    uint8_t min_volt_cell;

    uint16_t curr_max_temp;
    uint8_t max_temp_cell;
    uint16_t curr_min_temp;
    uint8_t min_temp_cell;

    // 2949 readings
    uint16_t max_power;
    uint16_t min_power;
    uint16_t max_voltage_2949;
    uint16_t min_voltage_2949;
    uint16_t max_curr_2949;
    uint16_t min_curr_2949;

    uint16_t packCurrent;
    uint16_t packVoltage;
    uint16_t packPower;
    uint16_t packCharge;
    uint16_t packEnergy;

    // additional readings
    bool invalid_data;
    uint8_t invalid_data_cell;

    bool cell_connection_fault;
    uint8_t cell_connection_num;

    bool is_fault;
    uint8_t fault_board_num;

} BMS_critical_info_t;

void init_BMS_info(BMS_critical_info_t * bms_struct, BMSConfigStructTypedef * cfg);
bool FAULT_check(BMSConfigStructTypedef *cfg, BMS_critical_info_t *bms_struct, CellData bmsData[144], uint8_t bmsStatus[6]);

#endif //FAULT_H
