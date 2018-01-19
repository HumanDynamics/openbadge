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

// ====== Accel LIS3DH and SPI functions ======
volatile bool transmission_completed_spi = false;
bool self_test_enable= false;

//=== event handler ====
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

// ==== Init SPI for LIS3DH comunication ====
void accel_spi_init(){
    uint32_t err_code = NRF_SUCCESS;

    //manually handle SS pin
    nrf_gpio_pin_set(SPIM0_SS_PIN_2);
    nrf_gpio_cfg_output(SPIM0_SS_PIN_2);

    // Configure SPI master.
    spi_master_config_t spi_config = {
        SPI_FREQUENCY_FREQUENCY_M8, // Serial clock frequency 8 Mbps.
        SPIM0_SCK_PIN,              // Defined in badge_03v6.h
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

//==== Funcion read registers ====
uint8_t readRegister8(uint8_t reg){
  uint8_t txBuf[1] = {reg | 0x80}; //Array to send
  uint8_t rxBuf[2] = {0,0}; //Array to receive
  spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),rxBuf,2); //Send and receive over SPI protocol
  while(spi_busy()); //Wait while spi is bussy
  return rxBuf[1]; // Value retorned from spi register
}

//==== Funcion write registers and send value ====
void writeRegister8(uint8_t reg, uint8_t value) {
  uint8_t prueba[2] = {reg & ~0x80, value}; // prueba[1] = register to write / prueba[2] = value to send //Array to send
  uint8_t rx[2] = {0,0}; //Array to receive
  spi_master_send_recv(SPI_MASTER_0,prueba,sizeof(prueba),rx,2); //Send and receive over SPI protocol
  while(spi_busy()); //Wait while spi is bussy
}

// ==== Set data rate 400hz ====
void setDataRate(){
  uint8_t ctl1 = readRegister8(LIS3DH_REG_CTRL1);
  ctl1 &= ~(0xF0); // mask off bits
  ctl1 |= (0b0111 << 4); // 0b0111 = 400hz
  writeRegister8(LIS3DH_REG_CTRL1, ctl1);
}

// ==== Init accel test and set movement detection ====
void accel_test(){
    debug_log("Init accelerometer\r\n");
    accel_spi_init();

    //==== Check conection by Whoami ====
    if (readRegister8(LIS2DH_WHO_AM_I_REGISTER) == LIS2DH_WHO_AM_I_VALUE){ //Call howami register
        nrf_gpio_pin_write(GREEN_LED,LED_ON);
        nrf_delay_ms(LED_BLINK_MS);
        nrf_gpio_pin_write(GREEN_LED,LED_OFF);
        nrf_delay_ms(LED_BLINK_MS);
    }
    else {
        nrf_gpio_pin_write(RED_LED,LED_ON);  //turn on LED
    }

    //==== Set basic registers ====
    writeRegister8(LIS3DH_REG_CTRL1, 0x07);// enable all axes, normal mode
    setDataRate();// 400Hz rate
    writeRegister8(LIS3DH_REG_CTRL4, 0x88);// High res & BDU enabled
    writeRegister8(LIS3DH_REG_CTRL3, 0x10);// DRDY on INT1
    writeRegister8(LIS3DH_REG_TEMPCFG, 0x80);// enable adcss

    //==== Set click (sigle tap) ====
    writeRegister8(LIS3DH_REG_CTRL3, 0x80); // turn on int1 click
    writeRegister8(LIS3DH_REG_CTRL5, 0x08); // latch interrupt on int1
    writeRegister8(LIS3DH_REG_CLICKCFG, 0x15); // turn on all axes & tap detection
    writeRegister8(LIS3DH_REG_CLICKTHS, CLICKTHRESHHOLD); // arbitrary
    writeRegister8(LIS3DH_REG_TIMELIMIT, 10); // arbitrary
    writeRegister8(LIS3DH_REG_TIMELATENCY, 20); // arbitrary
    writeRegister8(LIS3DH_REG_TIMEWINDOW, 255); // arbitrary
}

