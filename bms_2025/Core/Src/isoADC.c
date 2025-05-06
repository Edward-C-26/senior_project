/* Driver for AMC131M03-Q1
 * Written by cjlam2
 * note: the way this chip works, after every command, the 3 channels of ADC readings will be spit
 * onto the MISO line (5 words total per command).
*/
// TODO make this comment above more descriptive lol

#include "isoADC.h"

// DEFINES
#define ADC_BITS 24
#define ADC_SCALER (1 << ADC_BITS)
#define VBUS_RESISTANCE 4000000
#define VBUS_SENSE_RESISTANCE 1000
#define MASK_TO_INT(x) (1 << (x))
#define ISO_ADC_ID 0x23
#define ADC_RANGE 2.4
#define WORD_PER_XMISSION 5
#define WORD_SIZE 3
#define XMISSION_SIZE (WORD_PER_XMISSION * WORD_SIZE)
#define REG_SIZE 2

// EXPORTED GLOBAL VARS
isoADCConfig_t gIsoADCConfig = {
	/* CLOCK Register Configurations (page 60)
	 * - ch2_enable		- Channel 2 ADC enable
	 * - ch1_enable		- Channel 1 ADC enable
	 * - ch0_enable		- Channel 0 ADC enable
	 * - clk_div		- Clock divider ratio selection
	 * - turbo_enable	- Selects oversampling ratio 64 by setting this bit to 1b.
	 * 					  The OSR[2:0] bits are ignored if this bit is set to 1b.
	 * - osr			- Modulator oversampling ratio selection
	 * - pwr			- Power mode selection
	 */
	.ch2_enable 	= true,
	.ch1_enable 	= true,
	.ch0_enable 	= true,
	.clk_div 		= DIV_2,
	.turbo_enable	= false,
	.osr 			= OSR_4096,
	.pwr			= HIGH_RES,

	/* GAIN Register Configurations (page 61)
	 * - ch2_gain_enum	- PGA gain selection for channel 2
	 * - ch1_gain_enum	- PGA gain selection for channel 1
	 * - ch0_gain_enum	- PGA gain selection for channel 0
	 */
	// Driving car configuration is commented below
    //	cfg_ptr->ch2_gain_enum = GAIN_1;
    //	cfg_ptr->ch1_gain_enum = GAIN_1;
    //	cfg_ptr->ch0_gain_enum = GAIN_128;

	// Debugging configuration
	.ch2_gain_enum = GAIN_1,
	.ch1_gain_enum = GAIN_1,
	.ch0_gain_enum = GAIN_1,

	/* CFG Register Configurations (page 62)
	 * - gpo_enable 	- Enables digital output (GPO) functionality at AIN2P pin.
	 * - gpo_data 		- Digital output (GPO) data if GPO_EN = 1b.
	 * - gc_delay_enum	- Delay in modulator clock periods before measurement begins.
	 * - gc_enable		- Global-chop enable
	 */
	.gpo_enable		= false,
	.gpo_data		= false,
	.gc_delay_enum	= GC_DLY_2,
	.gc_enable		= true,

	/* DCDC_CTRL Register Configurations (page 78)
	 * - dcdc_freq		- DC/DC frequency range selection
	 * - dcdc_enable	- DC/DC enable
	 */
	.dcdc_freq		= DCDC_FREQ_0,
	.dcdc_enable	= true,

	// set shunt resistor value (50uohms)
    .shunt_resistance		= 0.00005f,

	// set top half of HV sense resistor divider (4M ohms)
	.vbus_resistance 		= 4000000.0f,

	// set bottom half of HV sense resistor divider (1k ohms)
	.vbus_sense_resistance	= 1000.0f
};

isoADCData_t gIsoADCData = {
	// Set all variables to default values
	.raw_shunt_voltage 			= 0,
	.raw_scaled_bus_voltage		= 0,
	.raw_thermistor_voltage		= 0,
	.shunt_temp					= 0,
	.adjusted_shunt_voltage		= 0,
	.reset						= true,
	.fuse_fail					= true,
	.sec_fail					= true,
	.abort_fault				= true,
};

// FUNCTION DEFINITIONS

/*	\brief Wake up isoADC chip
 *
 *	Wakes up isoADC by turning on internal DCDC
 *	Can also be used to reconfigure isoADC
 *	Only reconfigure if isoADC_error is true
 * 	Start-Up sequence described on page 36
 *
 * 	\param cfg_ptr	- pointer to isoADC configuration
 * 	\param data_ptr - pointer to isoADC data
 * 	\return SPI status
 */
