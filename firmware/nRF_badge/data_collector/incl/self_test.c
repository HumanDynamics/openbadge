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
    if (storer_test()) 
    {
        debug_log("  Success\r\n");
    }
    else
    {
        debug_log("  Failed\r\n");
        while(1) {};
    }
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

    // ====== test button and mic ======
    debug_log("Push button to test button and mic.\r\n");
    while(nrf_gpio_pin_read(BUTTON_1) != 0);
    
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
        
        // turn on green light if button is pressed
        if(nrf_gpio_pin_read(BUTTON_1) == 0)
        {
            nrf_gpio_pin_write(GREEN_LED,LED_ON);
        }
        else {
            nrf_gpio_pin_write(GREEN_LED,LED_OFF);   
        }

        nrf_delay_ms(10);
    }
    
    while(1) {};
    
}