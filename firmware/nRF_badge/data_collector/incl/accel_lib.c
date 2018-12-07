#include "accel_lib.h"
#include "spi_lib.h"

#include "debug_lib.h"

#include "nrf_drv_gpiote.h"		// Needed for the INT-Pin


#include "app_util_platform.h"

#include "systick_lib.h" // Needed for accel_selftest()




#if ACCELEROMETER_PRESENT
#define ACCEL_INT1_PIN														ACCELEROMETER_INT_PIN
#else
#define ACCEL_INT1_PIN														25
#endif

/**< The default configuration parameters for the accelerometer */
#define	ACCEL_DATARATE_DEFAULT												ACCEL_DATARATE_10_HZ 
#define ACCEL_OPERATING_MODE_DEFAULT										ACCEL_POWER_DOWN_MODE
#define ACCEL_AXIS_DEFAULT													(ACCEL_X_AXIS_ENABLE | ACCEL_Y_AXIS_ENABLE | ACCEL_Z_AXIS_ENABLE)
#define ACCEL_FULL_SCALE_DEFAULT											ACCEL_FULL_SCALE_2G
#define ACCEL_FIFO_DEFAULT													ACCEL_FIFO_DISABLE
#define ACCEL_HP_FILTER_DEFAULT												ACCEL_HP_FILTER_DISABLE
#define ACCEL_INTERRUPT_EVENT_DEFAULT										ACCEL_NO_INTERRUPT
#define ACCEL_MOTION_INTERRUPT_PARAMETER_THRESHOLD_MG_DEFAULT				250
#define	ACCEL_MOTION_INTERRUPT_PARAMETER_MINIMAL_DURATION_MS_DEFAULT		0


/**< Parameters for the selftest-function of the accelerometer */
#define ACCEL_SELFTEST_TIME_FOR_INTERRUPT_GENERATION_MS						10000
#define ACCEL_SELFTEST_INTERRUPT_THRESHOLD_MG								250
#define ACCEL_SELFTEST_INTERRUPT_MINIMAL_DURATION_MS						20





#define LIS2DH12_WHO_AM_I_VALUE       	0x33		/**< The value of the WHO_AM_I-register */


/**< All register addresses of LIS2DH12 */
#define LIS2DH12_STATUS_REG_AUX_ADDR	0x07
#define LIS2DH12_OUT_TMEP_L_ADDR		0x0C
#define LIS2DH12_OUT_TMEP_H_ADDR		0x0D
#define LIS2DH12_WHO_AM_I_ADDR			0x0F
#define LIS2DH12_CTRL_REG0_ADDR			0x1E
#define LIS2DH12_TEMP_CFG_REG_ADDR		0x1F
#define LIS2DH12_CTRL_REG1_ADDR			0x20
#define LIS2DH12_CTRL_REG2_ADDR			0x21
#define LIS2DH12_CTRL_REG3_ADDR			0x22
#define LIS2DH12_CTRL_REG4_ADDR			0x23
#define LIS2DH12_CTRL_REG5_ADDR			0x24
#define LIS2DH12_CTRL_REG6_ADDR			0x25
#define LIS2DH12_REFERENCE_ADDR			0x26
#define LIS2DH12_STATUS_REG_ADDR		0x27
#define LIS2DH12_OUT_X_L_ADDR			0x28
#define LIS2DH12_OUT_X_H_ADDR			0x29
#define LIS2DH12_OUT_Y_L_ADDR			0x2A
#define LIS2DH12_OUT_Y_H_ADDR			0x2B
#define LIS2DH12_OUT_Z_L_ADDR			0x2C
#define LIS2DH12_OUT_Z_H_ADDR			0x2D
#define LIS2DH12_FIFO_CTRL_REG_ADDR		0x2E
#define LIS2DH12_FIFO_SRC_REG_ADDR		0x2F
#define LIS2DH12_INT1_CFG_ADDR			0x30
#define LIS2DH12_INT1_SRC_ADDR			0x31
#define LIS2DH12_INT1_THS_ADDR			0x32
#define LIS2DH12_INT1_DURATION_ADDR		0x33
#define LIS2DH12_INT2_CFG_ADDR			0x34
#define LIS2DH12_INT2_SRC_ADDR			0x35
#define LIS2DH12_INT2_THS_ADDR			0x36
#define LIS2DH12_INT2_DURATION_ADDR		0x37
#define LIS2DH12_CLICK_CFG_ADDR			0x38
#define LIS2DH12_CLICK_SRC_ADDR			0x39
#define LIS2DH12_CLICK_THS_ADDR			0x3A
#define LIS2DH12_TIME_LIMIT_ADDR		0x3B
#define LIS2DH12_TIME_LATENCY_ADDR		0x3C
#define LIS2DH12_TIME_WINDOW_ADDR		0x3D
#define LIS2DH12_ACT_THS_ADDR			0x3E
#define LIS2DH12_ACT_DUR_ADDR			0x3F

#define FIFO_SIZE						32					/**< The number of levels in the accelerometer FIFO */
#define FIFO_BUFFER_SIZE				(1 + FIFO_SIZE * 6)	/**< The buffer size of the spi operation (read/write registers). "1+" for the header byte that characterize the operation. */



