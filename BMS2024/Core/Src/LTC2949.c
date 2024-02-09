/*
LTC 2949 Command Functions
BMS 2024
Curtis Lam
Sidarth Raman

Chunks of this code are recycled from LTC6811.c
    
    readMaxVoltage: bool readMaxVoltage()
        - gets values from readRegister2949
        - converts to unit value
        - stores value in variables within .h file
        - returns true if received PEC matches calculated PEC for every register read

    readMinVoltage: bool readMinVoltage()
        - gets values from readRegister2949
        - converts to unit value
        - stores value in variables within .h file
        - returns true if received PEC matches calculated PEC for every register read

    note to look at Register Control Register 0xFF that selects memory page (page 0 vs page 1)
        - will need to set threshold registers
        - add software reset function after faults go off and are cleared



Fault Functions (unnecessary as of now)
    detectFaults: int detectFaults() 
        - checks for faults based on bits on the registers 
        - 1 means fault in the byte
        - returns a boolean array corresponding to a specific fault (1 means fault)

Wakeup Function
    wakeup_idle: void wakeup_idle()
        - wakes up 2949 to transition from IDLE state to READY
        - create differential activity by sending data to one of the pins (chip select pin)
        - write 0x00 to register 0x70

PEC Functions (taken from 2949 datasheet)
    initPECTable: void initPECTable(void);
        - taken from 2949 datasheet
        - generates PEC look-up table
        - should be called on start-up
    calculatePEC: uint16_t calculatePEC(uint8_t len, uint8_t *data);
        - taken from 2949 datasheet
        - used when sending command to calculate necessary PEC bytes to follow command bytes
        - used when receiving data to compare received PEC with the PEC that should have been received based on data
        - returns uint16_t PEC value



*/

// address is 0xF or 1111

#include "LTC2949.h"
#include "LTC6811.h"
#include "Fault.h"
#include <math.h>

// uint16_t 2949_pec15Table[256];
// uint16_t 2949_CRC15_POLY = 0x4599;

// void init_2949_PEC15_Table() // initialize Packet Error Checking Table
// {
//     uint16_t remainder;
//     for (int i = 0; i < 256; i++)
//     {
//         remainder = i << 7;
//         for (int bit = 8; bit > 0; --bit)
//         {
//             if (remainder & 0x4000)
//             {
//                 remainder = ((remainder << 1));
//                 remainder = (remainder ^ CRC15_POLY);
//             }
//             else
//             {
//                 remainder = ((remainder << 1));
//             }
//         }
//         pec15Table[i] = remainder & 0xFFFF;
//     }
// }


//uint16_t 2949_calculatePEC(uint8_t len, uint8_t *data) // calculate Packet Error Checking value
//{
//    uint16_t remainder, address;
//    remainder = 16; // PEC seed
//    for (int i = 0; i < len; i++)
//    {
//        address = ((remainder >> 7) ^ data[i]) & 0xff; // calculate PEC table address
//        remainder = (remainder << 8) ^ pec15Table[address];
//    }
//    return (remainder * 2); // The CRC15 has a 0 in the LSB so the final value must be multiplied by 2
//}

//void 2949_wakeup_idle() { // pin PA4 (chip select pin on 2949)
//	uint32_t delay = 15;
//	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
//	while (delay--);
//	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
//}

// void writeConfig(BMSconfigStructTypedef cfg) { // modify this to fit 2949
//     uint8_t config[6];
// 	uint8_t cmd_len = 12;
//     uint8_t cmd[cmd_len] = {};
// 	uint16_t PEC_return;

//     wakeup_idle();

//     // go to page 1 of memory map for configurations

//     cmd[0] = (uint8_t)(DirectCommand & 0xFF);
//     cmd[1] = (uint8_t)(ControlRegister & 0xFF)
    
//     PEC_return = calculatePEC(2, cmd);

// 	cmd[2] = (PEC_return >> 8) & 0xFF;
// 	cmd[3] = PEC_return & 0xFF;
//     cmd[4] = (uint8_t)(Write1Byte & 0xFF) // tell the bus that you are writing 1 byte (ask jack and david if the & 0xFF is necessary)
    
//     cmd[5] = (uint8_t)(FlipPage1 & 0xFF) // select page 1

//     PEC_return = calculatePEC(1, cmd[5]);

//     cmd[6] = (PEC_return >> 8) & 0xFF;
// 	cmd[7] = PEC_return & 0xFF;

//     SPIWrite(cmd, cmd_len);

//     // page flipped, now for the configuration


