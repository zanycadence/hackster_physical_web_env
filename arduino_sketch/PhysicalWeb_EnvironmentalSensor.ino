/*PhysicalWeb_EnvironmentalSensor
 * Combines a dust sensor, soil sensors, and temp/hum
 * sensor into one BLE device that also broadcasts an eddystone
 * beacon address for use with the physical web.
 * 
 * 
 * 
 * 
 * Based off of Eddystone Beacon Code from: 
 * Copyright (c) 2016 Bradford Needham, North Plains, Oregon, USA
 * @bneedhamia, https://www.needhamia.com
 * Licensed under the Apache 2.0 License, a copy of which
 * should have been included with this software.
 */

/*
 * Currently doesn't work on 2.0.2, works well on 1.0.7 board library
 */
#include <CurieBLE.h>  

//DHT library for temp/hum sensor
#include "DHT.h"

/*
 * Pinout:
 *   PIN_BLINK = digital Output. The normal Arduino 101 LED.
 *     If everything is working, the LED blinks.
 *     If instead there is a startup error, the LED is solid.
 */
const int PIN_BLINK = 13;        // Pin 13 is the on-board LED

/*
 * The (unencoded) URL to put into the
 * Eddystone-URL beacon frame.
 * 
 * NOTE: the encoded length must be under 18 bytes long.
 * See initEddystoneUrlFrame().
 * 
 * For use with Physical Web app, url must be https://
 *
 * NOTE: This Sketch supports only lower-case URLs.
 * See initEddystoneUrlFrame().
 */
const char* MY_URL = "https://goo.gl/Z5hA1b";

/*
 * The reported Tx Power value to put into the Eddystone-URL beacon frame.
 * 
 * From the Eddystone-URL Protocol specification
 * (https://github.com/google/eddystone/blob/master/eddystone-url/README.md#tx-power-level)
 * "the best way to determine the precise value
 * to put into this field is to measure the actual output
 * of your beacon from 1 meter away and then add 41dBm to that.
 * 41dBm is the signal loss that occurs over 1 meter."
 * 
 * I used the Nordic nRF Master Control Panel Android app
 * (https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en)
 * to measure the beacon power at 1 meter.
 */
const int8_t TX_POWER_DBM = (-70 + 41);

/*
 * Maximum number of bytes in an Eddystone-URL frame.
 * The Eddystone-URL frame contains
 * the frame type, Tx Power, Url Prefix, and up to 17 bytes of Url.
 * (The spec isn't completely clear.  That might be 18 rather than 17.)
 */
const uint8_t MAX_URL_FRAME_LENGTH = 1 + 1 + 1 + 17;

/*
 * Eddystone-URL frame type
 */
const uint8_t FRAME_TYPE_EDDYSTONE_URL = 0x10;

/*
 * Eddystone-URL url prefix types
 * representing the starting characters of the URL.
 * 0x00 = http://www.
 * 0x01 = https://www.
 * 0x02 = http://
 * 0x03 = https://
 */
const uint8_t URL_PREFIX_HTTP_WWW_DOT = 0x00;
const uint8_t URL_PREFIX_HTTPS_WWW_DOT = 0x01;
const uint8_t URL_PREFIX_HTTP_COLON_SLASH_SLASH = 0x02;
const uint8_t URL_PREFIX_HTTPS_COLON_SLASH_SLASH = 0x03;

/*
 * eddyService = Our BLE Eddystone Service.
 * See https://developer.bluetooth.org/gatt/services/Pages/ServicesHome.aspx 
 * and https://github.com/google/eddystone/blob/master/protocol-specification.md
 * NOTE: as of this writing, Eddystone doesn't seem to be part of the Standard BLE Services list.
 * 
 */
BLEService eddyService("FEAA");

BLEPeripheral ble;              // Root of our BLE Peripheral (server) capability
BLEService dustService("00001234-0000-1000-8000-00805f9b34fb");
BLEFloatCharacteristic dustCharacteristic("00001234-0000-1000-8000-00805f9b34fc", BLERead | BLENotify);

BLEService soilService("00001234-0000-1000-8000-00805f9b34fd");
BLEIntCharacteristic soilCharacteristic1("00001234-0000-1000-8000-00805f9b34fe", BLERead | BLENotify);
BLEIntCharacteristic soilCharacteristic2("00001234-0000-1000-8000-00805f9b34ff", BLERead | BLENotify);

BLEService tempHumService("00001234-0000-1000-8000-00805f9b3501");
BLEFloatCharacteristic tempCharacteristic("00001234-0000-1000-8000-00805f9b3502", BLERead | BLENotify);
BLEFloatCharacteristic humCharacteristic("00001234-0000-1000-8000-00805f9b3503", BLERead | BLENotify);


/*
 * Sharp Dust Sensor Pin Values and
 * timing constants.
 * 
 */

