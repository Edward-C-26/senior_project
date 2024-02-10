#ifndef LTC_2949
#define LTC_2949

#include "SPI.h"
#include "BMSconfig.h"
#include "math.h"
#include "Fault.h"

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
bool readPackCurrent(BMS_critical_info_t bms);
bool readPackVoltage(BMS_critical_info_t bms);
bool readPackPower(BMS_critical_info_t bms);
bool readPackCharge(BMS_critical_info_t bms);
bool readPackEnergy(BMS_critical_info_t bms);
bool readMaxMinCurrent(BMS_critical_info_t bms);
bool readMaxMinPower(BMS_critical_info_t bms);
bool readMaxMinVoltage(BMS_critical_info_t bms);
void init_PEC15_Table_2949();
uint16_t calculatePEC_2949(uint8_t len, uint8_t *data);
void wakeup_idle_2949(void);

#endif	//LTC_2949
