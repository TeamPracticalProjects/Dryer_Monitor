/*
 * Project Temperature_Test
 * Description:  Testing out code for reading temperature from DS18B20 in non-blocking mode
 * Author: Bob Glicksman
 * Version: 1.0
 * Date: 4/16/23
 * (c) 2023. Bob Glicksman, Jim Schrempp, Team Practical Projects.  All rights reserved.
 * 
 * version 1.1; 4/17/23.  Added in a global variable to record the maximum temperature measured.
 *   4/30/2023  now publishes an event every 30 seconds. Expected to webhook into a google sheet.
 *   5/4/2023   constrains temp to -50:400;  tracks min temp;  tracks 5 point moving average temp
 */

// Libraries
#include <OneWire.h>
#include <spark-dallas-temperature.h>

// Basic definitions for use in this program
const int ONE_WIRE_BUS_PIN = D0;
const int TEMPERATURE_RESOLUTION = 12;
const int NUMBER_OF_SENSORS = 1;
const int READOUT_INTERVAL = 2000;  // write current temperature to serial port every 2 seconds
const int PUBLISH_INTERVAL = 30000; // write to google sheet this often
const float GOOD_TEMP_READING_MAX = 400; // readings over this are ignored
const float GOOD_TEMP_READING_MIN = -50; // readings below this are ignored


// Global variables for temperature measurement
float g_currentTemperature = 0.0f;  // global variable to hold the latest temperature reading
String g_displayTemperature = "";   // global variable for string representation of the temperature
String g_maximumTemperature = "";   // global variable for string representation of the peak temperature
String g_minimumTemperature = "";   // global variable for string representation of the minimum temperature

// DS18B20 definitions
OneWire oneWire(ONE_WIRE_BUS_PIN);   // create an instance of the one wire bus
DallasTemperature sensors(&oneWire);  // create instance of DallasTemperature devices on the one wire bus
DeviceAddress g_processTemperatureProbeAddress;  // array to hold the device ID code for the process monitoring temperature probe

/********************************* setup() ***********************************/
void setup() {

    Time.zone(-8);
  
  // initialize the internal LED
  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW);

  // define globle variables to hold the temperature and maximum temperature
  Particle.variable("Temperature", g_displayTemperature);
  Particle.variable("Maximum Temperature", g_maximumTemperature);
  Particle.variable("Minimum Temperature", g_minimumTemperature);

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
  sensors.getAddress(g_processTemperatureProbeAddress, 0);  // first sensor found, index 0
  sensors.setResolution(TEMPERATURE_RESOLUTION);
  sensors.setWaitForConversion(false);  // set non-blocking operation

} // end of setup()


/********************************* loop() ***********************************/
void loop() {

  static bool firstAvgTempCalc = true;
  static unsigned long lastDisplayTime = millis();
  static unsigned long lastPublishTime = millis();
  static float avgTemp = -253;  // moving average of the temperature
  static float maxTemp = -253;  // record the maximum temperature encountered
  static float minTemp = 999;
  unsigned long currentTime;

  // non-blocking call to read temperature and write it to global variable
  if(readTemperatureSensors() == true) {

    if ((g_currentTemperature < GOOD_TEMP_READING_MIN) || (g_currentTemperature > GOOD_TEMP_READING_MAX)) {
        // spurious bad reading, ignore it

    } else {

        // new reading
        g_displayTemperature = String(g_currentTemperature);  // set the global variable for cloud reading

        // determine if the latest temperature reading is the maximum encountered
        if(g_currentTemperature > maxTemp) {
        maxTemp = g_currentTemperature;
        g_maximumTemperature = String(maxTemp);
        }

        // determine if the latest temp reading is the minimum encountered
        if(g_currentTemperature < minTemp) {
        minTemp = g_currentTemperature;
        g_minimumTemperature = String(minTemp);
        }

        // calculate the moving average
        if (firstAvgTempCalc) {
            // first time we've come through the loop since start up
            avgTemp = g_currentTemperature;
            firstAvgTempCalc =  false;
        } else {
            avgTemp =  avgTemp * 0.8 + g_currentTemperature * 0.2;  // 10 point moving average
        }

    }
  }
  
  currentTime = millis();

  // is it time to print out the temperature?
  unsigned long intervalReadout = diff(currentTime, lastDisplayTime);
  if (intervalReadout >= READOUT_INTERVAL) {
    Serial.printlnf("Temperature = %.2f  Average = %.2f   Max = %.2f   Min = %.2f", g_currentTemperature, avgTemp, maxTemp, minTemp);
    lastDisplayTime = currentTime;
  }

  // is it time to publish the temperature?
  unsigned long intervalPublish = diff(currentTime, lastPublishTime);
    if (intervalPublish >= PUBLISH_INTERVAL) {
        //publish to spreadsheet
        publishTempToSpreadsheet(g_currentTemperature);
        Serial.println("Sent temp to g sheet");
        lastPublishTime = currentTime;
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
      g_currentTemperature = sensors.getTempF (g_processTemperatureProbeAddress);
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

//  publish new temperature and humidity values
void publishTempToSpreadsheet(float temp) {
  String eData = "";

  // build the data string with time, temp and rh values
  eData += "{\"etime\":";
  eData += String(Time.now());
  eData += ",\"temp\":";
  eData += String(temp);
  eData += ",\"localtime\":\"";
  eData += String(Time.format("%F %T"));
  eData += "\"}";

  // publish to the webhook
  Particle.publish("dryerTemp", eData, PRIVATE);

  return;
} // end of publishTRH()

