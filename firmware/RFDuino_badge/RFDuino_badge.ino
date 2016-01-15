#include <RFduinoBLE.h>

// get Latest time library from https://github.com/PaulStoffregen/Time.git
#include <Time.h>

#include <Lazarus.h>
// from http://forum.rfduino.com/index.php?topic=801.0
// to allow BLE events to break from the ULPDelay
Lazarus Lazarus;

// class for dealing with flash storage
#include "flashHandling.h"
FlashHandler Flash = FlashHandler();

//============================= I/O-related stuff ======================================
//======================================================================================
const int GREEN_LED_PIN = 6;
const int RED_LED_PIN = 5;
const int MIC_PIN = 2;

// Setup expected "zero" for the mic value.
// assuming mic has a regulated 2V, on-mic bias is 1/2 (giving us 1V),
//   reference is set to VBG, and prescaling is 1/3.
const int zeroValue = 1023.0 / 3.6;


//============================ sound-related stuff =====================================
//======================================================================================
const unsigned long SAMPLE_WINDOW = 100; // how long we should sample
const unsigned long SAMPLE_PERIOD = 250;   // time between samples - must exceed SAMPLE_WINDOW
unsigned int readingsCount = 0;  //number of mic readings taken \
unsigned long readingsSum = 0;  //sum of mic readings taken      } for computing average
unsigned long sampleStart = 0;  //beginning of sample period
boolean storedSample = false;  //whether we've stored the sample from the current sampling period

//============================ BLE/data-related stuff ==================================
//======================================================================================
volatile boolean BLEconnected = false;  //whether we're currently connected
volatile boolean sendStatus = false;  //flag signaling that the server asked for status

//Word buffer: mic data is collected in bytes, but data must be stored to flash as 4byte words
unsigned char wordIndex = 0;  //next place in word buffer to be written to
union  {  //buffer for storing bytes of data to flash (flash writes must be whole 32-bit words)
  unsigned char c[4];  //chars
  unsigned long l;     //long
} wordBuf;

char* CLOCK_STATES[] = {"CLKUN","CLKSYN"};
unsigned int clock_state = 0;
boolean pendingBLEreset = false; // if reset is pending, reset when possible

//============================ time-related stuff ======================================
//======================================================================================
volatile int dateReceived = false; // signifies whether or not the RFduino has received a date
volatile unsigned long dataTimestamp;    // holds the date

boolean sleep = false;  //whether we should sleep (so actions like data sending can override sleep)


//==================================== setup ================================================
//===========================================================================================

