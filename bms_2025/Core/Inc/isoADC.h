#ifndef __ISOADC_H
#define __ISOADC_H

#include <main.h>
#include <math.h>
#include <stdint.h>

/* Command Words for the isoADC
 * 2 bytes each
 * RREG and WREG must be modified from bit [12:0] before being send to isoADC
 * Description on page 47
 */
typedef enum {
	NULL_CMD	= 0x0000,
	RESET_CMD	= 0x0011,
	STANDBY_CMD	= 0x0022,
	WAKEUP_CMD	= 0x0033,
	LOCK_CMD	= 0x0555,
	UNLOCK_CMD	= 0x0655,
	RREG_CMD	= 0xA000,
	WREG_CMD	= 0x6000
} isoADCCommandCode_e;

// TODO comments
typedef enum {
	ID_ADDRESS				= 0x00,
	STATUS_ADDRESS			= 0x01,
	MODE_ADDRESS			= 0x02,
	CLOCK_ADDRESS			= 0x03,
	GAIN_ADDRESS			= 0x04,
	CFG_ADDRESS				= 0x06,
	CH0_CFG_ADDRESS			= 0x09,
	CH0_OCAL_MSB_ADDRESS	= 0x0A,
	CH0_OCAL_LSB_ADDRESS	= 0x0B,
	CH0_GCAL_MSB_ADDRESS	= 0x0C,
	CH0_GCAL_LSB_ADDRESS	= 0x0D,
	CH1_CFG_ADDRESS			= 0x0E,
	CH1_OCAL_MSB_ADDRESS	= 0x0F,
	CH1_OCAL_LSB_ADDRESS	= 0x10,
	CH1_GCAL_MSB_ADDRESS	= 0x11,
	CH1_GCAL_LSB_ADDRESS	= 0x12,
	CH2_CFG_ADDRESS			= 0x13,
	CH2_OCAL_MSB_ADDRESS	= 0x14,
	CH2_OCAL_LSB_ADDRESS	= 0x15,
	CH2_GCAL_MSB_ADDRESS	= 0x16,
	CH2_GCAL_LSB_ADDRESS	= 0x17,
	DCDC_CTRL_ADDRESS		= 0x31,
	REGMAP_CRC_ADDRESS		= 0x3E
} isoADCRegisterAddr_e;

// TODO comments
typedef enum {
	DIV_2,
	DIV_4,
	DIV_8,
	DIV_12
} clkDivider_e;

// TODO comments
typedef enum {
	OSR_128,
	OSR_256,
	OSR_512,
	OSR_1024,
	OSR_2048,
	OSR_4096,
	OSR_8192,
	OSR_16384
} osr_t;

// TODO comments
typedef enum {
	LOW_PWR		= 0x01,
	HIGH_RES	= 0x10
} pwr_t;

// TODO comments
typedef enum {
	GAIN_1,
	GAIN_2,
	GAIN_4,
	GAIN_8,
	GAIN_16,
	GAIN_32,
	GAIN_64,
	GAIN_128
} chGain_e;

typedef enum {
	GC_DLY_2,
	GC_DLY_4,
	GC_DLY_8,
	GC_DLY_16,
	GC_DLY_32,
	GC_DLY_64,
	GC_DLY_128,
	GC_DLY_256,
	GC_DLY_512,
	GC_DLY_1024,
	GC_DLY_2048,
	GC_DLY_4096,
	GC_DLY_8192,
	GC_DLY_16384,
	GC_DLY_32768,
	GC_DLY_65535
} gcDelay_e;