uint8_t wakeup_isoADC(isoADCConfig_t const* cfg_ptr, isoADCData_t* data_ptr) {
	// Configures CLOCK Register
    uint8_t clock_config_response[XMISSION_SIZE];
    uint8_t clock_config_message[REG_SIZE];
    clock_config_message[0] =
    		((uint8_t)(0x00)) |
			(((uint8_t)(cfg_ptr->ch2_enable == true)) << 2) |
    		(((uint8_t)(cfg_ptr->ch1_enable == true)) << 1) |
			((uint8_t)(cfg_ptr->ch0_enable == true));
    clock_config_message[1] =
    		((uint8_t)(0x00)) |
    		((uint8_t)(cfg_ptr->clk_div & 0x03) << 6) |
    		(((uint8_t)(cfg_ptr->turbo_enable == true)) << 5) |
			((uint8_t)(cfg_ptr->osr & 0x07) << 2) |
			((uint8_t)(cfg_ptr->pwr & 0x03));

    uint8_t clock_config_status = write_isoADC_register(CLOCK_ADDRESS, clock_config_response, clock_config_message);

    // Configures DCDC_CTRL Register
    uint8_t dcdc_ctrl_config_response[XMISSION_SIZE];
    uint8_t dcdc_ctrl_config_message[REG_SIZE]; // = {0x00, 0x01}; // enables DCDC_EN to enable DCDC
    dcdc_ctrl_config_message[0] =
    		((uint8_t)(0x00)) |
			((uint8_t)(cfg_ptr->dcdc_freq & 0x0F));
    dcdc_ctrl_config_message[1] =
    		((uint8_t)(0x00)) |
			((uint8_t)(cfg_ptr->dcdc_enable == true));

    uint8_t dcdc_ctrl_config_status = write_isoADC_register(DCDC_CTRL_ADDRESS, dcdc_ctrl_config_response, dcdc_ctrl_config_message);

    // Configures GAIN Register
    uint8_t gain_config_response[XMISSION_SIZE];
    uint8_t gain_config_message[REG_SIZE];
    gain_config_message[0] =
    		((uint8_t)(0x00)) |
			((uint8_t)(cfg_ptr->ch2_gain_enum) & 0x7);
    gain_config_message[1] =
    		((uint8_t)(0x00)) |
			(((uint8_t)(cfg_ptr->ch1_gain_enum & 0x7)) << 4) |
			((uint8_t)(cfg_ptr->ch0_gain_enum & 0x7));

    uint8_t gain_config_status = write_isoADC_register(GAIN_ADDRESS, gain_config_response, gain_config_message);

    // Configures CFG Register
    uint8_t cfg_response[XMISSION_SIZE];
    uint8_t cfg_send[REG_SIZE];
    cfg_send[0] =
    		((uint8_t)(0x00)) |
			(((uint8_t)(cfg_ptr->gpo_enable == true)) << 6) |
			(((uint8_t)(cfg_ptr->gpo_data == true)) << 5) |
			(((uint8_t)(cfg_ptr->gc_delay_enum & 0x07)) << 1) |
			((uint8_t)(cfg_ptr->gc_enable == true));
    cfg_send[1] = 0x00;

    uint8_t cfg_config_status = write_isoADC_register(CFG_ADDRESS, cfg_response, cfg_send);

    // Apply external clock to start DC/DC Converter
    // Verify that this produces a 8MHz square wave
    HAL_TIM_PWM_Start(isoADC_PWM_ptr_g, isoADC_PWM_ch_g);

    // Verify DC/DC Converter Status
    uint8_t status_response_1[XMISSION_SIZE];
    uint8_t status_response_2[XMISSION_SIZE];

    read_isoADC_register(STATUS_ADDRESS, status_response_1);
    read_isoADC_register(STATUS_ADDRESS, status_response_2);

    // verify potential faults
	data_ptr->reset			= (status_response_2[0] & 0x04) == 0x04;
	data_ptr->fuse_fail		= (status_response_2[1] & 0x80) == 0x80;
	data_ptr->sec_fail		= (status_response_2[1] & 0x40) == 0x40;

    return (dcdc_ctrl_config_status && clock_config_status && cfg_config_status && gain_config_status);
}

