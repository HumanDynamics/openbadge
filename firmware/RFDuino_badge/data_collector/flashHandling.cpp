/*
 * Classes for handling storage to flash, and retreival of data from flash for sending
 * FlashHandler simplifies writing to flash.  The program may always use storeWord() to
 *   store data; update(), if called repeatedly, handles the actual storage and organization
 *   in flash, including avoiding conflict with BLE activity.
 * RetrieveHandler simplifies sending data over BLE
 */



#include <RFduinoBLE.h>
#include <Arduino.h>
#include "flashHandling.h"


FlashHandler::FlashHandler()  {
  //set things to default values
  _storageEnabled = false;
  _bufIn = _bufOut = 0;
  _chunk = _chunkTimestamp = _storeLoc = 0;

  _sendingEnabled = false;
  _toSend = _sendLoc = 0;
  _sentHeader = false;

  _panicLEDpin = 5;  //red LED for panicking
  samplePeriod = 0;  //sample period.  needs to be set externally because it isn't stored in flash
}


//=======================  Storage-related function definitions ============================
//==========================================================================================

//On restart, we need to make sure a page is prepped to have data stored to it
//but don't want to lose old data, e.g. :
// [                               Page 251                              ]
// [(CHNK7 )( CHNK6 )( CHNK5 )( CHNK4 )( CHNK3 )( CHNK2 )( CHNK1 )(CHNK0)]
//  \  need to be cleared   /  \start/  \   old, perhaps unsent, data  /
boolean FlashHandler::initStorageFromChunk(int chunk)  {
  if (chunk < 0 || chunk > lastChunk)  {
    return false;  //return false if chunk number invalid
  }
  //set the initial chunk.  This one is full, storeWord will move to the next one
  _chunk = chunk;
  uint32_t* chunkAddr = ADDRESS_OF_CHUNK(_chunk);
  _storeLoc = 0;
  unsigned char chunkPage = PAGE_OF_CHUNK(_chunk);
  uint32_t* chunkPageAddr = ADDRESS_OF_PAGE(chunkPage);  //start address of _chunk's page
  //if it's the last chunk in page, no need to do anything here, storeWord will clear the next page
  if (chunkAddr != chunkPageAddr)  {
    int spaceToErase = int(chunkAddr) - int(chunkPageAddr); //how much of page we need to clear
    unsigned char temp[PAGE_SIZE];  //temporary buffer for page
    memcpy(temp, chunkPageAddr, PAGE_SIZE); //copy old (possibly unsent) data to buffer
    memset(temp, 0xff, spaceToErase);  //erase old data from buffer (flash bits erase to all 1's)
    _erasePage(chunkPage);  //erase page
    _writeBlock(chunkPageAddr, temp, PAGE_SIZE);  //replace data from buffer
  }
  return true;
}

//enable and disable flash storage.  Don't enable if sending is enabled
void FlashHandler::disableStorage()  {
  _storageEnabled = false;
}
boolean FlashHandler::enableStorage()  {
  if (_sendingEnabled)  { //don't do storage if sending is enabled
    Serial.println("ERR: can't store while sending");
    return false;
  }
  else  {
    _storageEnabled = true;
    return true;
  }
}

//Adds a long's worth of data to storage buffer.  Returns false and does nothing if buffer is full
boolean FlashHandler::storeWord(uint32_t value, unsigned long timestamp)  {
  int next = (_bufIn + 1) % BUFSIZE;
  if (next == _bufOut)  { //did we wrap around and fill buffer
    Serial.println("ERR: storeBuf ovf");
    return false;
  }
  else  {
    //put data and its timestamp into buffer
    _storeBuf[_bufIn] = value;
    _storeBufTS[_bufIn] = timestamp;
    _bufIn = next;  //increment to next place in buffer
  }
}