typedef enum {
	/* DC/DC frequency range selection
		Selects the range of the modulator clock frequency, based on the frequency at the CLKIN
		pin and clock divider ratio.
		0000b = 3.76 MHz to 4.10 MHz
	    0001b = 3.52 MHz to 3.84 MHz
	  	0010b = 3.30 MHz to 3.59 MHz
		0011b = 3.09 MHz to 3.36 MHz
		0100b = 2.89 MHz to 3.15 MHz
		0101b = 2.71 MHz to 2.95 MHz
		0110b = 2.53 MHz to 2.76 MHz
		0111b = 2.37 MHz to 2.59 MHz
		1000b = 2.22 MHz to 2.42 MHz
		1001b = 2.08 MHz to 2.27 MHz
		1010b = 1.95 MHz to 2.12 MHz
		1011b = 1.82 MHz to 1.99 MHz
		1100b = 1.71 MHz to 1.86 MHz
		1101b = 1.60 MHz to 1.74 MHz
		1110b = 1.50 MHz to 1.63 MHz
		1111b = 1.40 MHz to 1.53 MHz
	*/
	DCDC_FREQ_0,
	DCDC_FREQ_1,
	DCDC_FREQ_2,
	DCDC_FREQ_3,
	DCDC_FREQ_4,
	DCDC_FREQ_5,
	DCDC_FREQ_6,
	DCDC_FREQ_7,
	DCDC_FREQ_8,
	DCDC_FREQ_9,
	DCDC_FREQ_10,
	DCDC_FREQ_11,
	DCDC_FREQ_12,
	DCDC_FREQ_13,
	DCDC_FREQ_14,
	DCDC_FREQ_15
} dcdcFreq_t;

typedef struct {
	// CLOCK Register Configurations
	bool ch2_enable;
	bool ch1_enable;
	bool ch0_enable;

	clkDivider_e clk_div;
	bool turbo_enable;
	osr_t osr;
	pwr_t pwr;

	// GAIN Register Configurations
	chGain_e ch2_gain_enum;
	chGain_e ch1_gain_enum;
	chGain_e ch0_gain_enum;

	// CFG Register Configurations
	bool gpo_enable;
	bool gpo_data;
	gcDelay_e gc_delay_enum;
	bool gc_enable;

	// DCDC_CTRL Register Configurations
	dcdcFreq_t dcdc_freq;
	bool dcdc_enable;

	// Driver Level Configurations
	float shunt_resistance;
	float vbus_resistance;
	float vbus_sense_resistance;

} isoADCConfig_t;

typedef struct {
	// channel 0 - Shunt Resistor
	float raw_shunt_voltage;		// raw data processed through SPI

	// channel 1 - Pack Voltage Divider
	float raw_scaled_bus_voltage;	// raw data processed through SPI

	// channel 2 - Shunt Thermistor
	float raw_thermistor_voltage;	// raw data processed through SPI

    // Used to know if the channel was read by the last operation
    bool ch0_drdy;
    bool ch1_drdy;
    bool ch2_drdy;


	// processed data
    float bus_voltage;
	float shunt_temp;					// calculated shunt resistor temperature
	float adjusted_shunt_voltage;		// adjusted based on shunt resistor temperature

	// chip error management
	bool reset;
	bool fuse_fail;
	bool sec_fail;
	bool abort_fault;
} isoADCData_t;

// Startup functions
uint8_t wakeup_isoADC(isoADCConfig_t const* cfg_ptr, isoADCData_t* data_ptr);

// SPI level transactions
uint8_t read_isoADC_register(uint8_t address, uint8_t* response_buffer);
uint8_t write_isoADC_register(uint8_t address, uint8_t* response_buffer, uint8_t const* write_bits);

// Read Registers
uint8_t read_isoADC_ID(void);
uint8_t read_isoADC_ADCs(isoADCConfig_t const* cfg_ptr, isoADCData_t* data_ptr);
uint8_t read_isoADC_status(isoADCData_t* data_ptr);

// CRC
uint16_t calculate_crc(uint8_t *data, uint8_t length);
uint8_t write_isoADC_reg_with_crc(uint8_t address, uint8_t* response_buffer, uint8_t const* write_bits);
uint8_t read_isoADC_reg_with_crc(uint8_t address, uint8_t* response_buffer);

// Conversion
bool convert_raw_to_actual(isoADCConfig_t *config, isoADCData_t *data_ptr);

extern SPI_HandleTypeDef* isoADC_SPI_ptr_g;
extern GPIO_TypeDef* isoADC_SPI_cs_port_ptr_g;
extern uint16_t isoADC_SPI_cs_pin_g;
extern TIM_HandleTypeDef* isoADC_PWM_ptr_g;
extern uint32_t	isoADC_PWM_ch_g;


#endif