static accel_datarate_t 		datarate = ACCEL_DATARATE_DEFAULT;						/**< The current datarate */
static accel_full_scale_t 		full_scale = ACCEL_FULL_SCALE_DEFAULT;					/**< The current full-scale */
static accel_operating_mode_t 	operating_mode = ACCEL_OPERATING_MODE_DEFAULT;			/**< The current operating-mode */
static accel_HP_filter_t 		HP_filter = ACCEL_HP_FILTER_DEFAULT;					/**< The current HP filter mode */


static volatile accel_interrupt_event_t		interrupt_event = ACCEL_INTERRUPT_EVENT_DEFAULT;	/**< The current interrupt event that is generated, when the interrupt-pin switches from low to high */
static volatile accel_interrupt_handler_t 	interrupt_handler = NULL;							/**< The current interrupt handler callback function */

static uint16_t motion_interrupt_parameter_threshold_mg = ACCEL_MOTION_INTERRUPT_PARAMETER_THRESHOLD_MG_DEFAULT;	/**< The current threshold for the motion interrupt */
static uint16_t motion_interrupt_parameter_minimal_duration_ms = ACCEL_MOTION_INTERRUPT_PARAMETER_MINIMAL_DURATION_MS_DEFAULT;		/**< The current minimal duration for the motion interrupt */


static spi_instance_t spi_instance;								/**< The spi instance used to communicate with the LIS2DH12-IC via SPI */

static uint8_t fifo_buf[FIFO_BUFFER_SIZE];						/**< The buffer for the FIFO-read operation */



/**@brief	Function for reading a 8-bit register value from an accelerometer register.
 *
 * @param[in]	addr	The address of the accelerometer register.
 * @param[out]	value	Pointer to memory where to store the read value.
 *
 *
 * @retval  NRF_SUCCESS    			If the spi-read operation was successful.
 * @retval 	NRF_ERROR_BUSY			If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL		If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
static ret_code_t accel_read_reg_8(uint8_t addr, uint8_t* value) {

	uint8_t tx_buf[1]= {addr | 0x80};
	uint8_t rx_buf[2]= {0, 0};
	
	ret_code_t ret = spi_transmit_receive(&spi_instance, tx_buf, 1, rx_buf, 2);
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY or NRF_ERROR_INVALID_ADDR
	if(ret == NRF_ERROR_INVALID_ADDR)
		ret = NRF_ERROR_INTERNAL;
	
	*value = rx_buf[1];
	
	return ret;
}


/**@brief   Function for writing a 8-bit register value to an accelerometer register.
 *
 * @param[in]	addr	The address of the accelerometer register.
 * @param[in]	value	The value that should be written to the register.
 *
 *
 * @retval  NRF_SUCCESS    			If the spi-write operation was successful.
 * @retval 	NRF_ERROR_BUSY			If the spi-module is busy.
 * @retval	NRF_ERROR_INTERNAL		If there were some internal problems, because the buffers weren't in RAM-section (should not happen!).
 */
static ret_code_t accel_write_reg_8(uint8_t addr, uint8_t value) {
	uint8_t tx_buf[2] = {addr & (~0x80), value};
	uint8_t rx_buf[2] = {0, 0};
	
	
	ret_code_t ret = spi_transmit_receive(&spi_instance, tx_buf, 2, rx_buf, 2);
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY or NRF_ERROR_INVALID_ADDR
	if(ret == NRF_ERROR_INVALID_ADDR)
		ret = NRF_ERROR_INTERNAL;
	
	
	return ret;
}



/**@brief   The interrupt handler function of the INT1-interrupt pin.
 *
 * @param[in]	pin		The gpio-pin where the event/interrupt was detected.
 * @param[in]	action	The action of the pin (e.g. NRF_GPIOTE_POLARITY_LOTOHI)
 *
 */
void accel_int1_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
	if(pin == ACCEL_INT1_PIN && action == NRF_GPIOTE_POLARITY_LOTOHI) {
		if(interrupt_event != ACCEL_NO_INTERRUPT) {
			// TODO: remove
			//nrf_gpio_pin_toggle(9);
			
			
			if(interrupt_handler != NULL) {
				accel_interrupt_event_t evt = interrupt_event; // A temporary copy, so the called handler can't change the internal interrupt_event
				interrupt_handler(&evt);
			}
		}
		
	} 	
}

/**@brief Function for retrieving the shift-factor and the sensitvity for a specific accelerometer configuration.
 *
 * @param[out]	shift_factor			The number of bits the raw accelerometer values should be shifted to left.
 * @param[out]	sensitivity				The sensitivity the raw accelerometer values should be multiplied with [mg/LSB].
 * @param[in]	accel_operating_mode	The current operating-mode of the accelerometer.
 * @param[in]	accel_full_scale		The current full-scale of the accelerometer.
 */