/*	\brief Read a single register of the isoADC
 *
 *	Sends 1 RREG command followed by a NULL command to receive the data
 *
 *	\param address 			- address of register to read
 *	\param response_buffer 	- pointer to write register data into
 *	\return	SPI status
 */
uint8_t read_isoADC_register(isoADCRegisterAddr_e address, uint8_t* response_buffer) {
    // command construction from address
    uint8_t initialBuffer[XMISSION_SIZE];

    uint8_t cmd_word[XMISSION_SIZE];							// 0001 1111
    cmd_word[0] = (uint8_t)((RREG_CMD & 0xFF00) >> 8) | ((address >> 1) & 0x1F); //    00 001
    cmd_word[1] = (uint8_t)(RREG_CMD & 0x00FF) | ((address & 0x01) << 7);
    cmd_word[2] = 0x00;

    uint8_t null_word[XMISSION_SIZE];   // send a null word to get response + data
    null_word[0] = (uint8_t)((NULL_CMD & 0xFF00) >> 8);
    null_word[1] = (uint8_t)(NULL_CMD & 0x00FF);
    null_word[2] = 0x00;

    START_CRITICAL_SECTION;
    // wake up chip
    HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_RESET); // pull SPI2_NSS low to turn chip on

    // send command through SPI and store into dataBuffer
    HAL_StatusTypeDef status1 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, cmd_word, initialBuffer, sizeof(cmd_word), 1000);

    // send second command to get response
    HAL_StatusTypeDef status2 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, null_word, response_buffer, sizeof(null_word), 1000);

    // sleep chip
    HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_SET); // pull SPI2_NSS low to turn chip on
    END_CRITICAL_SECTION;

    return (uint8_t)((status1 == HAL_OK) && (status2 == HAL_OK));
}

/*	\brief write 1 register into isoADC
 *
 *	Sends a WREG command followed by a NULL command to receive write response
 *
 *	\param address 			- address of register to read
 *	\param response_buffer 	- pointer to write response data into
 *	\param write_bits		- uint8_t array of data to write into register
 *	\return SPI status
*/
uint8_t write_isoADC_register(isoADCRegisterAddr_e address, uint8_t* response_buffer, uint8_t const* write_bits) {
    uint8_t dummy_buffer[XMISSION_SIZE];

    // command construction from address
    uint8_t cmd_word[XMISSION_SIZE];
    cmd_word[0] = (uint8_t)((WREG_CMD & 0xFF00) >> 8) | ((address >> 1) & 0x1F); // write word to write 1 register
    cmd_word[1] = (uint8_t)(WREG_CMD & 0x00FF) | ((address & 0x01) << 7);
    cmd_word[2] = 0x00;

    cmd_word[3] = write_bits[0];
    cmd_word[4] = write_bits[1];
    cmd_word[5] = 0x00;

    uint8_t null_word[XMISSION_SIZE];   // send a null word to get response + data
	null_word[0] = (uint8_t)((NULL_CMD & 0xFF00) >> 8);
	null_word[1] = (uint8_t)(NULL_CMD & 0x00FF);
	null_word[2] = 0x00;

    START_CRITICAL_SECTION;
    // wake up chip
    HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_RESET); // pull SPI3_NSS low to turn chip on

    // send command through SPI and store into dataBuffer
    HAL_StatusTypeDef status1 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, cmd_word, dummy_buffer, sizeof(cmd_word), 1000);

    // send second command to get response
    HAL_StatusTypeDef status2 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, null_word, response_buffer, sizeof(null_word), 1000);

    // sleep chip
    HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_SET); // pull SPI3_NSS low to turn chip on
    END_CRITICAL_SECTION;

    return (uint8_t)((status1 == HAL_OK) && (status2 == HAL_OK));
}

/*	\brief read the isoADC ID
 *
 *	Uses the read_isoADC_register function to check the chip's ID
 *	This function is mainly for debugging
 *
 *	\return whether or not the ID matches
*/
uint8_t read_isoADC_ID(void) {
    uint8_t id_response[XMISSION_SIZE];
    (void)read_isoADC_register(ID_ADDRESS, id_response);

    // response[0:1] should be 0010 0011 XXXX XXXX
    return (uint8_t)(id_response[0] == ISO_ADC_ID);
}

