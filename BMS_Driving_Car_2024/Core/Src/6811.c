/* LTC6811 Drivers
 *	Matt Vlasaty: March 29th, 2019
 *	Aditya Nikumbh: Refactored Jan 31, 2024
 * 	David Lacayo: February, 2024
 *
 *	TODO:
 *		- checkCellConnection return value needs to disregard unused cell inputs
 *		- resolve issue with writeConfig not changing every bit in the first register group
 *		- test functions used to read from every board (readAllCellVoltages should be [12][8])
 *		- minor changes (using more user-defined constants, changing return values)
*/

#include "LTC6811.h"

uint16_t pec15Table[256];	   // Packet Error Code
uint16_t CRC15_POLY = 0x4599;  // Explain magic number por favor -> In datasheet :)

//! \brief Initializes Packet Error Code LUT by generating PEC look up table -> call on startup
//! \returns none
void initPECTable(void) {
    uint16_t remainder;

    for (int i = 0; i < 256; i++) {
        remainder = i << 7;
        for (int bit = 8; bit > 0; bit--) {
            if (remainder & 0x4000) {
                remainder = ((remainder << 1));
                remainder = (remainder ^ CRC15_POLY);
            } else {
                remainder = ((remainder << 1));
            }
        }
        pec15Table[i] = remainder & 0xFFFF;
    }
}

//! \brief This method is used when sending a command to calculate the necessary PEC bytes to follow command bytes. 
//		Should be used when receiving data to compare receieved PEC with expected PEC value
//! \param len is the number of bytes that we will be stepping through in our data 
//! \param data is the data that we will be using to calculate the expected PEC
//! \returns the expected PEC value
uint16_t calculatePEC(uint8_t len, uint8_t *data) {	 // changed to take data by value now that we are not using malloc (ensure no edit)
	uint16_t remainder, address;
	remainder = 16;	 // PEC seed

	for (uint8_t i = 0; i < len; i++) {
		address = ((remainder >> 7) ^ data[i]) & 0xFF;	// calculate PEC table address
		remainder = (remainder << 8) ^ pec15Table[address];
	}

	return (remainder * 2);	 // The CRC15 has a 0 in the LSB so the final value must be multiplied by 2
}

//! \brief this method is used for cellDischarge in order to write configuration datra to the LTC with specified address
//! \param cfg is the configuration struct for BMS constants 
//! \param address is the address used to pass into our config file 
//! \returns none 
void writeConfigAddress(BMSConfigStructTypedef *cfg, uint8_t address) {
	uint8_t config[6];
	uint8_t cmd[12];
	uint16_t PEC_return;
	uint8_t dummy[8];

	config[0] = (uint8_t)((cfg->GPIO5PulldownOff << 7) | (cfg->GPIO4PulldownOff << 6) | (cfg->GPIO3PulldownOff << 5) | (cfg->GPIO2PulldownOff << 4) | (cfg->GPIO1PulldownOff << 3) | (cfg->ReferenceOn << 2) | (cfg->ADCModeOption));
	config[1] = (uint8_t)(cfg->UndervoltageComparisonVoltage & 0xFF);
	config[2] = (uint8_t)(((cfg->OvervoltageComparisonVoltage << 4) & 0xF0) | ((cfg->UndervoltageComparisonVoltage >> 8) & 0x0F));
	config[3] = (uint8_t)((cfg->OvervoltageComparisonVoltage >> 4) & 0xFF);
	config[4] = (uint8_t)((cfg->DischargeCell[8] << 7) | (cfg->DischargeCell[7] << 6) | (cfg->DischargeCell[6] << 5) | (cfg->DischargeCell[5] << 4) | (cfg->DischargeCell[4] << 3) | (cfg->DischargeCell[3] << 2) | (cfg->DischargeCell[2] << 1) | (cfg->DischargeCell[1]));
	config[5] = (uint8_t)(((cfg->DischargeTimeoutValue << 4) & 0xF0) | (cfg->DischargeCell[12] << 3) | (cfg->DischargeCell[11] << 2) | (cfg->DischargeCell[10] << 1) | (cfg->DischargeCell[9]));

	cmd[0] = (uint8_t)((0x80 | ((address << 3) & 0x78) | ((WriteConfigurationRegisterGroup >> 8) & 0x07)));
	cmd[1] = (uint8_t)(WriteConfigurationRegisterGroup & 0xFF);

	PEC_return = calculatePEC(2, cmd);

	cmd[2] = (PEC_return >> 8) & 0xFF;
	cmd[3] = PEC_return & 0xFF;

	memcpy(&cmd[4], config, 6 * sizeof(config[0]));

	PEC_return = calculatePEC(6, cmd + 4);

	cmd[10] = (PEC_return >> 8) & 0xFF;
	cmd[11] = PEC_return & 0xFF;

	SPIWrite(cmd, sizeof(cmd));

	readConfig(address, dummy);
}