static void get_shift_factor_and_sensitivity(int16_t* shift_factor, int16_t* sensitivity, accel_operating_mode_t accel_operating_mode, accel_full_scale_t accel_full_scale) {
	// The values in this function are from the Driver of the ST-package
	switch(accel_operating_mode) {
		case ACCEL_LOW_POWER_MODE: 		
			*shift_factor = 8;
			switch(accel_full_scale) {
				case ACCEL_FULL_SCALE_2G:
					*sensitivity = 16;
				break;
				case ACCEL_FULL_SCALE_4G:
					*sensitivity = 32;
				break;
				case ACCEL_FULL_SCALE_8G:
					*sensitivity = 64;
				break;
				case ACCEL_FULL_SCALE_16G:
					*sensitivity = 192;
				break;
				default:
					*sensitivity = 0;
				break;				
			}
		break;
		
		case ACCEL_NORMAL_MODE: 		
			*shift_factor = 6;
			switch(accel_full_scale) {
				case ACCEL_FULL_SCALE_2G:
					*sensitivity = 4;
				break;
				case ACCEL_FULL_SCALE_4G:
					*sensitivity = 8;
				break;
				case ACCEL_FULL_SCALE_8G:
					*sensitivity = 16;
				break;
				case ACCEL_FULL_SCALE_16G:
					*sensitivity = 48;
				break;
				default:
					*sensitivity = 0;
				break;				
			}
		break;
		case ACCEL_HIGH_RESOLUTION_MODE: 			
			*shift_factor = 4;
			switch(accel_full_scale) {
				case ACCEL_FULL_SCALE_2G:
					*sensitivity = 1;
				break;
				case ACCEL_FULL_SCALE_4G:
					*sensitivity = 2;
				break;
				case ACCEL_FULL_SCALE_8G:
					*sensitivity = 4;
				break;
				case ACCEL_FULL_SCALE_16G:
					*sensitivity = 12;
				break;
				default:
					*sensitivity = 0;
				break;				
			}
		break;
		default:
			*shift_factor = 0;
			*sensitivity = 0;
		break;
	}
}


/**@brief Function for retrieving the mg per LSB for the threshold register for interrupt configuration.
 *
 * @param[in]	accel_full_scale	The current full-scale of the accelerometer.
 *
 * @retval 	mg per LSB for the current configuration.
 */
static uint16_t get_threshold_mg_per_lsb(accel_full_scale_t accel_full_scale) {
	
	uint16_t mg_per_lsb = 16;
	switch(accel_full_scale) {	// See Datasheet LIS2DH12: chapter 8.21, p.43/53 
		case ACCEL_FULL_SCALE_2G:
			mg_per_lsb = 16;
		break;
		case ACCEL_FULL_SCALE_4G:
			mg_per_lsb = 32;
		break;
		case ACCEL_FULL_SCALE_8G:
			mg_per_lsb = 62;
		break;
		case ACCEL_FULL_SCALE_16G:
			mg_per_lsb = 186;
		break;
		default:
			mg_per_lsb = 16;
		break;
		
	}
	return mg_per_lsb;
	
}


/**@brief Function for retrieving the datarate as integer.
 *
 * @param[in]	accel_datarate			The current datarate of the accelerometer.
 * @param[in]	accel_operating_mode	The current operating-mode of the accelerometer.
 *
 * @retval 	The datarate as integer.
 */
static uint16_t get_datarate_as_number(accel_datarate_t accel_datarate, accel_operating_mode_t accel_operating_mode) {
	uint16_t datarate_as_number = 1;
	switch(accel_datarate) {
		case ACCEL_DATARATE_1_HZ:
			datarate_as_number = 1;
		break;
		case ACCEL_DATARATE_10_HZ:
			datarate_as_number = 10;
		break;
		case ACCEL_DATARATE_25_HZ:
			datarate_as_number = 25;
		break;
		case ACCEL_DATARATE_50_HZ:
			datarate_as_number = 50;
		break;
		case ACCEL_DATARATE_100_HZ:
			datarate_as_number = 100;
		break;
		case ACCEL_DATARATE_200_HZ:
			datarate_as_number = 200;
		break;
		case ACCEL_DATARATE_400_HZ:
			datarate_as_number = 400;
		break;
		case ACCEL_DATARATE_1600_HZ:
			datarate_as_number = 1600;
		break;
		case ACCEL_DATARATE_1344_HZ_5376_HZ:
			if(accel_operating_mode == ACCEL_LOW_POWER_MODE)
				datarate_as_number = 5376;
			else 
				datarate_as_number = 1344;
		break;		
		default:
			datarate_as_number = 1;
		break;
	}
	
	return datarate_as_number;
}





