#ifndef LTC_2949
#define LTC_2949

#include "SPI.h"
#include "BMSconfig.h"
#include <math.h>


// data variables

int32_t packCurrent;
int32_t packVoltage;
int32_t packPower;
int32_t packCharge;
int32_t packEnergy;
int32_t maxCurrent;
int32_t minCurrent;
int32_t maxPower;
int32_t minPower;
int32_t maxVoltage;
int32_t minVoltage;

// CONSTANTS

typedef enum {
    // data registers
    ReadCharge1 = 0x00,
    ReadEnergy1 = 0x06,
    ReadCurr1 = 0x90,
    ReadPow1 = 0x93,
    ReadVolt = 0xA0,
    ReadMaxMinCurr = 0x40,
    ReadMaxMinPow = 0x44,
    ReadMaxMinVoltage = 0x50,
    // ReadMaxMinTemp = 0x54, this is chip temperature, not pack temp

    // command constants

    DirectCommand = 0xFE,
    ControlRegister = 0xFF,
    FlipPage1 = 0x81,
    FlipPage0 = 0x01,
    Write1Byte = 0x45



    // Faults are unused
    // ReadSTATVT = 0x81, // voltage/temp
    // ReadSTATIP = 0x82, // curr/pow
    // ReadSTATC = 0x83, // Charge
    // ReadSTATE = 0x84, // Energy
    // ReadSTATVCC = 0x87, // VCC


} CommandCodeTypedef2949;

bool readRegister2949(CommandCodeTypedef2949 command, uint16_t * data);
bool readPackCurrent();
bool readPackVoltage();
bool readPackPower();
bool readPackCharge();
bool readPackEnergy();
bool readMaxMinCurrent();
bool readMaxMinPower();
bool readMaxMinVoltage();
// void init_PEC15_Table(void);
//uint16_t calculatePEC(uint8_t len, uint8_t *data);
//void wakeup_idle(void);
uint8_t detectFaults(void);

#endif	//LTC_2949