//Must be called repeatedly to ensure storage buffer is transferred to flash
//Returns true if there was data to send, false if all good.
boolean FlashHandler::updateStorage()  {
  if (_storageEnabled && _bufOut != _bufIn)  { //is storage enabled/is there data in buffer
    RFduinoBLE.end();  //don't use BLE while manipulating flash
    if (_storeLoc == 0)  {  //if loc is back at beginning, move to next chunk
      _startNewChunk(_storeBufTS[_bufOut]);
    }
    uint32_t* chunkAddr = ADDRESS_OF_CHUNK(_chunk);
    _writeWord(chunkAddr + _storeLoc, _storeBuf[_bufOut]);  //store the word from buffer
    _bufOut = (_bufOut + 1) % BUFSIZE; //increment outgoing index of ring buffer
    Serial.print("loc: ");
    Serial.println(_storeLoc);
    _storeLoc++;
    if (_storeLoc >= WORDS_PER_CHUNK - 1)  { //if we finished a chunk, mark it as complete
      _writeWord(chunkAddr + _storeLoc, _chunkTimestamp); //mark as complete: write timestamp to end
      _storeLoc = 0;
    }
    RFduinoBLE.begin();
  }
  return (_storageEnabled && _bufOut != _bufIn);  //return true if there's still stuff in the buffer
}

//Mark a chunk as sent.  Returns true if successful (i.e. storage enabled)
void FlashHandler::_markSent(int chunk)  {
  _writeWord(ADDRESS_OF_CHUNK(chunk) + WORDS_PER_CHUNK - 1, 0UL);  //set last word of chunk to 0
}

int FlashHandler::currentStoreChunk()  {
  return _chunk;
}

unsigned long FlashHandler::currentStoreTime()  {
  return _chunkTimestamp;
}

// intializes a new chunk, erasing a new page if necessary, and writing timestamp and battery header
void FlashHandler::_startNewChunk(unsigned long timestamp)  {
  unsigned char oldPage = PAGE_OF_CHUNK(_chunk);  //the page of the complete chunk
  unsigned char newPage;  //page of the new chunk, to be erased if it's a new page
  _chunk = (_chunk < lastChunk) ? _chunk + 1 : 0; //advance to next chunk
  uint32_t* chunkAddr = ADDRESS_OF_CHUNK(_chunk);
  newPage = PAGE_OF_CHUNK(_chunk);  //page of the new chunk
  if (newPage != oldPage)  {  // do we have to start a new page
    //If storage wraps all the way around available flash, we need to bump toSend forward
    //  till it's out of the page we're erasing, so we still send from the oldest stored data
    while (PAGE_OF_CHUNK(_toSend) == newPage)  {
      _toSend = (_toSend < lastChunk) ? _toSend + 1 : 0;
    }
    _erasePage(newPage);  //erase new page
    Serial.print("Started page: ");
    Serial.println(int(newPage));
  }
  //write the header info (sample period is constant, so we don't need to store that)
  _chunkTimestamp = timestamp;
  _writeWord(chunkAddr + _storeLoc, _chunkTimestamp);
  _storeLoc++;  //next long address
  long battery = float2Long(_readBattery());
  _writeWord(chunkAddr + _storeLoc, battery);
  _storeLoc++;  //next long address
  Serial.print("Started chunk: ");
  Serial.println(_chunk);
}

// If we try an illegal flash write, something's very wrong, so we should not continue.
void FlashHandler::_writeWord(uint32_t* addr, uint32_t val)  {
  int result = flashWrite(addr, val);  //from RFDuino Memory.h
  if (result == 1)  {
    Serial.println("Tried writing to softdevice page");
  }
  else if (result == 2)  {
    Serial.println("Tried writing to program space");
  }
  _panic(result);
}

void FlashHandler::_writeBlock(void* to, const void* from, int numBytes)  {
  int result = flashWriteBlock(to, from, numBytes);  //from RFDuino Memory.h
  if (result == 1)  {
    Serial.println("Tried writing block to softdevice page");
  }
  else if (result == 2)  {
    Serial.println("Tried writing block to program space");
  }
  _panic(result);
}