void setup()  {
  // Sets the Reference to 1.2V band gap. Otherwise, if using the VCC as reference
  // the input will change as the input voltage drops
  // (since the mic is connected to a voltage regulator)
  analogReference(VBG);

  //Serial.begin(9600); // comment to save power

  // LED
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Microphone
  pinMode(MIC_PIN, INPUT);

  // blink twice to start (different than other versions)
  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(1000);
  digitalWrite(GREEN_LED_PIN, LOW);
  delay(1000);
  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(1000);
  digitalWrite(GREEN_LED_PIN, LOW);

  Flash.samplePeriod = SAMPLE_PERIOD;  //kludgey, give Flash access to SAMPLE_PERIOD

  //=========================== Resume after reset ================================
  /*We need to find where we left off - what was the last chunk we wrote, and the last sent?
    If a chunk is completed but not sent, the last 4 bytes match the first 4 - the timestamp
    If a chunk has been sent, the last 4 bytes are all 0
    Otherwise, a chunk is not valid (probably reset midway through chunk)
  */
  digitalWrite(RED_LED_PIN, HIGH);  //red LED says we're intializing flash
  Serial.println("\n\n\nInitializing...");

  //printAllChunks();  //print the timestamps and checks for every chunk, for debugging

  //We need to find the latest stored chunk, to start storing after it
  //  and the earliest unsent chunk, to start sending from it
  unsigned long lastStoredTime = MODERN_TIME;  //ignore obviously false timestamps from way in past
  int lastStoredChunk = 0;
  unsigned long earliestUnsentTime = 0xffffffffUL;  //start at any timestamp found
  int earliestUnsentChunk = 0;

  for (int c = 0; c <= lastChunk; c++)  {
    uint32_t* addr = ADDRESS_OF_CHUNK(c);  //address of chunk
    uint32_t timestamp = *addr;  //timestamp of chunk
    uint32_t chunkCheck = *(addr + WORDS_PER_CHUNK - 1);  //last word of chunk, the check

    if (timestamp != 0xffffffffUL && timestamp > MODERN_TIME)  { //is the timestamp possibly valid?
      if (timestamp == chunkCheck || chunkCheck == 0)  { //is it a completely stored chunk?
        if (timestamp > lastStoredTime)  { //is it later than the latest one so far?
          lastStoredChunk = c; //keep track of latest stored chunk
          lastStoredTime = timestamp;
        }
        if (chunkCheck != 0)  { //if it's completely stored, is it also unsent?
          if (timestamp < earliestUnsentTime)  {  //is it earlier than the earliest so far?
            earliestUnsentChunk = c; //keep track of earliest unsent chunk
            earliestUnsentTime = timestamp;
          }
        }
      }  //end of if("completetely stored")
    }  //end of if("timestamp is valid")
  }  //end of for loop


  Serial.print("Last stored chunk: ");
  Serial.print(lastStoredChunk);
  Serial.print(" at time: ");
  setTime(lastStoredTime);
  Serial.printf("%d:%d:%d %d-%d-%d\n", hour(), minute(), second(), month(), day(), year());

  Flash.initStorageFromChunk(lastStoredChunk);  //initialize storage handler


  if (earliestUnsentTime == 0xffffffffUL)  { //no completely stored, unsent chunks found
    Serial.println("No complete unsent chunks");
    earliestUnsentChunk = lastStoredChunk; //start where we're starting storage
  }
  else  {  //toSend is the earliest unsent chunk
    Serial.print("Earliest unsent chunk: ");
    Serial.print(earliestUnsentChunk);
    Serial.print(" from time: ");
    setTime(earliestUnsentTime);
    Serial.printf("%d:%d:%d %d-%d-%d\n", hour(), minute(), second(), month(), day(), year());
  }

  Flash.initSendingFromChunk(earliestUnsentChunk);

  digitalWrite(RED_LED_PIN, LOW);  //done initializing

  //=========================== End of resume after reset handler ====================


  // BLE settings
  RFduinoBLE.txPowerLevel = 0;
  RFduinoBLE.deviceName = "MiniBadge";
  RFduinoBLE.advertisementData = CLOCK_STATES[clock_state];
  RFduinoBLE.advertisementInterval = 200; //Sets the interval to 200ms

  // start the BLE stack
  RFduinoBLE.begin();

  Serial.println("Done with setup()");

}

//==================================== loop =================================================
//===========================================================================================

void loop()  {
  //================ Sampling/Sleep handler ================
  sleep = true;  //default to sleeping at the end of loop()

  if (dateReceived)  {  //don't start sampling unless we have valid date
    if (millis() - sampleStart <= SAMPLE_WINDOW)  {   //are we within the sampling window
      addMicReading();  //add to total, increment count
      sleep = false;  //don't sleep, we're sampling
    }
    else if (millis() - sampleStart >= SAMPLE_PERIOD)  {  //if not, have we completed the sleep cycle
      sampleData();  //add average of readings to sample array
      sampleStart = millis();  //mark start of sample period, resetting cycle
      sleep = false;  //we're done sleeping
    }
    else  {  //otherwise we should be sleeping
      sleep = true;
    }
  }

  //================== Sending Status ====================
  if (sendStatus)  {  //triggered from onReceive 's'
    if (dateReceived)  {
      if (Flash.unsentChunkReady())  {  //are there any chunks ready to be sent
        while (!RFduinoBLE.send('d'));  //synced, and there's data to be sent
      }
      else  {
        while (!RFduinoBLE.send('s'));  //synced, but there's no data ready yet
      }
    }
    else  {
      while (!RFduinoBLE.send('n'));  //not synced
    }
    sendStatus = false;
    Serial.println("Sent Status");
  }


  //================= Flash storage/sending handler ================
  if (Flash.updateStorage())  { //do flash storage stuff (ensure storage buffer gets stored to flash)
    sleep = false;  //don't sleep if there is buffered data to store to flash
  }

  if (Flash.updateSending())  { //do BLE sending stuff (send data if sending enabled)
    sleep = false;  //don't sleep if there's data to be sent
  }

  //============== Reset BLE, if we're supposed to =============
  if (pendingBLEreset) {
    Serial.println("Resetting BLE");
    RFduinoBLE.end();
    RFduinoBLE.advertisementData = CLOCK_STATES[clock_state];
    RFduinoBLE.begin();
    pendingBLEreset = false;    
  }

  //============== Sleep, if we're supposed to =================
  if(!dateReceived)  {  //if we're not synced at all yet, just sleep.  save power.
    RFduino_ULPDelay(SAMPLE_PERIOD);
  }
  else if (sleep)  {
    unsigned long elapsed = millis() - sampleStart;
    if (elapsed < SAMPLE_PERIOD)  {  //avoid wraparound in delay time calculation
      RFduino_ULPDelay(SAMPLE_PERIOD - elapsed);
    }
    if (Lazarus.lazarusArising())  {
      Serial.println("Arisen");
    }
  }

}



