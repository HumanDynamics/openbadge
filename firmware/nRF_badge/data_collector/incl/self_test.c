#include "self_test.h"

/*bool testInternalFlash(void)
{
    // Erase all pages. Check values of all pages
	// read entire flash
	for (int c = FIRST_PAGE; c <= LAST_PAGE; c++)  
    {
        uint32_t* addr = ADDRESS_OF_PAGE(c);  //address of chunk
        debug_log("%d\r\n",sizeof(*addr));

    }
    

    // erase a page
    erasePageOfFlash(FIRST_PAGE);


	// try writing one byte
    uint32_t* addr = ADDRESS_OF_PAGE((FIRST_PAGE));
    ble_flash_word_write( addr, (uint32_t)1);
    addr = ADDRESS_OF_PAGE((FIRST_PAGE));
    return (*addr == 1);
}*/

void testMicAddSample() {
    unsigned int readingsCount = 0;  //number of mic readings taken
    unsigned long readingsSum = 0;  //sum of mic readings taken      } for computing average

    for (int i=0; i<THRESH_SAMPLES; i++) {
        int sample = analogRead(MIC_PIN);
        readingsSum += abs(sample - threshold_buffer.zeroValue);
        readingsCount++;
    }

    unsigned int micValue = readingsSum / readingsCount;
    uint8_t reading = micValue <= 255 ? micValue : 255;  //clip reading

    threshold_buffer.pos = (threshold_buffer.pos + 1) % THRESH_BUFSIZE;
    threshold_buffer.bytes[threshold_buffer.pos] = reading;
    debug_log("added : %d\r\n", reading);
}

void testMicInit(int zeroValue) {
    threshold_buffer.pos = 0;
    threshold_buffer.zeroValue = zeroValue;
    memset(threshold_buffer.bytes, 0, sizeof(threshold_buffer.bytes));
}

uint8_t testMicAvg() {
    int sum = 0;
 
    for ( int i = 0; i < THRESH_BUFSIZE; i++ ) {
        //debug_log("%d,", threshold_buffer.bytes[i]);
        sum += threshold_buffer.bytes[i];
    }
    debug_log("Avg: %d\r\n", sum/THRESH_BUFSIZE);
    return sum/THRESH_BUFSIZE;
}

bool testMicAboveThreshold() {
    uint8_t avg = testMicAvg();
    double avg_with_sd = (avg + THRESH_MAGIC_NUMBER) * THRESH_SD;
    uint8_t lastSample = threshold_buffer.bytes[threshold_buffer.pos];
    //debug_log("avg %d, avg with SD %f, sample %d\r\n", avg, avg_with_sd, lastSample);   
    return lastSample > avg_with_sd;
}

//TODO: move accelerometer code somewhere else
#define SPIM0_SS_PIN_2   2 // accelerometer
#define LIS2DH_WHO_AM_I_REGISTER 		0x0F
#define LIS2DH_WHO_AM_I_VALUE           0x33
volatile bool transmission_completed_spi = false;

void accel_spi_evt_handler(spi_master_evt_t spi_master_evt)
{
    switch(spi_master_evt.evt_type){
		case SPI_MASTER_EVT_TRANSFER_COMPLETED:
			//Transmission done.
			transmission_completed_spi = true;
			//rx_buffer must have received data now.
	    	break;
		default:
			// no implementation is required for now.
		    break;
	}
}