//! \brief This function is called every loop to accommodate dischargeCells method 
//		Specifically writes configuration data (UV, OV, ADCMode, etc) to BMSConfig struct
//! \param cfg is the configuration files for the constants in the BMS system
//! \returns none
void writeConfigAll(BMSConfigStructTypedef *cfg) {
	wakeup_idle();

	for (uint8_t i = 0; i < cfg->numOfICs; i++) {
		writeConfigAddress(cfg, cfg->address[i]);
	}
}

//! \brief This function sends an ADCV command that begins conversion for every cell to specified LTC. 
//		This results in the function readings all cell voltage registers using the readRegister function. 
//! \param address is the address of the board that will be read 
//! \param cellVoltage is the array that will store the voltages of all the cells on the board being read
//! \returns true if PEC matches expected PEC value, else false 
bool readCellVoltage(uint8_t address, uint16_t cellVoltage[12]) {
	bool PEC_check = false;
	bool dataValid = true;
	uint16_t voltage[12];


	PEC_check = readRegister(ReadCellVoltageRegisterGroup1to3, address, voltage);
	dataValid = dataValid & PEC_check;

	PEC_check = readRegister(ReadCellVoltageRegisterGroup4to6, address, voltage);
	dataValid = dataValid & PEC_check;

	PEC_check = readRegister(ReadCellVoltageRegisterGroup7to9, address, voltage);
	dataValid = dataValid & PEC_check;

	PEC_check = readRegister(ReadCellVoltageRegisterGroup10to12, address, voltage);
	dataValid = dataValid & PEC_check;

	memcpy(cellVoltage, voltage, sizeof(voltage));

	return dataValid;
}

//! \brief This function reads all cell voltages by essentially parsing each board and reading the individual cell voltages. These are then 
//		stored in the bmsData array, along with the cell number that is associated with the reading. 
//! \param cfg is the BMS configuration struct with constants 
//! \param bmsData is the 2D array that stores all cell data for voltages, temperatures, faults, and cell number 
//! \returns true if no PEC for any register read for any board 
bool readAllCellVoltages(BMSConfigStructTypedef cfg, uint8_t bmsData[144][6]) {
	uint16_t boardVoltage[12];
	bool PEC_check[12];
	bool dataValid = true;
	uint8_t flag = 0;

	wakeup_idle();

	sendBroadcastCommand(ClearRegisters);
	sendBroadcastCommand(StartCellVoltageADCConversionAll);
	HAL_Delay(20);

	wakeup_idle();

	for (uint8_t board = 0; board < NUM_BOARDS; board++) {
		// read voltage of every cell input (1-12) for a specific address, store in boardVoltage
		PEC_check[board] = readCellVoltage(board, boardVoltage);
		dataValid &= PEC_check[board];

		// store cell number and valid data bit in bmsData
		for (uint8_t cell = 0; cell < NUM_BOARDS; cell++) {
			bmsData[(board * NUM_BOARDS) + cell][0] = (uint8_t)((board * NUM_BOARDS) + cell + 1);  // cell number
			bmsData[(board * NUM_BOARDS) + cell][1] = 0;										   // clear status byte

			if (PEC_check[board])
				bmsData[(board * NUM_BOARDS) + cell][1] |= 0b00000010;	// set valid data bit

			bmsData[(board * NUM_BOARDS) + cell][2] = (uint8_t)((boardVoltage[cell] >> 8) & 0xFF);
			bmsData[(board * NUM_BOARDS) + cell][3] = (uint8_t)(boardVoltage[cell] & 0xFF);
		}

		if (board == 11) {
			flag++;
		}
	}

	return dataValid;  // return true if no PEC errors for any board
}