// 	// config[0] = (uint8_t)(cfg.GPIO5PulldownOff << 7) | (cfg.GPIO4PulldownOff << 6) | (cfg.GPIO3PulldownOff << 5) | (cfg.GPIO2PulldownOff << 4) | (cfg.GPIO1PulldownOff << 3) | (cfg.ReferenceOn << 2) | (cfg.ADCModeOption);
// 	// config[1] = (uint8_t)(cfg.UndervoltageComparisonVoltage & 0xFF);
// 	// config[2] = (uint8_t)((cfg.OvervoltageComparisonVoltage << 4) & 0xF0) | ((cfg.UndervoltageComparisonVoltage >> 8) & 0x0F);
// 	// config[3] = (uint8_t)((cfg.OvervoltageComparisonVoltage >> 4) & 0xFF);
// 	// config[4] = (uint8_t)(cfg.DischargeCell8 << 7) | (cfg.DischargeCell7 << 6) | (cfg.DischargeCell6 << 5) | (cfg.DischargeCell5 << 4) | (cfg.DischargeCell4 << 3) | (cfg.DischargeCell3 << 2) | (cfg.DischargeCell2 << 1) | (cfg.DischargeCell1);
// 	// config[5] = (uint8_t)((cfg.DischargeTimeoutValue << 4) & 0xF0) | (cfg.DischargeCell12 << 3) | (cfg.DischargeCell11 << 2) | (cfg.DischargeCell10 << 1) | (cfg.DischargeCell9);

// 	// cmd[0] = (uint8_t)(0x80 | ((address << 3) & 0x78) | ((WriteConfigurationRegisterGroup >> 8) & 0x07));
// 	// cmd[1] = (uint8_t)(WriteConfigurationRegisterGroup & 0xFF);

// 	// PEC_return = calculatePEC(2, cmd);

// 	// cmd[2] = (PEC_return >> 8) & 0xFF;
// 	// cmd[3] = PEC_return & 0xFF;

// 	// cmd[4] = config[0];
// 	// cmd[5] = config[1];
// 	// cmd[6] = config[2];
// 	// cmd[7] = config[3];
// 	// cmd[8] = config[4];
// 	// cmd[9] = config[5];

// 	// PEC_return = calculatePEC(6, cmd + 4);

// 	// cmd[10] = (PEC_return >> 8) & 0xFF;
// 	// cmd[11] = PEC_return & 0xFF;

// 	SPIWrite(cmd, cmd_len);



//     // flip back to page 0 for regular readRegister2949

//     cmd[0] = (uint8_t)(0xFE)
//     cmd[1] = (uint8_t)(0xFF)
    
//     PEC_return = calculatePEC(2, cmd);

// 	cmd[2] = (PEC_return >> 8) & 0xFF;
// 	cmd[3] = PEC_return & 0xFF;
//     cmd[4] = (uint8_t)(Write1Byte & 0xFF) // tell the bus that you are writing 1 byte
    
//     cmd[5] = (uint8_t)(FlipPage0 & 0xFF) // select page 0

//     PEC_return = calculatePEC(1, cmd[5]);

//     cmd[6] = (PEC_return >> 8) & 0xFF;
// 	cmd[7] = PEC_return & 0xFF;

//     SPIWrite(cmd, cmd_len);


// }