//=========================== Global function definitions ==================================
//==========================================================================================

/* Reading mic values */
int addMicReading() {
  int sample = analogRead(MIC_PIN);
  readingsSum += abs(sample - zeroValue);
  readingsCount++;
}

/* read and store data samples */
void sampleData()  {
  unsigned int micValue = readingsSum / readingsCount;
  //Serial.print("count: ");
  //Serial.println(readingsCount);
  byte reading = micValue <= 255 ? micValue : 255;  //clip reading
  readingsCount = 0;
  readingsSum = 0;
  if (dateReceived)  {
    wordBuf.c[wordIndex] = reading;
    if (wordIndex == 0)  {
      dataTimestamp = now(); // keep dataTimestamp aligned with first reading in array
    }
    wordIndex++;  //advance
    if (wordIndex > 3)  {  //reset and store word if we fill buffer
      wordIndex = 0;
      Flash.storeWord(wordBuf.l, dataTimestamp);
    }
  }
}

//================================== BLE event callbacks ====================================
//===========================================================================================

void RFduinoBLE_onConnect() {
  Serial.println("Connected");
  Lazarus.ariseLazarus();  //break from ULPDelay
  Flash.disableStorage();  //don't manipulate flash while BLE is busy
}

void RFduinoBLE_onDisconnect() {
  Serial.println("Disconnected");
  Lazarus.ariseLazarus();  //break from ULPDelay
  Flash.disableSending();  // stop sending
  Flash.enableStorage();  //continue storage operations now that connection is ended

  // tell loop to reset BLE with new advertisement message
  // only if date hasn't been set yet
  if (dateReceived && clock_state == 0) {
    Serial.println("Setting advertisement to CLOCK_SYNCED");
    pendingBLEreset = true; // telling the loop to reset BLE
    clock_state = 1;
  }
}

/* Set the date if date received, else if 'd' sent dump data */
void RFduinoBLE_onReceive(char *input, int len) {
  Serial.println("Received");
  if (len > 2)  {  //is it longer than 1 character+null char: i.e. a timestamp.
    Serial.println("Getting date");
    unsigned long f =  readLong(input);
    setTime(f);
    Serial.printf("%d:%d:%d %d-%d-%d\n", hour(), minute(), second(), month(), day(), year());
    Flash.disableSending();

    // mark date as recieved    
    dateReceived = true;
    
    Serial.println("Finished setting up date");
  }
  else if (input[0] == 's')  {
    Serial.println("Got status request");
    sendStatus = true;
  }
  else if (input[0] == 'd')  {
    Serial.println("Got data request");
    if (dateReceived && Flash.unsentChunkReady())  {
      Flash.enableSending();
    }
  }
  else  {
    Serial.println("Unknown receive");
  }
  Lazarus.ariseLazarus();  //break from ULPDelay
}



//====================================== misc ==========================================
//======================================================================================

// Read long (expects little endian)
unsigned long readLong(char *a) {
  unsigned long retval;
  retval  = (unsigned long) a[3] << 24 | (unsigned long) a[2] << 16;
  retval |= (unsigned long) a[1] << 8 | a[0];
  return retval;
}

void printAllChunks()  {
  for (int c = 0; c <= lastChunk; c++)  {
    uint32_t* addr = ADDRESS_OF_CHUNK(c);  //address of chunk
    uint32_t timestamp = *addr;  //timestamp of chunk
    uint32_t chunkCheck = *(addr + 31);  //last word of chunk, the check
    Serial.print(c);   //Print results of this search, for debugging purposes.
    Serial.print(' ');
    Serial.print(int(addr));
    Serial.print(' ');
    Serial.print(timestamp);
    Serial.print(' ');
    Serial.println(chunkCheck);
  }
  Serial.flush(); //empty send buffer
}