//! \brief This function initiates ADC conversion for GPIO inputs connected to temperature sensors. 
//		Reads auxiliary register groups using readRegister function. Then, converts measured voltage into temperature 
//			based on temperature sensor response. Also checks for disconnected temperature sensor and OT faults. 
//				NOTE: We only read 4 temps per board, and they are the 4 highest temps on each board.
//! \param address is the address of the board being read
//! \param cellTemp is the array that will hold the cell temps read during readRegister
//! \param dcFault is the array that stores temperature sensor ceonnection for all cells read 
//! \param tempFault stores OT fault info for each cell read 
//! \returns true if PEC value read matches expected PEC value calculated. Otherwise, false. 
bool readCellTemp(uint8_t address, uint16_t cellTemp[4], bool dcFault[4], bool tempFault[4]) {
	bool PEC_check = false;
	bool dataValid = true;
	uint16_t temp[4];
	double realTemp[4];

	for (uint8_t i = 0; i < 4; i++) {
		temp[i] = 0;
		realTemp[i] = 0.0;
	}

	PEC_check = readRegister(ReadAuxiliaryGroupA, address, temp);
	dataValid = dataValid & PEC_check;

	PEC_check = readRegister(ReadAuxiliaryGroupB, address, temp);
	dataValid = dataValid & PEC_check;

	for (uint8_t i = 0; i < 4; i++) {
		realTemp[i] = (double)temp[i] / 10000;	// Divide by 10,000 b/c units in V x10^-4 from ADC reading
		// Convert millivolts to volts -> use approximation calculation -> convert to celcius -> multiply by 1000 to convert to integer without losing the end of the reading
		realTemp[i] = 1000 * (-0.5022 * pow(realTemp[i], 5) + 6.665 * pow(realTemp[i], 4) - 35.123 * pow(realTemp[i], 3) + 92.559 * pow(realTemp[i], 2) - 144.22 * realTemp[i] + 166.76);

		cellTemp[i] = (uint16_t)realTemp[i];

		dcFault[i] = ((cellTemp[i] < -(20 * 1000)) || ((125 * 1000) < cellTemp[i])) ? true : false;
		tempFault[i] = ((cellTemp[i] < (0 * 1000)) || ((60 * 1000) < cellTemp[i])) ? true : false;
	}

	return (dataValid);
}

//! \brief This function reads cell temps from all of our board by calling readCellTemp for each board. NOTE : This is not proven to work properly yet.
//! \param cfg is the configuration struct for BMS with all of the constants used 
//! \param bmsData is the 2D array where we track the data of each cell in our pack 
//! \returns true if PEC value received matches expected PEC value. Otherwise, false. -> i.e., returns false if any readCellTemp return values are false. 
bool readAllCellTemps(BMSConfigStructTypedef cfg, uint8_t bmsData[144][6]) {
	uint16_t boardTemp[4];
	bool boardDCFault[4];
	bool boardTempFault[4];
	bool PEC_check[12];
	bool dataValid = true;

	wakeup_idle();
	HAL_Delay(2);

	sendBroadcastCommand(ClearRegisters);
	sendBroadcastCommand(StartCellTempVoltageADCConversionAll);
	HAL_Delay(20);

	wakeup_idle();
	HAL_Delay(2);

	for (uint8_t board = 0; board < NUM_BOARDS; board++) {
		// read temperature, check for OT and temp DC
		PEC_check[board] = readCellTemp(board, boardTemp, boardDCFault, boardTempFault);
		dataValid &= PEC_check[board];

		// store OT and temp DC bits in status byte
		for (uint8_t cell = 0; cell < 12; cell++) {
			if (PEC_check[board])
				bmsData[(board * 12) + cell][1] |= 0b00000010;	// set valid data bit

			if (boardTempFault[cell / 3])
				bmsData[(board * 12) + cell][1] |= 0b00010000;	// set OT bit

			if (boardDCFault[cell / 3])
				bmsData[(board * 12) + cell][1] |= 0b00001000;	// set temp DC bit

			bmsData[(board * 12) + cell][4] = (uint8_t)((boardTemp[cell / 3] >> 8) & 0xFF);
			bmsData[(board * 12) + cell][5] = (uint8_t)(boardTemp[cell / 3] & 0xFF);
		}
	}

	return dataValid;
}