const int dustPin = A2;
const int samplingTime = 280;
const int deltaTime = 40;
const int sleepTime = 9680;

float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

/*
 * Soil Conductivity Probes
 * Analog pin locations
 * 
 */
 const int soilPin1 = A0;
 const int soilPin2 = A1;

 /*
  * DHT22 
  */
  #define DHTPIN 2
  #define DHTTYPE DHT22
  DHT dht(DHTPIN, DHTTYPE);

/*
 * The Service Data to Advertise.
 * See initEddystoneUrlFrame().
 * 
 * urlFrame[] = the Service Data for our Eddystone-URL frame.
 * urlFrameLength = the number of bytes in urlFrame[].
 */
uint8_t urlFrame[MAX_URL_FRAME_LENGTH];
uint8_t urlFrameLength;

/*
 * If false after setup(), setup failed.
 */
bool setupSucceeded;


void setup() {  

  pinMode(PIN_BLINK, OUTPUT);
  
  setupSucceeded = false;
  digitalWrite(PIN_BLINK, HIGH);

  if (!initEddystoneUrlFrame(TX_POWER_DBM, MY_URL)) {
    return; // don't start advertising if the URL won't work.
  }
    
  /*
   * Set up Bluetooth Low Energy Advertising
   * of the Eddystone-URL frame.
   */
  //ble.setLocalName("EnviroSense");
  //add dust service and char
  
  ble.setAdvertisedServiceUuid(dustService.uuid());
  ble.addAttribute(dustService);
  ble.addAttribute(dustCharacteristic);

  //add soil sensor service and char
  ble.setAdvertisedServiceUuid(soilService.uuid());
  ble.addAttribute(soilService);
  ble.addAttribute(soilCharacteristic1);
  ble.addAttribute(soilCharacteristic2);

  //add temp/hum service
  ble.setAdvertisedServiceUuid(tempHumService.uuid());
  ble.addAttribute(tempHumService);
  ble.addAttribute(tempCharacteristic);
  ble.addAttribute(humCharacteristic);  
  
  //initialize soil/dust/temp/hum chars to 0
  dustCharacteristic.setValue(0);
  soilCharacteristic1.setValue(0);
  soilCharacteristic2.setValue(0);
  tempCharacteristic.setValue(0);
  humCharacteristic.setValue(0);

  
  //set eddystone services...
  ble.setAdvertisedServiceUuid(eddyService.uuid());
  ble.setAdvertisedServiceData(eddyService.uuid(), urlFrame, urlFrameLength);
  
  // Start Advertising our Eddystone URL.
  ble.begin();
  //start dht sensor
  dht.begin();
  setupSucceeded = true;
  digitalWrite(PIN_BLINK, LOW);
}


unsigned long prevReportMillis = 0L;

void loop() {

  // If setup() failed, do nothing
  if (!setupSucceeded) {
    delay(1);
    return;
  }
  ble.poll();
  readTempHum();
  readDust();
  readSoil();

  unsigned long now = millis();

  if ((now % 1000) < 100) {
    digitalWrite(PIN_BLINK, HIGH);
  } else {
    digitalWrite(PIN_BLINK, LOW);
  }

  if ((long) (now - (prevReportMillis + 10000L))>= 0L) {
    prevReportMillis = now;
  }
}

/*
 * Read temp/hum values 
 *
 */
void readTempHum()
{
  delay(3000);
  float h = dht.readHumidity();
  float t = dht.readTemperature(true);

  if (isnan(h) || isnan(t))
  {
    return;
  }
  tempCharacteristic.setValue(t);
  humCharacteristic.setValue(h);
}

/*
 * Read soil conductivity values
 */
 void readSoil()
 {
  int soilValue1 = analogRead(soilPin1);
  int soilValue2 = analogRead(soilPin2);
  boolean soilChanged = (soilCharacteristic1.value() != soilValue1) || (soilCharacteristic2.value() != soilValue2);

  if (soilChanged) {
    soilCharacteristic1.setValue(soilValue1);
    soilCharacteristic2.setValue(soilValue2);
  }
 }


/*
 * Read Dust Sensor Status and Calc. Dust Level
 * 
 * 
 */
void readDust()
{
   delayMicroseconds(samplingTime);

  voMeasured = analogRead(dustPin); // read the dust value
  
  delayMicroseconds(deltaTime);
  delayMicroseconds(sleepTime);

  // 0 - 3.3V mapped to 0 - 1023 integer values 
  calcVoltage = voMeasured * (3.3 / 1024); 
  
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = (0.17 * calcVoltage - 0.1)*1000;


  boolean dustChanged = (dustCharacteristic.value() != dustDensity);

  if (dustChanged) {
    dustCharacteristic.setValue(dustDensity);
  }
  delay(1000);
}


/*
 * Prints the complete advertising block
 * that was assembled by CurieBLE.
 * For debugging.
 */