ret_code_t 	accel_init(void) {
	
	// TODO: retrieve Pin-numbers from the custom_board-file!
	// TODO: Interrupt pin configuration!
	
	static uint8_t init_done = 0;
	
	if(init_done == 0) {
		
		#if ACCELEROMETER_PRESENT
		
		// SPI-module intitalization
		spi_instance.spi_peripheral = 0;
		spi_instance.nrf_drv_spi_config.frequency 		= NRF_DRV_SPI_FREQ_8M;
		spi_instance.nrf_drv_spi_config.bit_order 		= NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
		spi_instance.nrf_drv_spi_config.mode			= NRF_DRV_SPI_MODE_3;
		spi_instance.nrf_drv_spi_config.orc				= 0;
		spi_instance.nrf_drv_spi_config.irq_priority	= APP_IRQ_PRIORITY_MID; 	
		spi_instance.nrf_drv_spi_config.ss_pin 			= SPIM0_ACCELEROMETER_SS_PIN;
		spi_instance.nrf_drv_spi_config.miso_pin		= SPIM0_MISO_PIN;
		spi_instance.nrf_drv_spi_config.mosi_pin		= SPIM0_MOSI_PIN;
		spi_instance.nrf_drv_spi_config.sck_pin			= SPIM0_SCK_PIN;
		
		ret_code_t ret = spi_init(&spi_instance);
		
		// ret could be NRF_SUCCESS or NRF_ERROR_INVALID_PARAM
		if(ret != NRF_SUCCESS)
			return NRF_ERROR_INTERNAL;
		
		// Check the WHO_AM_I-register value
		uint8_t value = 0;
		ret = accel_read_reg_8(LIS2DH12_WHO_AM_I_ADDR, &value);
		// ret could be NRF_SUCCESS, NRF_ERROR_BUSY or NRF_ERROR_INTERNAL
		if(ret != NRF_SUCCESS)
			return ret;
		
		if(value != LIS2DH12_WHO_AM_I_VALUE) {
			return NRF_ERROR_INTERNAL;
		}	
		
		
		// Interrupt pin configuration and initialization:
		if(!nrf_drv_gpiote_is_init())
			nrf_drv_gpiote_init();
		
		
		// GPIOTE_CONFIG_IN_SENSE_LOTOHI(true) or GPIOTE_CONFIG_IN_SENSE_HITOLO(true)
		nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(false);
		in_config.pull = NRF_GPIO_PIN_NOPULL;
		ret = nrf_drv_gpiote_in_init(ACCEL_INT1_PIN, &in_config, accel_int1_event_handler);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		nrf_drv_gpiote_in_event_enable(ACCEL_INT1_PIN, true);
		

		
		// Enable the default axis
		ret = accel_set_axis(ACCEL_AXIS_DEFAULT);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Set the default full scale
		full_scale = ACCEL_FULL_SCALE_DEFAULT;
		ret = accel_set_full_scale(ACCEL_FULL_SCALE_DEFAULT);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Set the default datarate
		datarate = ACCEL_DATARATE_DEFAULT;
		ret = accel_set_datarate(ACCEL_DATARATE_DEFAULT);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Set the default operating mode
		ret = accel_set_operating_mode(ACCEL_OPERATING_MODE_DEFAULT);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Set the default FIFO-configuration
		ret = accel_set_fifo(ACCEL_FIFO_DEFAULT);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Set the default HP-filter configuration
		HP_filter = ACCEL_HP_FILTER_DEFAULT;
		ret = accel_set_HP_filter(ACCEL_HP_FILTER_DEFAULT);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		
		// Set the default motion interrupt threshold and duration-parameters
		motion_interrupt_parameter_threshold_mg = ACCEL_MOTION_INTERRUPT_PARAMETER_THRESHOLD_MG_DEFAULT;
		motion_interrupt_parameter_minimal_duration_ms = ACCEL_MOTION_INTERRUPT_PARAMETER_MINIMAL_DURATION_MS_DEFAULT;
		accel_set_motion_interrupt_parameters(ACCEL_MOTION_INTERRUPT_PARAMETER_THRESHOLD_MG_DEFAULT, ACCEL_MOTION_INTERRUPT_PARAMETER_MINIMAL_DURATION_MS_DEFAULT);
		
		// Set the default interrupt operation/event
		interrupt_handler = NULL;
		interrupt_event = ACCEL_INTERRUPT_EVENT_DEFAULT;
		ret = accel_set_interrupt(ACCEL_INTERRUPT_EVENT_DEFAULT);
		if(ret != NRF_SUCCESS) return NRF_ERROR_INTERNAL;
		#endif
	}
	init_done = 1;
		
	return NRF_SUCCESS;
}



ret_code_t 	accel_set_datarate(accel_datarate_t accel_datarate) {	
	
	ret_code_t ret;
	uint8_t ctrl_reg1_val;	
	ret = accel_read_reg_8(LIS2DH12_CTRL_REG1_ADDR, &ctrl_reg1_val);
	if(ret != NRF_SUCCESS) return ret;
	
	ctrl_reg1_val &= ~(0xF0);							// Clear all ODR-bits
	ctrl_reg1_val |= ((accel_datarate << 4) & 0xF0);	// Set ODR-bits according to the datarate
	
	ret = accel_write_reg_8(LIS2DH12_CTRL_REG1_ADDR, ctrl_reg1_val);
	if(ret != NRF_SUCCESS) return ret;
	
	datarate = accel_datarate;
	
	return NRF_SUCCESS;
}


ret_code_t 	accel_set_operating_mode(accel_operating_mode_t accel_operating_mode) {
	
	
	ret_code_t ret;
	// Always set the datarate before changing the operating mode, because if
	// the application switches from power-down to another operating mode, 
	// the former datarate has to be restored.
	ret = accel_set_datarate(datarate);
	if(ret != NRF_SUCCESS) return ret;
	
	// See: Table 3: Operating mode selection 
	uint8_t ctrl_reg1_val;	
	uint8_t ctrl_reg4_val;
	
	ret = accel_read_reg_8(LIS2DH12_CTRL_REG1_ADDR, &ctrl_reg1_val);
	if(ret != NRF_SUCCESS) return ret;
	ret = accel_read_reg_8(LIS2DH12_CTRL_REG4_ADDR, &ctrl_reg4_val);
	if(ret != NRF_SUCCESS) return ret;
	
	switch(accel_operating_mode) {
		case ACCEL_POWER_DOWN_MODE:
		{
			ctrl_reg1_val &= ~(0x08);	// Clear LPen-bit 	(default: normal mode)
			ctrl_reg4_val &= ~(0x08);	// Clear HR-bit		(default: normal mode)
			ctrl_reg1_val &= ~(0xF0);	// Clear all ODR bits for power-down mode --> see: Table 4: Data rate configuration
		}
		break;
		case ACCEL_LOW_POWER_MODE:
		{
			ctrl_reg1_val |= (0x08);	// Set LPen-bit
			ctrl_reg4_val &= ~(0x08);	// Clear HR-bit
		}
		break;
		case ACCEL_NORMAL_MODE:
		{
			ctrl_reg1_val &= ~(0x08);	// Clear LPen-bit
			ctrl_reg4_val &= ~(0x08);	// Clear HR-bit
		}
		break;
		case ACCEL_HIGH_RESOLUTION_MODE:
		{
			ctrl_reg1_val &= ~(0x08);	// Clear LPen-bit
			ctrl_reg4_val |= (0x08);	// Set HR-bit
		}
		break;	
	}
	
	ctrl_reg4_val |= (0x80);	// Set BDU-bit
	
	
	ret = accel_write_reg_8(LIS2DH12_CTRL_REG1_ADDR, ctrl_reg1_val);
	if(ret != NRF_SUCCESS) return ret;
	ret = accel_write_reg_8(LIS2DH12_CTRL_REG4_ADDR, ctrl_reg4_val);
	if(ret != NRF_SUCCESS) return ret;
	
	operating_mode = accel_operating_mode;
	
	return NRF_SUCCESS;
}

