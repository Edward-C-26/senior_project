#include "Fault.h"

//! \brief This method initializes the BMS info struct, which contains critical info regarding maxs and mins for 
// readings in our battery pack
//! \param cfg is the configuration data used for BMS system
//! \param bms_struct is the struct that will be filled with critical values 
//! \returns None
void init_BMS_info(BMS_critical_info_t * bms_struct, BMSConfigStructTypedef * cfg) {

    bms_struct->curr_max_voltage = cfg->UV_threshold + 1;   // assings min voltage threshold for init
    bms_struct->max_volt_cell = -1;
    bms_struct->curr_min_voltage = cfg->OV_threshold - 1;   //asigns max voltage threshold for init
    bms_struct->min_volt_cell = -1;

    bms_struct->curr_max_temp = cfg->UT_threshold + 1;
    bms_struct->max_temp_cell = -1;
    bms_struct->curr_min_temp = cfg->OT_threshold - 1;        // assigns really hot temp for init
    bms_struct->min_temp_cell = -1;

    bms_struct->invalid_data = false;
    bms_struct->invalid_data_cell = -1;

    bms_struct->cell_connection_fault = false;
    bms_struct->cell_connection_num = -1;

    bms_struct->is_fault = false;
    bms_struct->fault_board_num = -1;
    
}


//! \brief This method checks for valid data as well as cell connection, OT, UT, OV, & UT faults
//! \param cfg is the configuration struct for constants and readings
//! \param bmsData is a 2D array of 144 cells, each with 6 bytes of info (partaining to cell #, fault, voltage & temp) 
//! \param bmsStatus is an array that keeps track of BMS Fault information that will be returned over CAN
//! \returns true if there is a BMS fault, and false if the system has returned no faults
bool FAULT_check(BMSConfigStructTypedef cfg, BMS_critical_info_t bms_struct, uint8_t bmsData[144][6], uint8_t bmsStatus[6]) {
    bool BMS_fault = false;
//    bool OT_fault = false;
//    bool UT_fault = false;
//    bool OV_fault = false;
//    bool UV_fault = false;


    // Clear out status bytes (protects against reset)
	bmsStatus[0] = 0;
	bmsStatus[1] = 0;
	bmsStatus[2] = 0;
	bmsStatus[3] = 0;
	bmsStatus[4] = 0;
	bmsStatus[5] = 0;

    // Overvolt Fault Check
    if(bms_struct.curr_max_voltage > cfg.OV_threshold && bms_struct.curr_max_voltage < INVALID_VOLTAGE_UPPER_THRESHOLD) {
        BMS_fault = true;
        bmsStatus[0] |= 0x01;       // Fault byte
        bmsStatus[1] = bms_struct.max_volt_cell;    // NOT zero indexed -> stored as cell # + 1
    }

    // Undervolt Fault Check
    if(bms_struct.curr_min_voltage < cfg.UV_threshold && bms_struct.curr_min_voltage < INVALID_VOLTAGE_LOWER_THRESHOLD) {
        BMS_fault = true;
        bmsStatus[0] |= 0x02;       // Fault byte
        bmsStatus[2] = bms_struct.min_volt_cell;    // NOT zero indexed -> stored as cell # + 1
    }

    // Overtemp Fault Check
    if(bms_struct.curr_max_temp > cfg.OT_threshold) {
        BMS_fault = true;
        bmsStatus[0] |= 0x04;       // Fault byte
        bmsStatus[3] = bms_struct.max_temp_cell;    // NOT zero indexed -> stored as cell # + 1
    }

    // Undertemp Fault Check
    if(bms_struct.curr_min_temp < cfg.UT_threshold) {
        BMS_fault = true;
        bmsStatus[0] = 0x08;    // Fault byte
        bmsStatus[4] = bms_struct.min_temp_cell;    // NOT zero indexed -> stored as cell # + 1
    }

    // Invalid data check -> don't throw fault, because well... the data is just invalid
    if(bms_struct.curr_max_voltage > INVALID_VOLTAGE_UPPER_THRESHOLD) {
            bms_struct.invalid_data = true;
            bms_struct.invalid_data_cell = bms_struct.max_volt_cell;
    } else if(bms_struct.curr_min_voltage < INVALID_VOLTAGE_LOWER_THRESHOLD) {
        bms_struct.invalid_data = true;
        bms_struct.invalid_data_cell = bms_struct.min_volt_cell;
    } else {
        bms_struct.invalid_data = false;
    }

    // Set the status of fault (true | false) in our bms critical info struct
    bms_struct.is_fault = BMS_fault;

    return BMS_fault;
}