void report() {
  uint8_t len = ble.getAdvertisingLength();
  uint8_t* pAdv = ble.getAdvertising();

  Serial.print("Advertising block length: ");
  Serial.println(len);
  for (int i = 0; i < len; ++i) {
    Serial.print("0x");
    Serial.println(pAdv[i], HEX);
  }
}


/*
 * Fills urlFrame and urlFrameLength
 * based on the given Transmit power and URL.
 * 
 * txPower = the transmit power of the Arduino 101 to report.
 * See TX_POWER_DBM above.
 * 
 * url = the URL to encode.  Likely a shortened URL
 * produced by an URL shortening service such as https://goo.gl/
 * 
 * Returns true if successful; false otherwise.
 * If the encoded URL is too long, use an URL shortening service
 * before passing the (shortened) URL to setServiceData().
 * 
 * NOTE: most of the code of this function is untested.
 */
boolean initEddystoneUrlFrame(int8_t txPower, const char* url) {
  urlFrameLength = 0;

  // The frame starts with a type byte, then power byte.
  urlFrame[urlFrameLength++] = FRAME_TYPE_EDDYSTONE_URL;
  urlFrame[urlFrameLength++] = (uint8_t) txPower;

  if (url == 0 || url[0] == '\0') {
    return false;   // empty URL
  }

  const char *pNext = url;

  /*
   * the next byte of the frame is an URL prefix code.
   * See URL_PREFIX_* above.
   */
  
  if (strncmp("http", pNext, 4) != 0) {
    return false;  // doesn't start with HTTP or HTTPS.
  }
  pNext += 4;
  
  bool isHttps = false; // that is, HTTP
  if (*pNext == 's') {
    pNext++;
    isHttps = true;
  }

  if (strncmp("://", pNext, 3) != 0) {
    return false; // malformed URL
  }
  pNext += 3;

  urlFrame[urlFrameLength] = URL_PREFIX_HTTP_COLON_SLASH_SLASH;
  if (isHttps) {
    urlFrame[urlFrameLength] = URL_PREFIX_HTTPS_COLON_SLASH_SLASH;
  }

  if (strncmp("www.", pNext, 4) == 0) {
    pNext += 4;

    urlFrame[urlFrameLength] = URL_PREFIX_HTTP_WWW_DOT;
    if (isHttps) {
      urlFrame[urlFrameLength] = URL_PREFIX_HTTPS_WWW_DOT;
    }
  }
  
  urlFrameLength++;

  // Encode the URL.

  while (urlFrameLength < MAX_URL_FRAME_LENGTH && *pNext != '\0') {
    if (strncmp(".com/", pNext, 5) == 0) {
      pNext += 5;
      urlFrame[urlFrameLength++] = 0x00;
    } else if (strncmp(".org/", pNext, 5) == 0) {
      pNext += 5;
      urlFrame[urlFrameLength++] = 0x01;
    } else if (strncmp(".edu/", pNext, 5) == 0) {
      pNext += 5;
      urlFrame[urlFrameLength++] = 0x02;
    } else if (strncmp(".net/", pNext, 5) == 0) {
      pNext += 5;
      urlFrame[urlFrameLength++] = 0x03;
    } else if (strncmp(".info/", pNext, 6) == 0) {
      pNext += 6;
      urlFrame[urlFrameLength++] = 0x04;
    } else if (strncmp(".biz/", pNext, 5) == 0) {
      pNext += 5;
      urlFrame[urlFrameLength++] = 0x05;
    } else if (strncmp(".gov/", pNext, 5) == 0) {
      pNext += 5;
      urlFrame[urlFrameLength++] = 0x06;
    } else if (strncmp(".com", pNext, 4) == 0) {
      pNext += 4;
      urlFrame[urlFrameLength++] = 0x07;
    } else if (strncmp(".org", pNext, 4) == 0) {
      pNext += 4;
      urlFrame[urlFrameLength++] = 0x08;
    } else if (strncmp(".edu", pNext, 4) == 0) {
      pNext += 4;
      urlFrame[urlFrameLength++] = 0x09;
    } else if (strncmp(".net", pNext, 4) == 0) {
      pNext += 4;
      urlFrame[urlFrameLength++] = 0x0A;
    } else if (strncmp(".info", pNext, 5) == 0) {
      pNext += 5;
      urlFrame[urlFrameLength++] = 0x0B;
    } else if (strncmp(".biz", pNext, 4) == 0) {
      pNext += 4;
      urlFrame[urlFrameLength++] = 0x0C;
    } else if (strncmp(".gov", pNext, 4) == 0) {
      pNext += 4;
      urlFrame[urlFrameLength++] = 0x0D;
    } else {
      // It's not special.  Just copy the character
      urlFrame[urlFrameLength++] = (uint8_t) *pNext++;
    }
  }

  return true;
}