void accel_spi_init()
{
    uint32_t err_code = NRF_SUCCESS;

    //manually handle SS pin
    nrf_gpio_pin_set(SPIM0_SS_PIN_2);
    nrf_gpio_cfg_output(SPIM0_SS_PIN_2);

    // Configure SPI master.
    spi_master_config_t spi_config = {
        SPI_FREQUENCY_FREQUENCY_M8, // Serial clock frequency 8 Mbps.
        SPIM0_SCK_PIN,              // Defined in badge_03.h
        SPIM0_MISO_PIN,
        SPIM0_MOSI_PIN,
        SPIM0_SS_PIN_2,                         //    // SS, we'll handle that manually
        APP_IRQ_PRIORITY_LOW,       // Interrupt priority LOW.
        SPI_CONFIG_ORDER_MsbFirst,  // Bits order ?
        SPI_CONFIG_CPOL_ActiveLow, // Serial clock polarity ACTIVELOW.
        SPI_CONFIG_CPHA_Trailing,    // Serial clock phase TRAILING.
        0                           // Don't disable all IRQs.
    };

    err_code = spi_master_open(SPI_MASTER_0, &spi_config);
    if (err_code != NRF_SUCCESS)  {
        nrf_gpio_pin_write(LED_2,LED_ON);  //turn on LED
        debug_log("Err\r\n");
    }

    // Register event handler for SPI master.
    spi_master_evt_handler_reg(SPI_MASTER_0, accel_spi_evt_handler);
}



void accel_test()
{
    debug_log("Init accelerometer\r\n");
    accel_spi_init();

    debug_log("Asking whoami\r\n");
    transmission_completed_spi = false;

    uint8_t txBuf[1] = {LIS2DH_WHO_AM_I_REGISTER | 0x80};      //call whoami
    uint8_t rxBuf[2] = {0,0};
    uint32_t err_code = spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),rxBuf,2);
	if (err_code != NRF_SUCCESS)
	{
	    //Data transmission failed.
	    nrf_gpio_pin_write(LED_2,LED_ON);  //turn on LED
        debug_log("Err\r\n");
	}

    //while(!transmission_completed_spi);//wait for the data to transfer complete.
    while(spi_busy());
    uint8_t whoami = rxBuf[1];
    if (whoami == LIS2DH_WHO_AM_I_VALUE)
    {
        debug_log("LIS2DH found!\r\n");
    }
    else
    {
        debug_log("LIS2DH not found!\r\n");
        nrf_gpio_pin_write(LED_2,LED_ON);  //turn on LED
    }
}

void runSelfTests()
{
    debug_log("Running self-tests.\r\n");
    
    // Blink both LEDs
    nrf_gpio_pin_write(GREEN_LED,LED_ON);
    nrf_gpio_pin_write(RED_LED,LED_ON);
    nrf_delay_ms(LED_BLINK_MS);
    nrf_gpio_pin_write(GREEN_LED,LED_OFF);
    nrf_gpio_pin_write(RED_LED,LED_OFF);
    nrf_delay_ms(LED_BLINK_MS);
    
    // ====== test internal flash ======
    debug_log("Testing internal flash\r\n");
    nrf_gpio_pin_write(GREEN_LED,LED_ON);

    //TODO: fix test after we decouple the storer from flash management. See bug on github
    debug_log("... Skipping test\r\n");
    /*
    if (storer_test()) 
    {
        debug_log("  Success\r\n");
    }
    else
    {
        debug_log("  Failed\r\n");
        while(1) {};
    }
    */
    nrf_delay_ms(LED_BLINK_MS);
    nrf_gpio_pin_write(GREEN_LED,LED_OFF);
    nrf_delay_ms(LED_BLINK_MS);
    
    // ====== test external flash ======
    debug_log("Testing external flash\r\n");
    nrf_gpio_pin_write(GREEN_LED,LED_ON);

    // read/write
    if (testExternalEEPROM()) {
        debug_log("  Success\r\n");
    }
    else{
        debug_log("  Failed\r\n");
        while(1) {};
    }

    nrf_delay_ms(LED_BLINK_MS);
    nrf_gpio_pin_write(GREEN_LED,LED_OFF);
    nrf_delay_ms(LED_BLINK_MS);

    // ====== test mic ======
    testMicInit(MIC_ZERO);
    while(1) // stay in infinite loop, spit out mic values
    {
        // update reading
        testMicAddSample();
        
        if (testMicAboveThreshold()) {
            nrf_gpio_pin_write(RED_LED,LED_ON);
            nrf_delay_ms(100);                  
        }
        else {
            nrf_gpio_pin_write(RED_LED,LED_OFF);   
        }

        nrf_delay_ms(10);
    }
    
    while(1) {};
    
}