/*	\brief read the ADC channels of the isoADC
 *
 *	Sends NULL commands to chip until data is ready on all 3 channels
 *	aka beating the chip into submission
 *	If a read fails 100 times, a fault bit will be enabled in data_ptr
 *
 *	\param cfg_ptr	- pointer to isoADC configuration
 * 	\param data_ptr - pointer to isoADC data
 *	\return SPI status
*/
uint8_t read_isoADC_ADCs(isoADCConfig_t const* cfg_ptr, isoADCData_t* data_ptr) {
    uint8_t response_buffer[XMISSION_SIZE];
    uint8_t abort_counter = 0;

	uint8_t null_word[XMISSION_SIZE];   // send a null word to get response + data
	null_word[0] = (uint8_t)((NULL_CMD & 0xFF00) >> 8);
	null_word[1] = (uint8_t)(NULL_CMD & 0x00FF);
	null_word[2] = 0x00;

	// SPI Transaction

    START_CRITICAL_SECTION;
	// wake up chip
	HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_RESET); // pull SPI2_NSS low to turn chip on

	// send command through SPI and store into initialBuffer
	HAL_StatusTypeDef status1 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, null_word, response_buffer, sizeof(null_word), 5);

	// this while loop should only occur on startup to manage how DRDY bits are cleared
	while (((response_buffer[1] & 0x01) != 0x01) || ((response_buffer[1] & 0x02) != 0x02) || ((response_buffer[1] & 0x04) != 0x04)) {
		abort_counter++;
		status1 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, null_word, response_buffer, sizeof(null_word), 5);

		// each abort_counter increment is a failed ADC read (data is not ready)
		if (abort_counter > 100) {
			data_ptr->abort_fault = true;
			break;
		}
	}

	// sleep chip
	HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_SET); // pull SPI2_NSS low to turn chip on
    END_CRITICAL_SECTION;

	// set DRDY bits from response
	bool ch0_drdy = (response_buffer[1] & 0x01) == 0x01;	// CH0
	bool ch1_drdy = (response_buffer[1] & 0x02) == 0x02;	// CH1
	bool ch2_drdy = (response_buffer[1] & 0x04) == 0x04;	// CH2

	if (ch0_drdy == true && ch1_drdy == true && ch2_drdy == true) {
		data_ptr->abort_fault = false;
	}

	// raw data from each channel (3 bytes long)
	int32_t raw_ch0_data = (int32_t)((((response_buffer[3] << 16) | (response_buffer[4] << 8) | response_buffer[5]) << 8) >> 8);
	int32_t raw_ch1_data = (int32_t)((((response_buffer[6] << 16) | (response_buffer[7] << 8) | response_buffer[8]) << 8) >> 8);
	int32_t raw_ch2_data = (int32_t)((((response_buffer[9] << 16) | (response_buffer[10] << 8) | response_buffer[11]) << 8) >> 8);

	/* ADC Conversion Data (page 46)
	 * 1 LSB = (2.4V / GAIN) / 2^24
	 *
	 */
    data_ptr->ch0_drdy = ch0_drdy;
    data_ptr->ch1_drdy = ch1_drdy;
    data_ptr->ch2_drdy = ch2_drdy;
	if (ch0_drdy == true) {
		data_ptr->raw_shunt_voltage = (float)((raw_ch0_data) * (ADC_RANGE / MASK_TO_INT(cfg_ptr->ch0_gain_enum))) / ADC_SCALER;
	}
	if (ch1_drdy == true) {
		data_ptr->raw_scaled_bus_voltage = (float)((raw_ch1_data) * (ADC_RANGE / MASK_TO_INT(cfg_ptr->ch1_gain_enum))) / ADC_SCALER;
	}
	if (ch2_drdy == true) {
		data_ptr->raw_thermistor_voltage = (float)((raw_ch2_data) * (ADC_RANGE / MASK_TO_INT(cfg_ptr->ch2_gain_enum))) / ADC_SCALER;
	}

	return (uint8_t)(status1 == HAL_OK);
}

/*	\brief read the isoADC status
 *
 *	Uses the read_isoADC_register function to ready status and update status bits in data_ptr
 *
 * 	\param data_ptr	- pointer to isoADC data
 *	\return SPI status
*/
uint8_t read_isoADC_status(isoADCData_t* data_ptr) {
    uint8_t status_response[XMISSION_SIZE];
    uint8_t read_status = read_isoADC_register(STATUS_ADDRESS, status_response);

    // update faults
	data_ptr->reset			= (status_response[0] & 0x04) == 0x04;
	data_ptr->fuse_fail		= (status_response[1] & 0x80) == 0x80;
	data_ptr->sec_fail		= (status_response[1] & 0x40) == 0x40;

    return read_status;
}