void FlashHandler::_erasePage(uint8_t page)  {
  while (RFduinoBLE_radioActive); //don't erase while BLE active
  int result = flashPageErase(page);  //from RFDuino Memory.h
  if (result == 1)  {
    Serial.println("Tried erasing softdevice page");
  }
  else if (result == 2)  {
    Serial.println("Tried erasing program space");
  }
  _panic(result);
}


//=======================  Sending-related function definitions ============================
//==========================================================================================

//initialize sending from a chunk; if sending is enabled, updateSending will start sending chunk
boolean FlashHandler::initSendingFromChunk(int chunk)  {
  if (chunk < 0 || chunk > lastChunk)  {
    return false;  //return false if chunk number invalid
  }
  _toSend = chunk;
  _sentHeader = false;
  _sendLoc = 0;
  return true;
}

//returns true if there's an unsent chunk ready
boolean FlashHandler::unsentChunkReady()  {
  if (_findNextUnsentChunk() != -1)  {
    return true;
  }
  return false;  //no unsent chunks found
}

boolean FlashHandler::updateSending()  {
  if (_sendingEnabled)  {
    if (_sentHeader == false)  {
      _sendHeader();  //try to send header, if successful, _sentHeader is now true
    }
    else  {  //else "if _sentHeader==true"
      // send data. The max buffer size to send it 20 byte
      // so we'll send up to 20 at a time
      if (_sendLoc < (WORDS_PER_CHUNK - 1)) { // have we sent all actual data in the chunk?
        Serial.print("pkt TS:");
        Serial.print(_toSend);
        Serial.print(" SL:");
        Serial.println(_sendLoc);
        int wordsLeft = (WORDS_PER_CHUNK - 1) - _sendLoc;  //last word is timestamp, not data
        int wordsToSend = (wordsLeft > WORDS_PER_PACKET) ? WORDS_PER_PACKET : wordsLeft;
        long buf[wordsToSend];  // buffer to hold one packet of data to be sent
        memset(buf, 0, sizeof(buf));
        memcpy(buf, ADDRESS_OF_CHUNK(_toSend) + _sendLoc, sizeof(buf));
        if (RFduinoBLE.send((char*)(buf), sizeof(buf)))  {  //can we send now?  (BLE not busy)
          _sendLoc += wordsToSend;  //if we sent, advance forward thru toSend chunk
        }
      }
      else  {
        _markSent(_toSend);
        int nextChunk = _findNextUnsentChunk();
        if (nextChunk != -1)  { //are there more unsent chunks ready?
          initSendingFromChunk(nextChunk);
          return true;
        }
        else  {
          disableSending();  //if we've sent all ready chunks, stop sending.
        }
      }
    } //end of else "if _sentHeader==true"
  }  //end of if(_sendingEnabled)
  return false;  //return false if sending disabled, or if there's no more sending to be done
}

//Start or stop sending chunks over BLE.  Don't start if we're currently storing
void FlashHandler::disableSending()  {
  _sendingEnabled = false;
}
boolean FlashHandler::enableSending()  {
  if (_storageEnabled)  {
    Serial.println("ERR: can't send while storing");
    return false;  //sending not enabled because storage is active
  }
  else  {
    if (_sendingEnabled == false)  { //don't reset stuff if we're already sending
      int nextChunk = _findNextUnsentChunk();
      if (nextChunk == -1)  { //no unsent chunks ready
        Serial.println("ERR: nothing to send");
        return false;  //sending not enabled because there's nothing to send
      }
      else  {
        initSendingFromChunk(nextChunk);  //Initialize sending from that chunk
        _sendingEnabled = true;
      }
    }
    return true;  //sending enabled
  }
}


