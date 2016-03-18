/************************************************************
Simple code for mic sampling
************************************************************/

#include <RFduinoBLE.h>

// get Latest time library from https://github.com/PaulStoffregen/Time.git
#include <Time.h>

/************************************************************
Input/output
************************************************************/
const int GREEN_LED_PIN = 6;
const int RED_LED_PIN = 5;
const int SENSOR_PIN = 2; 


int cnt = 0;  // number of bytes written
int snd = 0;  // number of bytes sent


// Setup expected "zero" for the mic value.
// assuming mic has a regulated 2V, on-mic bias is 1/2 (giving us 1V),
//   reference is set to VBG, and prescaling is 1/3.
const int zeroValue = 1023/3.6; 

/************************************************************
Sound
************************************************************/
const int SAMPLE_WINDOW = 32; // how long we should sample
const int SAMPLE_WAIT = 64;   // time between samples - must exceed SAMPLE_WINDOW
unsigned int readingsCount = 0;  //number of mic readings taken \
unsigned long readingsSum = 0;  //sum of mic readings taken      } for computing average

unsigned int THRESHOLD_VALUE = 15;

/************************************************************
BLE
************************************************************/
boolean isConnected = false;
const int PACKET_SIZE = 20;

/************************************************************
Time
************************************************************/

void setup()
{
  // Sets the Reference to 1.2V band gap. Otherwise, if using the VCC as reference
  // the input will change as the input voltage drops (since the mic is connected to a voltage regulator)
  analogReference(VBG); 
  
  //Serial.begin(9600); // turn off serial to save battery

  // BLE settings
  RFduinoBLE.txPowerLevel = 0;
  RFduinoBLE.deviceName = "MiniBadge";  
  RFduinoBLE.advertisementData = "mic";

  RFduinoBLE.advertisementInterval = 100; //Sets the interval to 100ms
  
  // LED
  pinMode(GREEN_LED_PIN, OUTPUT);  
  pinMode(RED_LED_PIN, OUTPUT);
  
  // start the BLE stack
  RFduinoBLE.begin();  

  // setting MIC pin
  pinMode(SENSOR_PIN, INPUT);  
  
  // blink to start
  digitalWrite(GREEN_LED_PIN,HIGH);  
  delay(1000);
  digitalWrite(GREEN_LED_PIN,LOW);
}

void loop()
{
  unsigned long startMillis = millis();  // Start of sample window    
 
  // get reading
  unsigned int micValue = 0;
  micValue = sampleMic();

  // Trigger LED?
  if (micValue > THRESHOLD_VALUE) {
    digitalWrite(GREEN_LED_PIN,HIGH);
  } else {
    digitalWrite(GREEN_LED_PIN,LOW);      
  }

  // clip the reading
  byte reading = micValue <= 255 ? micValue : 255;

  RFduinoBLE.sendByte(reading);
  Serial.println(reading);
  
  // wait between readings (or between empty iterations... save battery)
  int timeSpent = millis() - startMillis;
  timeSpent > 0 ? timeSpent : SAMPLE_WINDOW; // millis() will overflow
  RFduino_ULPDelay(SAMPLE_WAIT - timeSpent); // Wait before sampling again, put RFDuino to sleep  
}

/* Reading mic values */
int sampleMic() {
  unsigned long sampleStartMillis= millis();  // Start of sample window   
  unsigned int sample;
  int adjustedSample;  
  unsigned int count = 0;
  unsigned long sum = 0;
  
  unsigned int micValue = 0;
  unsigned int trimmerValue = 0;
 
  // collect data for XX mS
  while (millis() - sampleStartMillis < SAMPLE_WINDOW && millis() - sampleStartMillis >= 0)
   {
      sample = analogRead(SENSOR_PIN);
      if (sample < 1024) { // toss out spurious readings
            count += 1;
            adjustedSample = sample - zeroValue;  // adjust reading based on "real" zero
            adjustedSample = abs(adjustedSample); //must keep as seperate line.. soemthing todo with abs() implemetnation
            sum = sum + adjustedSample;
      }
   }
   
  // calc average amplitude
  micValue = sum / count;
  
  return micValue;
}

void RFduinoBLE_onConnect() {
  Serial.println("Connected");
  isConnected = true;
}

void RFduinoBLE_onDisconnect() {
  Serial.println("Disconnected");
  isConnected = false;
}