//! \brief reads register specified in command from specified board
//! \param command is the read command specified by caller
//! \param data is where the read bytes will be stored 
//! \returns true if data is valid (i.e., calculated PEC matches received PEC)
bool readRegister2949(CommandCodeTypedef2949 command, uint16_t * data) { 
	uint8_t cmd[13];
	uint8_t rx_data[13];
	uint16_t PEC_return;
	bool dataValid = true;
    uint8_t readLength; // number of bytes you want to receive from the comand
    bool readLengthArray[4];
    // bool successfulWriteRead;

    wakeup_idle();

    // vary length of data sent back

    if (command == ReadCharge1 || command == ReadEnergy1) {
		readLength = 6;
	}

    if (command == ReadMaxMinCurr || command == ReadMaxMinPow || command == ReadMaxMinVoltage){
        readLength = 4;
    }
    
    if (command == ReadCurr1 || command == ReadPow1){
        readLength = 3;
    }

    if (command == ReadVolt){
        readLength = 2;
    }

    // put readLength into an array of bits to send as part of cmd

    readLengthArray[0] = (readLength & 0x8) >> 3;
    readLengthArray[1] = (readLength & 0x4) >> 2;
    readLengthArray[2] = (readLength & 0x2) >> 1;
    readLengthArray[3] = readLength & 0x1;

    // max length of PEC_send will be 16 bytes
	uint8_t PEC_send[16] = {};

	PEC_send[0] = (uint8_t)(DirectCommand & 0xFF);
	PEC_send[1] = (uint8_t)(command & 0xFF);

	cmd[0] = PEC_send[0];
	cmd[1] = PEC_send[1];

	PEC_return = calculatePEC(2,PEC_send);

	cmd[2] = (PEC_return >> 8) & 0xFF;
	cmd[3] = PEC_return & 0xFF;
    
    // Byte ID
    cmd[4] = (uint8_t)(0x80 | ((readLengthArray[0] ^ readLengthArray[1]) << 5) | (readLengthArray[0] << 4) | (readLengthArray[1] << 3) | ((readLengthArray[2] ^ readLengthArray[3]) << 2) | (readLengthArray[2] << 1) | (readLengthArray[3])); // 0x80 to specify Read/Write

	cmd[5] = 0;
	cmd[6] = 0;
	cmd[7] = 0;
	cmd[8] = 0;
	cmd[9] = 0;
	cmd[10] = 0;
	cmd[11] = 0;
    cmd[12] = 0;

	// successfulWriteRead = SPIWriteRead(cmd, rx_data, 7 + readLength);	// send 5 command bytes, receive up to 6 bytes of data and 2 PEC bytes
	SPIWriteRead(cmd, rx_data, 7 + readLength);	// send 5 command bytes, receive up to 6 bytes of data and 2 PEC bytes

	// calculate PEC based on data received
    
	PEC_send[0] = rx_data[5];  
	PEC_send[1] = rx_data[6];  
	PEC_send[2] = rx_data[7];  
	PEC_send[3] = rx_data[8];  
	PEC_send[4] = rx_data[9]; 
	PEC_send[5] = rx_data[10];

	PEC_return = calculatePEC(readLength, PEC_send);

	// check if received PEC matches calculated PEC
	if (PEC_return != (((rx_data[11] << 8) & 0xFF00) | (rx_data[12] & 0x00FF))) {
		dataValid = false;
	}

    // data is read in at MSB first

    if (command == ReadCharge1 || command == ReadEnergy1) { // length of 6 (48 bits)
        data[0] = (uint16_t)(((rx_data[5] << 8) & 0xFF00) | (rx_data[6] & 0x00FF));
        data[1] = (uint16_t)(((rx_data[7] << 8) & 0xFF00) | (rx_data[8] & 0x00FF));
        data[2] = (uint16_t)(((rx_data[9] << 8) & 0xFF00) | (rx_data[10] & 0x00FF));

	}
    
    if (command == ReadMaxMinCurr || command == ReadMaxMinPow || command == ReadMaxMinVoltage){ // length of 4 (32 bits)
        data[0] = (uint16_t)(((rx_data[5] << 8) & 0xFF00) | (rx_data[6] & 0x00FF)); // max
        data[1] = (uint16_t)(((rx_data[7] << 8) & 0xFF00) | (rx_data[8] & 0x00FF)); // min
    }

    if (command == ReadCurr1 || command == ReadPow1){ // length of 3 (24 bits)
        data[0] = (uint16_t)(rx_data[5] & 0x00FF);
        data[1] = (uint16_t)(((rx_data[6] << 8) & 0xFF00) | (rx_data[7] & 0x00FF));
    }

    if (command == ReadVolt){ // length of 2 (16 bits)
        data[0] = (uint16_t)(((rx_data[5] << 8) & 0xFF00) | (rx_data[6] & 0x00FF));
    }

	return (dataValid);
}

//! \brief called by readRegister2949, reads the pack current and converts to unit value--
//      also stores value in bms struct 
//! \param bms is the bms struct that stores critical info
//! \returns true if PEC calc matches PEC received
bool readPackCurrent(BMS_critical_info_t bms){
    bool datavalid = true;
    bool PEC_check = false;
    uint16_t tempCurrent[2];
    int32_t current;
    PEC_check = readRegister2949(ReadCurr1, tempCurrent);
    datavalid = datavalid & PEC_check;
    current = (((tempCurrent[0] << 16) & 0x00FF0000) | ((tempCurrent[1]) & 0x0000FFFF));
    current = (current << 8) >> 8; // sign extending
    bms.packCurrent = (int32_t)(current * 950 / 50); // current in mV
    return datavalid;
}

//! \brief called by readRegister2949, reads the pack voltage and converts to unit value--
//      also stores value in bms struct 
//! \param bms is the bms struct that stores critical info
//! \returns true if PEC calc matches PEC received
bool readPackVoltage(BMS_critical_info_t bms){ 
    bool datavalid = true;
    bool PEC_check = false;
    uint16_t tempVoltage[1];
    int32_t voltage;
    PEC_check = readRegister2949(ReadVolt, tempVoltage);
    datavalid = datavalid & PEC_check;
    voltage = (tempVoltage[0] & 0x0000FFFF);
    voltage = (voltage << 16) >> 16; // sign extending
    bms.packVoltage = (int32_t)(voltage * 375 * 1000); // voltage in mV
    return datavalid;
}