//Find the next ready unsent chunk in flash, or return -1 if there are none.
int FlashHandler::_findNextUnsentChunk()  {
  int lookChunk = _toSend;
  while (lookChunk != _chunk)  { //look until we're caught up with storage
    uint32_t* lookAddr = ADDRESS_OF_CHUNK(lookChunk);
    unsigned long lookTimestamp = *lookAddr;
    unsigned long lookCheck = *(lookAddr + WORDS_PER_CHUNK - 1);
    //is the chunk valid, i.e. not apparently from past or future
    if (lookTimestamp > MODERN_TIME && lookTimestamp < _chunkTimestamp)  {
      if (lookTimestamp == lookCheck)  { //is it marked as completely stored
        return lookChunk;  //found a valid unsent chunk
      }
    }
    lookChunk = (lookChunk < lastChunk) ? lookChunk + 1 : 0; //look at next chunk
  }
  return -1;  //no unsent chunks found
}

// sends the header in a packet over BLE.  returns false and does nothing if BLE is busy
void FlashHandler::_sendHeader()  {
  Serial.print("Send header...");
  uint32_t* toSendAddr = ADDRESS_OF_CHUNK(_toSend);
  // send date
  char dateAsChars[4];
  long2Chars(*toSendAddr, dateAsChars);

  // send voltage
  //float batteryVoltage = readBattery();  stored in flash data
  char batAsChars[4];
  long2Chars(*(toSendAddr + 1), batAsChars);

  // send sample delay
  char delayAsChars[2];
  short2Chars(samplePeriod, delayAsChars);
  // pack and send
  char header[10];
  memcpy(header, dateAsChars, 4);
  memcpy(header + 4, batAsChars, 4);
  memcpy(header + 8, delayAsChars, 2);
  if (RFduinoBLE.send(header, sizeof(header)))  {  //only sends if BLE isn't busy
    Serial.println("OK.");
    _sendLoc = 2;  //should now start sending from 3rd word in chunk
    _sentHeader = true;
  }
  else  {
    Serial.println("busy.");  //BLE was busy
    _sentHeader = false;
  }
}





//============================= misc function definitions ==================================
//==========================================================================================

// Converts a short to an array of chars
void FlashHandler::short2Chars(short val, char* chars_array) {
  // Create union of shared memory space
  union {
    short short_variable;
    char temp_array[2];
  } u;
  u.short_variable = val;  // Overwrite bytes of union with float variable
  memcpy(chars_array, u.temp_array, 2);  // Assign bytes to input array
}

// Converts a float to an array of chars
void FlashHandler::float2Chars(float val, char* chars_array) {
  // Create union of shared memory space
  union {
    float float_variable;
    char temp_array[4];
  } u;
  u.float_variable = val;  // Overwrite bytes of union with float variable
  memcpy(chars_array, u.temp_array, 4);  // Assign bytes to input array
}

// Converts a long to an array of chars
void FlashHandler::long2Chars(long val, char* bytes_array) {
  // Create union of shared memory space
  union {
    long long_variable;
    char temp_array[4];
  } u;
  u.long_variable = val;  // Overwrite bytes of union with float variable
  memcpy(bytes_array, u.temp_array, 4);  // Assign bytes to input array
}

//Converts a float to a long with same bytes
unsigned long FlashHandler::float2Long(float val) {
  // Create union of shared memory space
  union {
    float float_variable;
    unsigned long long_variable;
  } u;
  u.float_variable = val;  // Overwrite bytes of union with float variable
  return u.long_variable;  // Return long version of float
}


//Halt the program if doWePanic == true
void FlashHandler::_panic(int doWePanic)  {
  if (doWePanic)  {
    Serial.println("Halting...");
    while (1)  {  //something's unresolvably wrong, we tried to erase out of bounds
      digitalWrite(_panicLEDpin, !digitalRead(_panicLEDpin));  //toggle pin
      delay(50);
    }
  }
}

/* Read battery voltage */
float FlashHandler::_readBattery() {
  analogSelection(VDD_1_3_PS);  //Selects VDD with 1/3 prescaling as the analog source
  int sensorValue = analogRead(1); // the pin has no meaning, it uses VDD pin
  float batteryVoltage = sensorValue * (3.6 / 1023.0); // convert value to voltage

  // reset analog source to the default: analog inputs with 1/3 prescaling
  analogSelection(DEFAULT_INPUT_SEL);
  return batteryVoltage;
}