// ==== Set Internal movement dectection and Interrupt activation ====
void init_ACC() {
  // configurations for control registers
  writeRegister8(LIS3DH_REG_CTRL2, 0x00);      // High-pass filter (HPF) enabled
  writeRegister8(LIS3DH_REG_CTRL3, 0x40);      // ACC AOI1 interrupt signal is routed to INT1 pin.
  writeRegister8(LIS3DH_REG_CTRL4, 0x00);      // Full Scale = +/-2 g
  writeRegister8(LIS3DH_REG_CTRL5, 0x00);      // Default value is 00 for no latching. Interrupt signals on INT1 pin is not latched.

  // configurations for wakeup and motionless detection
  writeRegister8(INT1_THS, 0x10); // Threshold interrupt
  writeRegister8(INT1_DURATION, 0x00); // Duration = 1LSBs * (1/10Hz) = 0.1s.
  writeRegister8(INT1_CFG, 0xA0); // Enable ZHIE interrupt generation, AND logic.
  writeRegister8(CTRL_REG1, 0x5C); // Turn on the sensor, Z axis with ODR = 100Hz low power mode.
  writeRegister8(INT1_CFG, 0x82); // Enable XHIE interrupt generation, AND logic.
  writeRegister8(CTRL_REG1, 0x59); // Turn on the sensor, X axis with ODR = 100Hz low power mode.
}

// ==== Tap detection function ====
void tap_accel(){
    //==== Read tap ====
    uint8_t click = readRegister8(LIS3DH_REG_CLICKSRC);
    if (click!=0) { // Movement detected?
      nrf_gpio_pin_write(GREEN_LED,LED_ON);  //turn on LED
    }else{
      nrf_gpio_pin_write(GREEN_LED,LED_OFF);  //turn off LED
     }
}

// ==== Internal motion detection with Interrupt output ====
void motion_interruput(){
    //==== Set Interrupt as Input ====
    nrf_gpio_pin_set(INT_PIN_INPUT);
    nrf_gpio_cfg_input(INT_PIN_INPUT, NRF_GPIO_PIN_PULLUP);
    //==== Interrupt Read ====
    if(nrf_gpio_pin_read(INT_PIN_INPUT)!=0){ //Is eneable interrupt pin?
     nrf_gpio_pin_write(GREEN_LED,LED_ON);  //turn on LED
    } else {
     nrf_gpio_pin_write(GREEN_LED,LED_OFF);  //turn off LED
    }
}

// ==== Accelerometer Internal Self-Test ====
void acc_self_test(){
  while(1){
  if (self_test_enable==false){ //Ask if self-test is eneable
  writeRegister8(LIS3DH_REG_CTRL4, 0x8A); // enable self-test register
  }
  //==== Set Interrupt as Input ====
  nrf_gpio_pin_set(INT_PIN_INPUT);
  nrf_gpio_cfg_input(INT_PIN_INPUT, NRF_GPIO_PIN_PULLUP);

  //==== Interrupt Read ====
  if(nrf_gpio_pin_read(INT_PIN_INPUT)!=0){ //Is eneable interrupt pin? // Blink both LEDs
    nrf_gpio_pin_write(GREEN_LED,LED_ON);
    nrf_gpio_pin_write(RED_LED,LED_ON);
    nrf_delay_ms(LED_BLINK_MS);
    nrf_gpio_pin_write(GREEN_LED,LED_OFF);
    nrf_gpio_pin_write(RED_LED,LED_OFF);
    nrf_delay_ms(LED_BLINK_MS);

    writeRegister8(LIS3DH_REG_CTRL4, 0x88);//Deseable self-test register
    self_test_enable=true;
    break; //Broke While
  } else {
   nrf_gpio_pin_write(GREEN_LED,LED_OFF);  //turn off LED
    }
  }
}

void runSelfTests(){
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

    // ====== Accel ======
    accel_test(); //Test whoiam and set basic registers
    init_ACC(); //Set internal movement detection with Interruput output
    acc_self_test(); //Internal self_test LIS3DH

    // ====== test mic =====
    testMicInit(MIC_ZERO);

    while(1) // stay in infinite loop for test mic and accel
    {
        // ====== Feature Motion detect ======
        //tap_accel(); //For tap detection reading register
        motion_interruput(); //For internal movement dectection reading interrupt pin

        // ====== Feature Mic ======
        testMicAddSample();// update reading
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