//! \brief called by readRegister2949, reads the pack power and converts to unit value--
//      also stores value in bms struct 
//! \param bms is the bms struct that stores critical info
//! \returns true if PEC calc matches PEC received
bool readPackPower(BMS_critical_info_t bms){ 
    bool datavalid = true;
    bool PEC_check = false;
    uint16_t tempPower[2];
    int32_t power;
    PEC_check = readRegister2949(ReadPow1, tempPower);
    datavalid = datavalid & PEC_check;
    power = (((tempPower[0] << 16) & 0x00FF0000) | ((tempPower[1]) & 0x0000FFFF));
    power = (power << 8) >> 8; // sign extending
    bms.packPower = (int32_t)(power * 58368 / 500); // power in mW
    return datavalid;
}

//! \brief called by readRegister2949, reads the pack charge and converts to unit value--
//      also stores value in bms struct 
//! \param bms is the bms struct that stores critical info
//! \returns true if PEC calc matches PEC received
bool readPackCharge(BMS_critical_info_t bms){
    bool datavalid = true;
    bool PEC_check = false;
    uint16_t tempCharge[3];
    int64_t charge;
    PEC_check = readRegister2949(ReadCharge1, tempCharge);
    datavalid = datavalid & PEC_check;
    charge = ((((int64_t)tempCharge[0] << 32) & 0x0000FFFF00000000) |  ((((int64_t)tempCharge[1]) << 16) & 0x00000000FFFF0000) | (((int64_t)tempCharge[2]) & 0x000000000000FFFF));
    charge = (charge << 16) >> 16; // sign extending
    bms.packCharge = (int32_t)(charge * 377887 / 50000000000); // charge in C downcasted from 64 bits
    return datavalid;
}

//! \brief called by readRegister2949, reads the pack energy and converts to unit value--
//      also stores value in bms struct 
//! \param bms is the bms struct that stores critical info
//! \returns true if PEC calc matches PEC received
bool readPackEnergy(BMS_critical_info_t bms){
    bool datavalid = true;
    bool PEC_check = false;
    uint16_t tempEnergy[3];
    int64_t energy;
    PEC_check = readRegister2949(ReadCharge1, tempEnergy);
    datavalid = datavalid & PEC_check;
    energy = ((((int64_t)tempEnergy[0] << 32) & 0x0000FFFF00000000) |  (((int64_t)tempEnergy[1] << 16) & 0x00000000FFFF0000) | (((int64_t)tempEnergy[2]) & 0x000000000000FFFF));
    energy = (energy << 16) >> 16; // sign extending
    bms.packEnergy = (int32_t)(energy * 232175 / 5000000000); // energy in J downcasted from 64 bits
    return datavalid;
}

//! \brief called by readRegister2949, reads the minimum & maximum pack currents and converts to unit value--
//      also stores value in bms struct 
//! \param bms is the bms struct that stores critical info
//! \returns true if PEC calc matches PEC received
bool readMaxMinCurrent(BMS_critical_info_t bms){
    bool datavalid = true;
    bool PEC_check = false;
    uint16_t tempCurrent[2];
    int32_t maxCurr;
    int32_t minCurr;
    PEC_check = readRegister2949(ReadMaxMinCurr, tempCurrent);
    datavalid = datavalid & PEC_check;
    maxCurr = (tempCurrent[0] & 0x0000FFFF);
    minCurr = (tempCurrent[1] & 0x0000FFFF);
    maxCurr = (maxCurr << 16) >> 16; // sign extending
    minCurr = (minCurr << 16) >> 16; // sign extending
    maxCurr = (maxCurr * 3800 / 50); // max current in mA
    minCurr = (minCurr * 3800 / 50); // min current in mA
    bms.maxCurrent = (uint32_t)(maxCurr);
    bms.minCurrent = (uint32_t)(minCurr);
    return datavalid;
}

//! \brief called by readRegister2949, reads the minimum & maximum pack power and converts to unit value--
//      also stores value in bms struct 
//! \param bms is the bms struct that stores critical info
//! \returns true if PEC calc matches PEC received
bool readMaxMinPower(BMS_critical_info_t bms){
    bool datavalid = true;
    bool PEC_check = false;
    uint16_t tempPower[2];
    int32_t maxPow;
    int32_t minPow;
    PEC_check = readRegister2949(ReadMaxMinCurr, tempPower);
    datavalid = datavalid & PEC_check;
    maxPow = (tempPower[0] & 0x0000FFFF);
    minPow = (tempPower[1] & 0x0000FFFF);
    maxPow = (maxPow << 16) >> 16; // sign extending
    minPow = (minPow << 16) >> 16; // sign extending
    maxPow = (maxPow * 23347 / 50); // max power in mW
    minPow = (minPow * 23347 / 50); // min power in mW
    bms.maxPower = (uint32_t)(maxPow);
    bms.minPower = (uint32_t)(minPow);
    return datavalid;
}