//! \brief This method uses general readRegister function to check current state of LTC configuration reg. 
//		Mostly used for testing purposes
//! \param address is the address passed into the readRegister function
//! \param cfg is part of the configuration that will be passed to check the current state of the LTC
//! \returns true if the received PEC from readRegister mathes the calculated PEC 
bool readConfig(uint8_t address, uint8_t cfg[8]) {
	uint16_t config[4];
	bool dataValid = false;

	wakeup_idle();

	dataValid = readRegister(ReadConfigurationRegisterGroup, address, config);

	cfg[0] = (uint8_t)((config[0] >> 8) & 0xFF);
	cfg[1] = (uint8_t)(config[0] & 0xFF);
	cfg[2] = (uint8_t)((config[1] >> 8) & 0xFF);
	cfg[3] = (uint8_t)(config[1] & 0xFF);
	cfg[4] = (uint8_t)((config[2] >> 8) & 0xFF);
	cfg[5] = (uint8_t)(config[2] & 0xFF);
	cfg[6] = (uint8_t)((config[3] >> 8) & 0xFF);
	cfg[7] = (uint8_t)(config[3] & 0xFF);

	return dataValid;
}

//! \brief This function checks the cell connections of each cell in the BMS data array.
//		Currently not used, as we have not been able to get it working, but the whole idea is that 
//			this function compares previously measured values to open wire check values, and if 
//				there is a significant drop in voltage, cell is allegedly disconnected. 
//! \param cfg is the BMS configuration struct with constants 
//! \param bmsData is a 2D array of 144 cells x 6 readings that stores our cell data throughout the system
//! \returns true is the cell is connnected properly, and false if the cell is "disconnected"
bool checkAllCellConnections(BMSConfigStructTypedef cfg, uint8_t bmsData[144][6]) {
	uint16_t ADOWvoltage[cfg.numOfCellInputs];
	uint16_t cellVoltage;
	bool PEC_check[12];
	bool dataValid = true;

	wakeup_idle();

	// at least 2
	// por que los dos
	sendBroadcastCommand(ClearRegisters);
	sendBroadcastCommand(StartOpenWireConversionPulldown);
	sendBroadcastCommand(StartOpenWireConversionPulldown);
	sendBroadcastCommand(StartOpenWireConversionPulldown);
	sendBroadcastCommand(StartOpenWireConversionPulldown);
	sendBroadcastCommand(StartOpenWireConversionPulldown);
	HAL_Delay(20);

	wakeup_idle();

	for (uint8_t board = 0; board < cfg.numOfICs; board++) {
		PEC_check[board] = readCellVoltage(cfg.address[board], ADOWvoltage);
		dataValid &= PEC_check[board];

		// I'm not touching this but wtf
		ADOWvoltage[4] = ADOWvoltage[6];
		ADOWvoltage[5] = ADOWvoltage[7];
		ADOWvoltage[6] = ADOWvoltage[8];
		ADOWvoltage[7] = ADOWvoltage[9];

		for (uint8_t cell = 0; cell < cfg.numOfCellsPerIC; cell++) {
			cellVoltage = (uint16_t)(bmsData[(board * cfg.numOfCellsPerIC) + cell][2]);
			cellVoltage <<= 8;
			cellVoltage += (uint16_t)(bmsData[(board * cfg.numOfCellsPerIC) + cell][3]);

			if ((cellVoltage - ADOWvoltage[cell]) < 1000) {
				bmsData[(board * cfg.numOfCellsPerIC) + cell][1] |= 0b00000001;	 // negligible drop in voltage, so cell is connected
			}
		}
	}

	return dataValid;
}

