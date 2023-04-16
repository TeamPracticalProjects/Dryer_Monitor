/*
 * Project Temperature_Test
 * Description:  Testing out code for reading temperature from DS18B20 in non-blocking mode
 * Author: Bob Glicksman
 * Version: 1.0
 * Date: 4/16/23
 * (c) 2023. Bob Glicksman, Jim Schrempp, Team Practical Projects.  All rights reserved.
 */

// Libraries
#include <OneWire.h>
#include <spark-dallas-temperature.h>

// Basic definitions for use in this program
const int ONE_WIRE_BUS_PIN = D0;
const int TEMPERATURE_RESOLUTION = 12;
const int NUMBER_OF_SENSORS = 1;
const int READOUT_INTERVAL = 2000;  // write current temperature to serial port every 2 seconds

// Global variables for temperature measurement
float currentTemperature = 0.0f;  // global variable to hold the latest temperature reading
String displayTemperature = "";   // global variable for string representation of the temperature

// DS18B20 definitions
OneWire oneWire(ONE_WIRE_BUS_PIN);   // create an instance of the one wire bus
DallasTemperature sensors(&oneWire);  // create instance of DallasTemperature devices on the one wire bus
DeviceAddress processTemperatureProbeAddress;  // array to hold the device ID code for the process monitoring temperature probe

/********************************* setup() ***********************************/
void setup() {
  
  // initialize the internal LED
  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW);

  // define globabl variable to hold the temperature
  Particle.variable("Temperature", displayTemperature);

  // start the serial port and the oneWire sensors
  Serial.begin(9600);
  sensors.begin();

  delay(2000);  // wait to get serial monitor going
  digitalWrite(D7, HIGH); // indicate that serial data is coming

  // determine the sensors on the onewire bus
  int numSensors = sensors.getDeviceCount();  // determine number of sensors on the bus
  if(numSensors == NUMBER_OF_SENSORS) {
    Serial.println("Found all sensors; OK!\n");
  }
  else {
    Serial.print("Found ");
    Serial.print(numSensors);
    Serial.println(" sensors. ERROR!");
  }

  // get the device address for the temperature sensor and set it up
  sensors.getAddress(processTemperatureProbeAddress, 0);  // first sensor found, index 0
  sensors.setResolution(TEMPERATURE_RESOLUTION);
  sensors.setWaitForConversion(false);  // set non-blocking operation

} // end of setup()


/********************************* loop() ***********************************/
void loop() {
  static bool timeForNewDisplay = false;
  static unsigned long lastDisplayTime = millis();
  unsigned long currentTime;

  // non-blocking call to read temperature and write it to global variable
  if(readTemperatureSensors() == true); {// new reading
    displayTemperature = String(currentTemperature);  // set the global variable for cloud reading
  }
  
  // is it time to print out the temperature?
  currentTime = millis();
  if(diff(currentTime, lastDisplayTime) >= READOUT_INTERVAL) {
    Serial.print("Temperature = ");
    Serial.println(currentTemperature);

    lastDisplayTime = currentTime;
  }

} // end of loop()

/********************************* readTemperatureSensors() ***********************************/
// Function to perform non-blocking reading of the DS18B20 Temerature sensors
//  Returns true if there is a new reading (750 ms from command)
//  also updates the global variables with the temperatures read

boolean readTemperatureSensors() {
  const unsigned long readingTime = 750L;  //DS18B20 spec wait time for 12 bit reading
  
  static boolean readingInProgress = false; 
  static unsigned long lastTime;
  static unsigned long newTime;
  
  unsigned long timeInterval;
  
  if (readingInProgress == false) { // idle
    sensors.requestTemperatures(); // get all sensors started reading
    readingInProgress = true;
    lastTime = millis();
    return false;
  }  
  else {  // waiting on conversion completion
    newTime = millis();
    timeInterval = diff(newTime, lastTime);

    if(timeInterval < readingTime) { // still reading
      return false;  // no new temps
    } 
    else {  // conversion complete -- get the values in degrees F
      currentTemperature = sensors.getTempF (processTemperatureProbeAddress);
      readingInProgress = false;
      return true; // new temps available 
    }

  }
  
}  // end of readTemperatureSensors()

/********************************* BEGINNING OF diff() *****************************************************/
// Function to subtract a new and old millis() reading, correcting for millis() overflow
//  arguments:
//    newTime - the current, updated time from millis()
//    oldTime - the previous time, from millis() for comparision for non-blocking delays
//  returns:  the time difference, correcting for millis() overflow which occurs one every 70 days
unsigned long diff (unsigned long newTime, unsigned long oldTime) {
  const unsigned long maxTimeValue = 0xFFFFFFFF;  // max value if millis() before overflow to 0x00000000
  long timeInterval;
 
  if (newTime < oldTime)  // overflow of millis() has occurred -- fix
  {
    timeInterval = newTime + (maxTimeValue - oldTime) + 1L;
  }  
  else {
    timeInterval = newTime - oldTime;
  }
  return timeInterval;

} // end of diff()
