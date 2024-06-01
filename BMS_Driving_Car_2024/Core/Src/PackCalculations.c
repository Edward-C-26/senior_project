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
void setCriticalVoltages(BMSConfigStructTypedef *cfg, BMS_critical_info_t *bms, uint8_t bmsData[144][6]) {
    uint16_t maxCellVoltage;
    uint16_t minCellVoltage;
    uint16_t cellVoltage;

    uint16_t totalVoltage;


	bms->curr_min_voltage = MAXINT16;
	bms->curr_max_voltage = 0;

    for(uint8_t cell = 0; cell < NUM_CELLS; cell++) {
    	//First two is due to fucky cell voltages, unsure about 0 -> for sume reason cellVoltage goes to 1027?

    	cellVoltage = (((uint16_t)bmsData[cell][2]) << 8 | (uint16_t)bmsData[cell][3]);

    	totalVoltage += cellVoltage;

    	maxCellVoltage = bms->curr_max_voltage;

        // Check if cell readings is a max voltage or min voltage
        if(cellVoltage > maxCellVoltage && cellVoltage < INVALID_VOLTAGE_UPPER_THRESHOLD) {
            bms->curr_max_voltage = cellVoltage;
			bms->max_volt_cell = cell;
        }

        minCellVoltage = bms->curr_min_voltage;

        if((cellVoltage < minCellVoltage || minCellVoltage == 0) && cellVoltage > INVALID_VOLTAGE_LOWER_THRESHOLD) {
            bms->curr_min_voltage = cellVoltage;
			bms->min_volt_cell = cell;
        }

        bms->packVoltage = totalVoltage;
        }
    }


//! \brief This method calculates the maximum and minimum cell temps in the pack, and sets those values w/ the associated
//      cell number to the bms critical info struct
//! \param cfg is the bms configuration file with constants used in our bms system
//! \param bms is the bms struct that contains critical info regarding our pack 
//! \param bmsData is the 2D array that stores the data for each of the cells in our pack
//! \returns none 
void setCriticalTemps(BMSConfigStructTypedef *cfg, BMS_critical_info_t *bms, uint8_t bmsData[144][6]) {
	 	uint16_t maxCellTemp;
	    uint16_t minCellTemp;
	    uint16_t cellTemp;

		bms->curr_min_temp = MAXINT16;
		bms->curr_max_temp = 0;
	
	    for(uint8_t cell = 0; cell < NUM_CELLS; cell++) {
	    	cellTemp = (((uint16_t)bmsData[cell][4]) << 8 | (uint16_t)bmsData[cell][5]);
	    	maxCellTemp = bms->curr_max_temp;
	    	if(cellTemp > maxCellTemp){
	    	   bms->curr_max_temp = cellTemp;
	    	   bms->max_temp_cell = cell;
	    	}
	        minCellTemp = bms->curr_min_temp;

	        if(cellTemp < minCellTemp || minCellTemp == 0) {
	            bms->curr_min_temp = cellTemp;
	            bms->min_temp_cell = cell;
	        }
	    }

}

//! \brief This function is still a work in progress
void balance(BMSConfigStructTypedef *cfg, BMS_critical_info_t *bms, uint8_t bmsData[144][6], bool cellDischarge[12][12], bool fullDischarge[12][12], uint8_t balanceCounter, uint8_t chargeRate) {
	uint16_t maxCellVoltage = bms->curr_max_voltage;
//	uint16_t minCellVoltage = bms.curr_min_voltage;
	uint16_t cellVoltage = 0;
	uint16_t differenceVoltage = 0;

	if ((maxCellVoltage > cfg->balancing_start_threshold)) {  // if cell voltage in range, balance cells
		for (uint8_t cell = 0; cell < NUM_CELLS; cell++) {

			cellVoltage = (((uint16_t)bmsData[cell][2]) << 8 | (uint16_t)bmsData[cell][3]);

			differenceVoltage = cellVoltage - cfg->balancing_start_threshold;
			if(differenceVoltage < 0) continue;	//For case where cell is not a "Top" (maximum)
			
			/*Unsure if this will work right now-- overall, structure is good, but actual balancing seems weird*/
			if (cellVoltage > cfg->stopCharge_threshold && cellVoltage != 65535) {  // if voltage is above charge threshold(and not a garbage value), turn off charger
				chargeRate = 0;
				fullDischarge[cell / cfg->numOfCellsPerIC][cell % cfg->numOfCellsPerIC] = 1;
				bmsData[cell][1] &= 0x1F;  // set charge rate to 0
			} else if (cell % 12 == balanceCounter && differenceVoltage > cfg->balancing_difference) {
				cellDischarge[cell / cfg->numOfCellsPerIC][cell % cfg->numOfCellsPerIC] = 1;

			} else {
				cellDischarge[cell / cfg->numOfCellsPerIC][cell % cfg->numOfCellsPerIC] = 0;
			}
		}
	}
	
	balanceCounter++;
}