//! \brief This function writes the configuration struct of cells that are currently being discharged. This is to track which cells are //		being discharged while running our balancing algorithm. 
//! \param cfg is the configuration struct that stores all the constants in our BMS algorithm 
//! \param cellDischarge is a 2D array of cells that tracks which cells are being discharged or not : 1 = discharging, 0 = not discharging 
//! returns 0 always -> not sure why we need a return value on this 
bool dischargeCellGroups(BMSConfigStructTypedef *cfg, bool cellDischarge[12][12]) {
	wakeup_idle();

	for (uint8_t i = 0; i < cfg->numOfICs; i++) {
		for (uint8_t j = 0; j < 12; j++) {
			cfg->DischargeCell[j] = cellDischarge[i][j];
		}

		writeConfigAddress(cfg, cfg->address[i]);
	}

	for (uint8_t i = 0; i < 12; i++)
		cfg->DischargeCell[i] = 0;

	return 0;
}

//! \brief This function is used to wakeup the LTC chip that we want to use to get readings from 
void wakeup_idle() {
	uint32_t delay = 15;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	while (delay--)
		;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

//! \brief Reads register specified by command from specified board address 
//! \param command is the command that we will send to the board
//! \param address is the specific board that will get the command
//! \param data is where the data read from the specific board will be StartOpenWireConversionPulldown
//! \returns true if the data is valid, otherwise false (i.e., received PEC matches expected PEC value)
bool readRegister(CommandCodeTypedef command, uint8_t address, uint16_t *data) {  // changed to take array by reference now that malloc is removed
	uint8_t cmd[12];
	uint8_t rx_data[12];
	uint16_t PEC_return;
	uint8_t PEC_send[6];
	bool dataValid = true;

	PEC_send[0] = (uint8_t)(0x80 | ((address << 3) & 0x78) | ((command >> 8) & 0x07));
	PEC_send[1] = (uint8_t)(command & 0xFF);

	cmd[0] = PEC_send[0];
	cmd[1] = PEC_send[1];

	PEC_return = calculatePEC(2, PEC_send);

	cmd[2] = (PEC_return >> 8) & 0xFF;
	cmd[3] = PEC_return & 0xFF;

	cmd[4] = 0;
	cmd[5] = 0;
	cmd[6] = 0;
	cmd[7] = 0;
	cmd[8] = 0;
	cmd[9] = 0;
	cmd[10] = 0;
	cmd[11] = 0;

	SPIWriteRead(cmd, rx_data, sizeof(cmd));  // send 4 command bytes, receive 6 cell voltage bytes (4-9) and 2 PEC bytes (10-11)

	// calculate PEC based on cell voltage data received
	PEC_send[0] = rx_data[4];  // cell 1 voltage low bytes
	PEC_send[1] = rx_data[5];  // cell 1 voltage high bytes
	PEC_send[2] = rx_data[6];  // cell 2 voltage low bytes
	PEC_send[3] = rx_data[7];  // cell 2 voltage high bytes
	PEC_send[4] = rx_data[8];  // cell 3 voltage low bytes
	PEC_send[5] = rx_data[9];  // cell 3 voltage high bytes

	PEC_return = calculatePEC(6, PEC_send);

	// check if received PEC matches calculated PEC
	if (PEC_return != (((rx_data[10] << 8) & 0xFF00) | (rx_data[11] & 0x00FF))) {
		dataValid = false;
	}

	if (command == ReadCellVoltageRegisterGroup1to3) {
		data[0] = (uint16_t)((rx_data[5] << 8) & 0xFF00) | (rx_data[4] & 0x00FF);
		data[1] = (uint16_t)((rx_data[7] << 8) & 0xFF00) | (rx_data[6] & 0x00FF);
		data[2] = (uint16_t)((rx_data[9] << 8) & 0xFF00) | (rx_data[8] & 0x00FF);
		// data[3] = (uint16_t) ((rx_data[10] << 8) & 0xFF00) | (rx_data[11] & 0x00FF);
		// data[4] = PEC_return;
	}

	if (command == ReadCellVoltageRegisterGroup4to6) {
		data[3] = (uint16_t)((rx_data[5] << 8) & 0xFF00) | (rx_data[4] & 0x00FF);
		data[4] = (uint16_t)((rx_data[7] << 8) & 0xFF00) | (rx_data[6] & 0x00FF);
		data[5] = (uint16_t)((rx_data[9] << 8) & 0xFF00) | (rx_data[8] & 0x00FF);
	}

	if (command == ReadCellVoltageRegisterGroup7to9) {
		data[6] = (uint16_t)((rx_data[5] << 8) & 0xFF00) | (rx_data[4] & 0x00FF);
		data[7] = (uint16_t)((rx_data[7] << 8) & 0xFF00) | (rx_data[6] & 0x00FF);
		data[8] = (uint16_t)((rx_data[9] << 8) & 0xFF00) | (rx_data[8] & 0x00FF);
	}

	if (command == ReadCellVoltageRegisterGroup10to12) {
		data[9] = (uint16_t)((rx_data[5] << 8) & 0xFF00) | (rx_data[4] & 0x00FF);
		data[10] = (uint16_t)((rx_data[7] << 8) & 0xFF00) | (rx_data[6] & 0x00FF);
		data[11] = (uint16_t)((rx_data[9] << 8) & 0xFF00) | (rx_data[8] & 0x00FF);
	}

	if (command == ReadAuxiliaryGroupA) {
		data[0] = (uint16_t)((rx_data[5] << 8) & 0xFF00) | (rx_data[4] & 0x00FF);
		data[1] = (uint16_t)((rx_data[7] << 8) & 0xFF00) | (rx_data[6] & 0x00FF);
		data[2] = (uint16_t)((rx_data[9] << 8) & 0xFF00) | (rx_data[8] & 0x00FF);
	}

	if (command == ReadAuxiliaryGroupB) {
		data[3] = (uint16_t)((rx_data[5] << 8) & 0xFF00) | (rx_data[4] & 0x00FF);
	}

	if (command == ReadConfigurationRegisterGroup) {
		data[0] = (uint16_t)((rx_data[4] << 8) & 0xFF00) | (rx_data[5] & 0x00FF);
		data[1] = (uint16_t)((rx_data[6] << 8) & 0xFF00) | (rx_data[7] & 0x00FF);
		data[2] = (uint16_t)((rx_data[8] << 8) & 0xFF00) | (rx_data[9] & 0x00FF);
		data[3] = (uint16_t)((rx_data[10] << 8) & 0xFF00) | (rx_data[11] & 0x00FF);
	}

	return (dataValid);
}

//! \brief Sends specified write only command to every LTC in the chain (ex: ADCV)
//! \param command contains every command code used 
//! \returns none 
void sendBroadcastCommand(CommandCodeTypedef command) {
	uint8_t cmd[4];
	uint16_t PEC_return;

	cmd[0] = (uint8_t)((command >> 8) & 0x0F);
	cmd[1] = (uint8_t)(command & 0xFF);

	PEC_return = calculatePEC(2, (uint8_t *)&(cmd));

	cmd[2] = (PEC_return >> 8) & 0xFF;
	cmd[3] = PEC_return & 0xFF;

	SPIWrite(cmd, 4);
}

//! \brief Sends specified write-only command to LTC with the specified address 
//! \param command is the command that will be sent to the LTC 
//! \param address is the specified address of the LTC that will receive the command
//! \returns none 
void sendAddressCommand(CommandCodeTypedef command, uint8_t address) {
	uint8_t cmd[4];
	uint16_t PEC_return;
	uint8_t msbytes[2];

	msbytes[0] = (uint8_t)(0x80 | ((address << 3) & 0x78) | ((command >> 8) & 0x07));
	msbytes[1] = (uint8_t)(command & 0xFF);

	cmd[0] = msbytes[0];
	cmd[1] = msbytes[1];

	PEC_return = calculatePEC(2, msbytes);

	cmd[2] = (PEC_return >> 8) & 0xFF;
	cmd[3] = PEC_return & 0xFF;

	SPIWrite(cmd, 4);
}
