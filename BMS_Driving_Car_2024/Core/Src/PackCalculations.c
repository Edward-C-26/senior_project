/**
 * Last Edited Spring 2024 : David Lacayo
*/
#include "PackCalculations.h"

//! \brief This method calculates the maximum and minimum cell voltages in the pack, and sets those values w/ the associated
//      cell number to the bms critical info struct
//! \param cfg is the bms configuration file with constants used in our bms system
//! \param bms is the bms struct that contains critical info regarding our pack 
//! \param bmsData is the 2D array that stores the data for each of the cells in our pack
//! \returns none 
void setCriticalVoltages(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, uint8_t bmsData[144][6]) {
    uint16_t maxCellVoltage = 0;
    uint8_t maxCell;
    uint16_t minCellVoltage = cfg.OV_threshold - 1;
    uint8_t minCell;
    uint16_t cellVoltage;
    uint16_t totalPackVoltage;

    int hi = 0;
    totalPackVoltage = 0;

    for(uint8_t cell = 0; cell < NUM_CELLS; cell++) {
        cellVoltage = (uint16_t)(bmsData[cell][2]);
        cellVoltage <<= 8;
        cellVoltage += (uint16_t)(bmsData[cell][3]);

        // Cummulative pack voltage counter
        totalPackVoltage += cellVoltage;
        if (cellVoltage != 65535) {
        	hi++;
        }

        // Check if cell readings is a max voltage or min voltage
        if(cellVoltage > maxCellVoltage && cellVoltage < INVALID_VOLTAGE_UPPER_THRESHOLD) {
            maxCellVoltage = cellVoltage;
            maxCell = cell + 1;
        }

        if(cellVoltage < minCellVoltage && cellVoltage > INVALID_VOLTAGE_LOWER_THRESHOLD) {
            minCellVoltage = cellVoltage;
            minCell = cell + 1;
        }
    }

    // Assign values to the BMS critical info struct 
    bms.curr_max_voltage = maxCellVoltage;
    bms.curr_min_voltage = minCellVoltage;
    bms.max_volt_cell = maxCell;
    bms.min_volt_cell = minCell;
    bms.packVoltage = totalPackVoltage;

}

//! \brief This method calculates the maximum and minimum cell temps in the pack, and sets those values w/ the associated
//      cell number to the bms critical info struct
//! \param cfg is the bms configuration file with constants used in our bms system
//! \param bms is the bms struct that contains critical info regarding our pack 
//! \param bmsData is the 2D array that stores the data for each of the cells in our pack
//! \returns none 
void setCriticalTemps(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, uint8_t bmsData[144][6]) {
    uint16_t maxCellTemp = 0;
    uint8_t maxCell = 180;
    uint16_t minCellTemp = cfg.OT_threshold - 1;
    uint8_t minCell = 180;
    uint16_t cellTemp;

    for(uint8_t cell = 0; cell < NUM_CELLS; cell++) {
        cellTemp = (uint16_t)(bmsData[cell][4]);
		cellTemp <<= 8;
		cellTemp += (uint16_t)(bmsData[cell][5]);

        // Check for cell Temps being max or min 
        if (cellTemp < minCellTemp && cellTemp > 5000) {
			minCellTemp = cellTemp;
			minCell = cell + 1;
		}

		if (cellTemp > maxCellTemp && cellTemp < 75000) {
			maxCellTemp = cellTemp;
			maxCell = cell + 1;
		}

    }

    // Assign values to the BMS critical info struct 
    bms.curr_min_temp = minCellTemp;
    bms.min_temp_cell = minCell;
    bms.curr_max_temp = maxCellTemp;
    bms.max_temp_cell = maxCell;

}

//! \brief This function is still a work in progress
void balance(BMSConfigStructTypedef cfg, BMS_critical_info_t bms, uint8_t bmsData[144][6], bool cellDischarge[12][12], bool fullDischarge[12][12], uint8_t balanceCounter, uint8_t chargeRate) {
	uint16_t maxCellVoltage = bms.curr_max_voltage;
//	uint16_t minCellVoltage = bms.curr_min_voltage;
	uint16_t cellVoltage = 0;
	uint16_t differenceVoltage = 0;

	if ((maxCellVoltage > cfg.balancing_start_threshold)) {  // if cell voltage in range, balance cells
		for (uint8_t cell = 0; cell < NUM_CELLS; cell++) {
			cellVoltage = (uint16_t)(bmsData[cell][2]);	 // bit shifting that will be used in all helper functions
			cellVoltage <<= 8;
			cellVoltage += (uint16_t)(bmsData[cell][3]);

			differenceVoltage = cellVoltage - cfg.balancing_start_threshold;
			if(differenceVoltage < 0) continue;	//For case where cell is not a "Top" (maximum)
			
			/*Unsure if this will work right now-- overall, structure is good, but actual balancing seems weird*/
			if (cellVoltage > cfg.stopCharge_threshold && cellVoltage != 65535) {  // if voltage is above charge threshold(and not a garbage value), turn off charger
				chargeRate = 0;
				fullDischarge[cell / cfg.numOfCellsPerIC][cell % cfg.numOfCellsPerIC] = 1;
				bmsData[cell][1] &= 0x1F;  // set charge rate to 0
			} else if (cell % 12 == balanceCounter && differenceVoltage > cfg.balancing_difference) {
				cellDischarge[cell / cfg.numOfCellsPerIC][cell % cfg.numOfCellsPerIC] = 1;

			} else {
				cellDischarge[cell / cfg.numOfCellsPerIC][cell % cfg.numOfCellsPerIC] = 0;
			}
		}
	}
	
	balanceCounter++;
}
