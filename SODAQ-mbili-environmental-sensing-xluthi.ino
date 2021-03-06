/*
 * Copyright 2018 AllThingsTalk, Xavier Lüthi
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/****
 * This experiments shows how a NB-IoT device can be used to monitor the
 * quality of your surrounding environment.
 * Measure air quality, noise levels, pressure, humidity, temperature and
 * light intensity to improve your quality of living.
 * 
 * Required connections:
 *  - AirQuality sensor on Groove pin A0
 *  - Light sensor on Groove pin A2
 *  - Sound (Loudness) sensor on Groove pin A4
 *  - TPH sensor (temperature, pression, humidity) on I2C Groove pin (written SCL SCA on the Mbili board)
 *  
 *  An account on orange.allthinkgstalk.com is required to gather measurements.  Specific authentication
 *   parameters must be set in 'my_keys.h'.  A template for this is provided in 'my_keys_EXAMPLE.h'.
 */

// Select your preferred method of sending data
#define CBOR
//#define BINARY
 
#include "ATT_NBIOT.h"
#include "my_keys.h"

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "AirQuality2.h"

// Mbili support
#define DEBUG_STREAM Serial
#define MODEM_STREAM Serial1
#define MODEM_ON_OFF_PIN 23

#define baud 9600

#define SEND_EVERY 10000  // Send every 10 seconds

ATT_NBIOT device;

#ifdef CBOR
  #include <CborBuilder.h>
  CborBuilder payload(device);
#endif

#ifdef BINARY
  #include <PayloadBuilder.h>
  PayloadBuilder payload(device);
#endif

#define AirQualityPin A0
#define LightSensorPin A2
#define SoundSensorPin A4

AirQuality2 airqualitysensor;
Adafruit_BME280 tph; // I2C

float soundValue;
float lightValue;
float temp;
float hum;
float pres;
int16_t airValue;

void setup() 
{
  pinMode(GROVEPWR, OUTPUT);  // Turn on the power for the secondary row of grove connectors
  digitalWrite(GROVEPWR, HIGH);

  delay(3000);

  DEBUG_STREAM.begin(baud);
  MODEM_STREAM.begin(baud);
  
  DEBUG_STREAM.println("Initializing and connecting... ");

  device.setAttDevice(MY_DEVICE_ID, MY_DEVICE_TOKEN, MY_APN);
  device.init(MODEM_STREAM, DEBUG_STREAM, MODEM_ON_OFF_PIN);
  
  if(device.connect())
    DEBUG_STREAM.println("Connected!");
  else
  {
    DEBUG_STREAM.println("Connection failed!");
    while(true) {
      // No connection. No need to continue the program
      DEBUG_STREAM.println("DEAD END! Reboot the device to try to connect again!");
      delay(5000);
      }  
  }
  
  DEBUG_STREAM.println();
  DEBUG_STREAM.println("-- Environmental Sensing NB-IoT experiment --");
  DEBUG_STREAM.println();

  initSensors();
}

unsigned long ctime = 0;
void loop() 
{
  unsigned long curTime = millis() + SEND_EVERY;  // Add interval to make sure data is send on startup
  if (curTime > (ctime + SEND_EVERY))
  {
    readSensors();
    displaySensorValues();
    sendSensorValues();
    
    ctime = curTime;
  }  
}

void initSensors()
{
  DEBUG_STREAM.println("Initializing sensors, this can take a few seconds...");
  
  pinMode(SoundSensorPin, INPUT);
  pinMode(LightSensorPin, INPUT);
  
  tph.begin();
  airqualitysensor.init(AirQualityPin);
  DEBUG_STREAM.println("Done");
}

void readSensors()
{
  DEBUG_STREAM.println("Start reading sensors");
  DEBUG_STREAM.println("---------------------");
    
  soundValue = analogRead(SoundSensorPin);
  lightValue = analogRead(LightSensorPin);
  lightValue = lightValue * 3.3 / 1023;  // Convert to lux based on the voltage that the sensor receives
  lightValue = pow(10, lightValue);
    
  temp = tph.readTemperature();
  hum = tph.readHumidity();
  pres = tph.readPressure()/100.0;
    
  airValue = airqualitysensor.getRawData();
}

void sendSensorValues()
{
  payload.reset();
  
  #ifdef CBOR  // Send data using Cbor
  payload.map(6);
  payload.addNumber(soundValue, "loudness");
  payload.addNumber(lightValue, "light");
  payload.addNumber(temp, "temperature");
  payload.addNumber(hum, "humidity");
  payload.addNumber(pres, "pressure");
  payload.addInteger(airValue, "air_quality");
  #endif

  #ifdef BINARY  // Send data using a Binary payload and our ABCL language
  payload.addNumber(soundValue);
  payload.addNumber(lightValue);
  payload.addNumber(temp);
  payload.addNumber(hum);
  payload.addNumber(pres);
  payload.addInteger(airValue);
  #endif
  
  payload.send();
  
}

void displaySensorValues()
{
  DEBUG_STREAM.print("Sound level: ");
  DEBUG_STREAM.print(soundValue);
  DEBUG_STREAM.println(" Analog (0-1023)");
      
  DEBUG_STREAM.print("Light intensity: ");
  DEBUG_STREAM.print(lightValue);
  DEBUG_STREAM.println(" Lux");
      
  DEBUG_STREAM.print("Temperature: ");
  DEBUG_STREAM.print(temp);
  DEBUG_STREAM.println(" °C");
      
  DEBUG_STREAM.print("Humidity: ");
  DEBUG_STREAM.print(hum);
	DEBUG_STREAM.println(" %");
      
  DEBUG_STREAM.print("Pressure: ");
  DEBUG_STREAM.print(pres);
	DEBUG_STREAM.println(" hPa");
  
  DEBUG_STREAM.print("Air quality: ");
  DEBUG_STREAM.print(airValue);
	DEBUG_STREAM.println(" Analog (0-1023)");
}