ret_code_t 	accel_set_axis(accel_axis_t accel_axis) {
	ret_code_t ret;
	uint8_t ctrl_reg1_val;
	ret = accel_read_reg_8(LIS2DH12_CTRL_REG1_ADDR, &ctrl_reg1_val);
	if(ret != NRF_SUCCESS) return ret;
	
	if(accel_axis & ACCEL_X_AXIS_ENABLE) {
		ctrl_reg1_val |= (1 << 0);
	}
	if(accel_axis & ACCEL_Y_AXIS_ENABLE) {
		ctrl_reg1_val |= (1 << 1);
	}
	if(accel_axis & ACCEL_Z_AXIS_ENABLE) {
		ctrl_reg1_val |= (1 << 2);
	}
	
	if(accel_axis & ACCEL_X_AXIS_DISABLE) {
		ctrl_reg1_val &= ~(1 << 0);
	}
	if(accel_axis & ACCEL_Y_AXIS_DISABLE) {
		ctrl_reg1_val &= ~(1 << 1);
	}
	if(accel_axis & ACCEL_Z_AXIS_DISABLE) {
		ctrl_reg1_val &= ~(1 << 2);
	}
	
	
	ret = accel_write_reg_8(LIS2DH12_CTRL_REG1_ADDR, ctrl_reg1_val);
	if(ret != NRF_SUCCESS) return ret;
	
	return NRF_SUCCESS;
}



ret_code_t 	accel_set_full_scale(accel_full_scale_t accel_full_scale) {
	ret_code_t ret;
	
	uint8_t ctrl_reg4_val;
	
	ret = accel_read_reg_8(LIS2DH12_CTRL_REG4_ADDR, &ctrl_reg4_val);
	if(ret != NRF_SUCCESS) return ret;
	
	
	ctrl_reg4_val &= ~(0x30);							// Clear the Full-scale bits
	ctrl_reg4_val |= ((accel_full_scale << 4) & 0xF0); 	// Set the Full-scale bits accordingly
	
	ret = accel_write_reg_8(LIS2DH12_CTRL_REG4_ADDR, ctrl_reg4_val);
	if(ret != NRF_SUCCESS) return ret;
	
	full_scale = accel_full_scale;
	
	return NRF_SUCCESS;
	
}