// TODO touch up comment
// CRC calculation for the ISO ADC
// when implementing CRC remember to add rx_crc_en to write/read regs
uint16_t calculate_crc(uint8_t *data, uint8_t length) {
    uint16_t crc = 0xFFFF; //init CRC 
    uint16_t polynomial = 0x1021; // polynomial for CCITT
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i] << 8; // xor byte into the crc to check if it passes poly check onto the high byte

        // after XORing each one, we need to process each bit in the byte and check the MSB
        for (uint8_t bit = 0; bit < 8; bit++ ){
            if (crc & 0x8000) {
                // using to check the MSB 1000 0000 is 1, if so then shift and XOR with poly
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1; // otherwise left shift
            }
        }
    }
    return crc; 

}

// TODO touch up comments
// keeping original read/write reg along with these new ones for testing purposes
uint8_t write_isoADC_reg_with_crc(isoADCRegisterAddr_e address, uint8_t* response_buffer, uint8_t const* write_bits) {
    uint8_t dummy_buffer[XMISSION_SIZE];

    uint8_t cmd_word[XMISSION_SIZE];
    cmd_word[0] = (0x60) | ((address >> 1) & (0x1F));
    cmd_word[1] = ((address & 0x01) << 7) | (0x00);
    cmd_word[2] = 0x00;

    cmd_word[3] = write_bits[0];
    cmd_word[4] = write_bits[1];

    uint16_t crc = calculate_crc(cmd_word, 5);
    cmd_word[5] = (uint8_t)(crc & 0xFF00) >> 8;
    cmd_word[6] = (uint8_t)(crc & 0x00FF);

    uint8_t null_word[XMISSION_SIZE] = {0x00, 0x00, 0x00}; 

    START_CRITICAL_SECTION;
    HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_RESET);
    HAL_StatusTypeDef status1 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, cmd_word, dummy_buffer, sizeof(cmd_word), 1000);
    HAL_StatusTypeDef status2 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, null_word, dummy_buffer, sizeof(null_word), 1000);
    HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_SET);
    END_CRITICAL_SECTION;

    return (uint8_t)((status1 == HAL_OK) && (status2 == HAL_OK));
}

bool convert_raw_to_actual(isoADCConfig_t *config, isoADCData_t *data_ptr) {
    // Pack voltage
    data_ptr->bus_voltage = data_ptr->raw_scaled_bus_voltage * (config->vbus_resistance + config->vbus_sense_resistance) / config->vbus_sense_resistance;
}

// TODO comments
uint8_t read_isoADC_reg_with_crc(isoADCRegisterAddr_e address, uint8_t* response_buffer) {
    uint8_t dummy_buffer[XMISSION_SIZE];

    uint8_t cmd_word[XMISSION_SIZE];
    cmd_word[0] = (0xA0) | ((address >> 1) & 0x1F);
    cmd_word[1] = ((address & 0x01) << 7) | (0x00);
    cmd_word[2] = 0x00;

    uint8_t null_word[XMISSION_SIZE] = {0x00, 0x00, 0x00};  
    START_CRITICAL_SECTION;
    HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_RESET);
    HAL_StatusTypeDef status1 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, cmd_word, dummy_buffer, sizeof(cmd_word), 1000);
    HAL_StatusTypeDef status2 = HAL_SPI_TransmitReceive(isoADC_SPI_ptr_g, null_word, response_buffer, sizeof(null_word), 1000);
    HAL_GPIO_WritePin(isoADC_SPI_cs_port_ptr_g, isoADC_SPI_cs_pin_g, GPIO_PIN_SET);
    END_CRITICAL_SECTION;

    if (status1 == HAL_OK && status2 == HAL_OK) {
        uint8_t received_crc_high = response_buffer[5];
        uint8_t received_crc_low = response_buffer[6];

        uint16_t received_crc = (received_crc_high << 8) | (received_crc_low);

        uint16_t calculated_crc = calculate_crc(response_buffer, 5);

        if (received_crc == calculated_crc) {
            return 1;  
        } else {
            return 0;  
        }
    }
    return 0;
}