ret_code_t 	accel_set_HP_filter(accel_HP_filter_t accel_HP_filter) {
	ret_code_t ret;
	uint8_t tmp;
	switch(accel_HP_filter) {
		case ACCEL_HP_FILTER_ENABLE:
			// Enable the High-pass filter for FIFO data:
			ret = accel_read_reg_8(LIS2DH12_CTRL_REG2_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
			tmp &= ~(0xF0);		// High pass filter: Normal mode, (CutOff 2Hz@100Hz)
			tmp |= 0x08;		// High pass filter: HP-data enabled for FIFO
			ret = accel_write_reg_8(LIS2DH12_CTRL_REG2_ADDR, tmp);	
			if(ret != NRF_SUCCESS) return ret;
			
			// Dummy read the reference register to reset the HP-filter:
			ret = accel_read_reg_8(LIS2DH12_REFERENCE_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
		break;
		
		case ACCEL_HP_FILTER_DISABLE:
			
		default:
			// Disable the High-pass filter for FIFO data:
			ret = accel_read_reg_8(LIS2DH12_CTRL_REG2_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
			tmp &= ~(0x08);		// High pass filter: HP-data disabled for FIFO
			ret = accel_write_reg_8(LIS2DH12_CTRL_REG2_ADDR, tmp);	
			if(ret != NRF_SUCCESS) return ret;
		break;
	}
	
	HP_filter = accel_HP_filter;
	
	return NRF_SUCCESS;
}


void 		accel_set_interrupt_handler(accel_interrupt_handler_t accel_interrupt_handler) {
	interrupt_handler = accel_interrupt_handler;
}



ret_code_t 	accel_set_motion_interrupt_parameters(uint16_t threshold_mg, uint16_t minimal_duration_ms) {

	
	ret_code_t ret;
	
	// Calculate and write the interrupt-threshold:
	uint16_t mg_per_lsb = get_threshold_mg_per_lsb(full_scale);
	uint16_t threshold = threshold_mg/mg_per_lsb;
	if(threshold > 0x7F) threshold = 0x7F;
	ret = accel_write_reg_8(LIS2DH12_INT1_THS_ADDR, (uint8_t) threshold);
	if(ret != NRF_SUCCESS) return ret;
	
	// Calculate and write the interrupt-duration:
	uint16_t datarate_as_number = get_datarate_as_number(datarate, operating_mode);
	uint16_t duration = (uint16_t)(((uint32_t)(((uint32_t)datarate_as_number) * ((uint32_t) minimal_duration_ms))) / 1000);
	if(duration > 0x7F) duration = 0x7F;
	ret = accel_write_reg_8(LIS2DH12_INT1_DURATION_ADDR, (uint8_t) duration);
	if(ret != NRF_SUCCESS) return ret;
	
	
	motion_interrupt_parameter_threshold_mg = threshold_mg;
	motion_interrupt_parameter_minimal_duration_ms = minimal_duration_ms;
	
	return NRF_SUCCESS;
	
}



ret_code_t 	accel_set_interrupt(accel_interrupt_event_t accel_interrupt_event) {
	ret_code_t ret;
	uint8_t tmp;
	
	// Disable everything (so that the former config of antoher interrupt source is replaced)
	
	// Disable HP_IA1 interrupt of HP-Filter:
	ret = accel_read_reg_8(LIS2DH12_CTRL_REG2_ADDR, &tmp);
	if(ret != NRF_SUCCESS) return ret;
	tmp &= ~(0x01);
	ret = accel_write_reg_8(LIS2DH12_CTRL_REG2_ADDR, tmp); 
	if(ret != NRF_SUCCESS) return ret;
	
	// Disable Interrupt Pin:
	ret = accel_write_reg_8(LIS2DH12_CTRL_REG3_ADDR, 0x00); 
	if(ret != NRF_SUCCESS) return ret;
	
	// Reset the interupt 1 latched bit (although it should not be set)
	ret = accel_read_reg_8(LIS2DH12_CTRL_REG5_ADDR, &tmp);
	if(ret != NRF_SUCCESS) return ret;
	tmp &= ~(0x08);	// Clear LIR_INT1-bit
	ret = accel_write_reg_8(LIS2DH12_CTRL_REG5_ADDR, tmp);
	if(ret != NRF_SUCCESS) return ret;
	
	// Reset the threshold:
	ret = accel_write_reg_8(LIS2DH12_INT1_THS_ADDR, 0x00);
	if(ret != NRF_SUCCESS) return ret;
	
	// Reset the duration to 0:
	ret = accel_write_reg_8(LIS2DH12_INT1_DURATION_ADDR, 0x00);
	if(ret != NRF_SUCCESS) return ret;

	
	// Disbale interrupt 1:
	ret = accel_write_reg_8(LIS2DH12_INT1_CFG_ADDR, 0x00);
	if(ret != NRF_SUCCESS) return ret;
	
	
	
	
	switch(accel_interrupt_event) {
		case ACCEL_NO_INTERRUPT: 
		{
		}
		break;
		

		
		case ACCEL_MOTION_INTERRUPT: 
		{
			// Enable the High-pass filter:
			ret = accel_read_reg_8(LIS2DH12_CTRL_REG2_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
			tmp &= ~(0xF0);		// High pass filter: Normal mode, (CutOff 2Hz@100Hz)
			tmp |= 0x01;		// High pass filter: HP-data enabled for INT1
			ret = accel_write_reg_8(LIS2DH12_CTRL_REG2_ADDR, tmp);	
			if(ret != NRF_SUCCESS) return ret;
			
			// Enable interrupt on INT1 Pin:
			ret = accel_write_reg_8(LIS2DH12_CTRL_REG3_ADDR, 0x40); 
			if(ret != NRF_SUCCESS) return ret;
			
			// Set interrupt 1 latched (So a new interrupt is only then generated if the INT1_SRC-register is read, e.g. by calling accel_reset_interrupt())
			ret = accel_read_reg_8(LIS2DH12_CTRL_REG5_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
			tmp |= (0x08);	// Set LIR_INT1-bit
			ret = accel_write_reg_8(LIS2DH12_CTRL_REG5_ADDR, tmp);
			if(ret != NRF_SUCCESS) return ret;
			
			// Set the motion interrupt parameters (threshold_mg and minimal_duration_ms)
			ret = accel_set_motion_interrupt_parameters(motion_interrupt_parameter_threshold_mg, motion_interrupt_parameter_minimal_duration_ms);
			if(ret != NRF_SUCCESS) return ret;
			
			// Dummy read the reference register to reset the HP-filter:
			ret = accel_read_reg_8(LIS2DH12_REFERENCE_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
			
			// Enable interrupt 1 on High X, Y and Z axis with OR-combination
			ret = accel_write_reg_8(LIS2DH12_INT1_CFG_ADDR, 0x2A);
			if(ret != NRF_SUCCESS) return ret;
			
			// Reset, only in case there was a reset before that was not cleared
			ret = accel_reset_interrupt();
			if(ret != NRF_SUCCESS) return ret;
		}
		break;
		case ACCEL_SINGLE_TAP_INTERRUPT: 
		{
			// TODO: implement/configure the single-tap interrupt generation of the accelerometer
		}
		break;
		
		case ACCEL_DOUBLE_TAP_INTERRUPT: 
		{
			// TODO: implement/configure the double-tap interrupt generation of the accelerometer
		}
		break;
	}
	
	
	interrupt_event = accel_interrupt_event;
	
	return NRF_SUCCESS;
}

ret_code_t 	accel_reset_interrupt(void) {
	// Resetting a latched-interrupt by reading the INT1_SRC-register value
	ret_code_t ret;
	uint8_t tmp;
	ret = accel_read_reg_8(LIS2DH12_INT1_SRC_ADDR, &tmp); // The read value could also be used to get information about the causing axis.
	if(ret != NRF_SUCCESS) return ret;
	
	return NRF_SUCCESS;
}



ret_code_t 	accel_set_fifo(accel_fifo_t accel_fifo) {
	ret_code_t ret;
	uint8_t tmp;
	
	switch(accel_fifo) {
		
		case ACCEL_FIFO_STREAM_ENABLE:
			// Enable the FIFO
			ret = accel_read_reg_8(LIS2DH12_CTRL_REG5_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
			tmp |= (0x40);	// Set FIFO_EN-bit
			ret = accel_write_reg_8(LIS2DH12_CTRL_REG5_ADDR, tmp);
			if(ret != NRF_SUCCESS) return ret;
			
			// Set the FIFO mode to Stream mode
			ret = accel_read_reg_8(LIS2DH12_FIFO_CTRL_REG_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
			tmp &= ~(0xC0);	// Clear the FM1 and FM0 bit
			tmp |= (0x80);	// Set FM1 bit
			ret = accel_write_reg_8(LIS2DH12_FIFO_CTRL_REG_ADDR, tmp);
			if(ret != NRF_SUCCESS) return ret;
		break;
		
		case ACCEL_FIFO_DISABLE:
		default: 
			// Disable the FIFO
			ret = accel_read_reg_8(LIS2DH12_CTRL_REG5_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
			tmp &= ~(0x40);	// Clear FIFO_EN-bit
			ret = accel_write_reg_8(LIS2DH12_CTRL_REG5_ADDR, tmp);
			if(ret != NRF_SUCCESS) return ret;
			
			// Set the FIFO mode to Bypass mode (default)
			ret = accel_read_reg_8(LIS2DH12_FIFO_CTRL_REG_ADDR, &tmp);
			if(ret != NRF_SUCCESS) return ret;
			tmp &= ~(0xC0);	// Clear the FM1 and FM0 bit
			ret = accel_write_reg_8(LIS2DH12_FIFO_CTRL_REG_ADDR, tmp);
			if(ret != NRF_SUCCESS) return ret;
		break;
	}
	
	return NRF_SUCCESS;
}


ret_code_t 	accel_read_acceleration(int16_t* accel_x, int16_t* accel_y, int16_t* accel_z, uint8_t* num_samples, uint32_t max_num_samples) {
	ret_code_t ret;
	*num_samples = 0;
	
	
	// First read out the number of available samples in FIFO through FIFO_SRC_REG
	uint8_t fifo_src_reg;
	ret = accel_read_reg_8(LIS2DH12_FIFO_SRC_REG_ADDR, &fifo_src_reg);
	if(ret != NRF_SUCCESS) return ret;
	
	*num_samples = (fifo_src_reg & 0x1F); // Extract the FSS-bits that contains the number of unread samples stored in FIFO-buffer
	
	// Increment because the maximum number of samples stored in FIFO is 32, but the maximum value in the FIFO_SRC_REG is 31. If there is only one sample available, the value is 0.
	*num_samples += 1;
	if(*num_samples > FIFO_SIZE) {	// Only for safety reasons
		*num_samples = FIFO_SIZE;
	}
	
	if((uint32_t) *num_samples > max_num_samples) {	// Only read maximal max_num_samples from the accelerometer
		*num_samples = (uint8_t) max_num_samples;
	}
	
	// Now read available number of samples from the fifo
	uint8_t tx_buf[FIFO_BUFFER_SIZE];
	tx_buf[0] = LIS2DH12_OUT_X_L_ADDR | 0x80 | 0x40;	// 0x40 for multiple sequential register read
	
	
	
	ret = spi_transmit_receive(&spi_instance, tx_buf, 1, fifo_buf, (*num_samples * 6) + 1);	// *6, because the accelerometer-data is stored in 6 sequential registers (see chapter 9.5 in AN5005).
	// ret could be NRF_SUCCESS, NRF_ERROR_BUSY or NRF_ERROR_INVALID_ADDR
	if(ret == NRF_ERROR_INVALID_ADDR)
		ret = NRF_ERROR_INTERNAL;
	if(ret != NRF_SUCCESS) return ret;
	
	// Get the shift_factor and the sensitivity for the current configuration, to convert the acceleration to mg
	int16_t shift_factor, sensitivity;
	get_shift_factor_and_sensitivity(&shift_factor, &sensitivity, operating_mode, full_scale);
	
	// Set the output values of the acceleration:	
	for(uint16_t i = 0; i < *num_samples; i++) {				
		accel_x[i] = (((int16_t) ((((int16_t)fifo_buf[(i*6+1) + 1]) << 8) | ((int16_t) fifo_buf[(i*6+0) + 1]))) >> shift_factor) * sensitivity;
		accel_y[i] = (((int16_t) ((((int16_t)fifo_buf[(i*6+3) + 1]) << 8) | ((int16_t) fifo_buf[(i*6+2) + 1]))) >> shift_factor) * sensitivity;
		accel_z[i] = (((int16_t) ((((int16_t)fifo_buf[(i*6+5) + 1]) << 8) | ((int16_t) fifo_buf[(i*6+4) + 1]))) >> shift_factor) * sensitivity;
	}
	
	return NRF_SUCCESS;
}



static volatile uint32_t selftest_event_counter = 0;
static void selftest_interrupt_handler(accel_interrupt_event_t const * event) {
	selftest_event_counter ++;
}

bool 		accel_selftest(void) {
	debug_log("ACCEL: Start accel selftest...\n\r");
	
	uint8_t selftest_failed = 0;
	
	// Backup the old configuration:
	accel_operating_mode_t 		former_operating_mode = operating_mode;
	accel_datarate_t			former_datarate	= datarate;
	accel_full_scale_t 			former_full_scale = full_scale;
	accel_HP_filter_t 			former_HP_filter = HP_filter;
	accel_interrupt_event_t		former_interrupt_event = interrupt_event;
	accel_interrupt_handler_t 	former_interrupt_handler = interrupt_handler;
	uint16_t 					former_motion_interrupt_parameter_threshold_mg = motion_interrupt_parameter_threshold_mg;
	uint16_t 					former_motion_interrupt_parameter_minimal_duration_ms = motion_interrupt_parameter_minimal_duration_ms;
	
	
	
	ret_code_t ret;
	
	// Configure for selftest 
	ret = accel_set_operating_mode(ACCEL_LOW_POWER_MODE);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_operating_mode failed!\n\r");
		selftest_failed = 1;
	}
	
	ret = accel_set_datarate(ACCEL_DATARATE_100_HZ);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_datarate failed!\n\r");
		selftest_failed = 1;
	}
	
	ret = accel_set_full_scale(ACCEL_FULL_SCALE_2G);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_full_scale failed!\n\r");
		selftest_failed = 1;
	}
	
	ret = accel_set_HP_filter(ACCEL_HP_FILTER_DISABLE);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_HP_filter failed!\n\r");
		selftest_failed = 1;
	}
	
	ret = accel_set_motion_interrupt_parameters(ACCEL_SELFTEST_INTERRUPT_THRESHOLD_MG, ACCEL_SELFTEST_INTERRUPT_MINIMAL_DURATION_MS);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_motion_interrupt_parameters failed!\n\r");
		selftest_failed = 1;
	}
	
	accel_set_interrupt_handler(selftest_interrupt_handler);
	
	ret = accel_set_interrupt(ACCEL_MOTION_INTERRUPT);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_interrupt failed!\n\r");
		selftest_failed = 1;
	}
	
	ret = accel_reset_interrupt();
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_reset_interrupt failed!\n\r");
		selftest_failed = 1;
	}
	
	debug_log("ACCEL: Waiting for an wake-up interrupt for %u ms.\n\r", ACCEL_SELFTEST_TIME_FOR_INTERRUPT_GENERATION_MS);
	
	// This is the actual test:
	// Waiting for an interrupt:
	uint32_t end_ms = systick_get_continuous_millis() + ACCEL_SELFTEST_TIME_FOR_INTERRUPT_GENERATION_MS;
	selftest_event_counter = 0;	
	while(selftest_event_counter == 0 && systick_get_continuous_millis() < end_ms);

	
	// Reading accelerometer data:
	int16_t x[32] = {0};
	int16_t y[32] = {0};
	int16_t z[32] = {0};
	uint8_t num_samples = 0;	
	ret = accel_read_acceleration(x, y, z, &num_samples, 32);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_read_acceleration failed!\n\r");
		selftest_failed = 1;
	}
	
	
	ret = accel_reset_interrupt();
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_reset_interrupt failed!\n\r");
		selftest_failed = 1;
	}
	
	
	
	// Restore the former configuration 
	ret = accel_set_operating_mode(former_operating_mode);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_operating_mode failed!\n\r");
		selftest_failed = 1;
	}
	
	ret = accel_set_datarate(former_datarate);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_datarate failed!\n\r");
		selftest_failed = 1;
	}
	
	ret = accel_set_full_scale(former_full_scale);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_full_scale failed!\n\r");
		selftest_failed = 1;
	}
	
	ret = accel_set_HP_filter(former_HP_filter);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_HP_filter failed!\n\r");
		selftest_failed = 1;
	}
	
	ret = accel_set_motion_interrupt_parameters(former_motion_interrupt_parameter_threshold_mg, former_motion_interrupt_parameter_minimal_duration_ms);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_motion_interrupt_parameters failed!\n\r");
		selftest_failed = 1;
	}
	
	accel_set_interrupt_handler(former_interrupt_handler);
	
	ret = accel_set_interrupt(former_interrupt_event);
	if(ret != NRF_SUCCESS) {
		debug_log("ACCEL: Accel_set_interrupt failed!\n\r");
		selftest_failed = 1;
	}
	
	
	// Check if the selftest was successful
	if(selftest_failed == 1) {
		debug_log("ACCEL: Accel selftest failed!\n\r");
		return 0;
	}
	
	if(selftest_event_counter == 0 || (x[0] == 0 && y[0] == 0 && z[0] == 0)) {
		debug_log("ACCEL: Accel selftest failed: No interrupt was generated in %u ms OR the accelerometer reading failed!\n\r", ACCEL_SELFTEST_TIME_FOR_INTERRUPT_GENERATION_MS);
		return 0;
	}
	debug_log("ACCEL: x %d, y %d, z %d\n\r", x[0], y[0], z[0]);
	
	debug_log("ACCEL: Accel selftest successful!\n");
	
	return 1;
}


