////////////////////////////////////
//   DEVICE-SPECIFIC LED SERVICES //
////////////////////////////////////

#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

uint16_t sample_frequency = 5000;
uint32_t timer = 0;                                // keep track of time since last update

// Read Measurement
uint16_t co2;
float temperature;
float humidity;

uint16_t error;
char errorMessage[256];

SensirionI2CScd4x scd4x;


struct DEV_TempSensor : Service::TemperatureSensor {     // A standalone Temperature sensor

  SpanCharacteristic *temp;                         // reference to the Current Temperature Characteristic

  DEV_TempSensor() : Service::TemperatureSensor() {      // constructor() method
    Wire.begin();

    scd4x.begin(Wire);
    uint16_t error;
    char errorMessage[256];

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
      LOG0("Error trying to execute stopPeriodicMeasurement(): ");
      errorToString(error, errorMessage, 256);
      LOG0(errorMessage);
      LOG0("\n");
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
      LOG0("Error trying to execute startPeriodicMeasurement(): ");
      errorToString(error, errorMessage, 256);
      LOG0(errorMessage);
      LOG0("\n");
    }

    temp = new Characteristic::CurrentTemperature(20);      // instantiate the Current Temperature Characteristic
    temp->setRange(-50, 100);                                 // expand the range from the HAP default of 0-100 to -50 to 100 to allow for negative temperatures

  } // end constructor

  void loop() {

    if (temp->timeVal() > sample_frequency) {                           // check time elapsed since last update and proceed only if greater than 5 seconds

      uint16_t error;
      char errorMessage[256];


      error = scd4x.readMeasurement(co2, temperature, humidity);
      if (error) {
        LOG2("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        LOG2(errorMessage);
        delay(500);
      } else {
        temp->setVal(temperature);                            // set the new temperature; this generates an Event Notification and also resets the elapsed time

        LOG1("Temperature Update: ");
        LOG1(temperature);
        LOG1("\n");
      }
    }

  } // loop

};

//////////////////////////////////

struct DEV_CarbonDioxideSensor : Service::CarbonDioxideSensor  {     // A standalone Air Quality sensor

  SpanCharacteristic *co2level;
  SpanCharacteristic *co2detected;


  DEV_CarbonDioxideSensor() : Service::CarbonDioxideSensor() {      // constructor() method

    co2level = new Characteristic::CarbonDioxideLevel(400);
    co2detected = new Characteristic::CarbonDioxideDetected(0);


    Serial.print("Configuring CO2 Sensor");   // initialization message
    Serial.print("\n");

  } // end constructor

  void loop() {

    if (co2level->timeVal() > sample_frequency) {                           // check time elapsed since last update and proceed only if greater than 5 seconds
      if (!error && co2 < 16300 && co2 != 0) { // Skip erroneous mesaurements.
        if (co2 > 1000 && co2detected->getVal() == 0) {         //        If co2 levels are above 1000 for both this reading and the last, then set CO2 detected.
          if (co2level->getVal() > 1000) {
            co2detected->setVal(1);
            LOG0("High CO2 levels detected!\n");
          }
        }

        if (co2 < 1000 && co2detected->getVal() == 1) {
          co2detected->setVal(0);
          LOG0("CO2 back to normal.\n");
        }
        co2level->setVal(co2);                            // set the new temperature; this generates an Event Notification and also resets the elapsed time

        LOG1("CO2 Update: ");
        LOG1(co2);
        LOG1("\n");
      }
    }



  } // loop

};
//////////////
struct DEV_HumiditySensor : Service::HumiditySensor  {     // A standalone Air Quality sensor

  // An Air Quality Sensor is similar to a Temperature Sensor except that it supports a wide variety of measurements.
  // We will use three of them.  The first is required, the second two are optional.

  SpanCharacteristic *CurrentRelativeHumidity;                 // reference to the Air Quality Characteristic, which is an integer from 0 to 5


  DEV_HumiditySensor() : Service::HumiditySensor() {      // constructor() method

    CurrentRelativeHumidity = new Characteristic::CurrentRelativeHumidity(40);                       // instantiate the Air Quality Characteristic and set initial value to 1


  } // end constructor

  void loop() {

    if (CurrentRelativeHumidity->timeVal() > sample_frequency) {                           // check time elapsed since last update and proceed only if greater than 5 seconds      timer = millis();

      if (humidity != 0.00 && !error) {
        CurrentRelativeHumidity->setVal(humidity);                            // set the new temperature; this generates an Event Notification and also resets the elapsed time

        LOG1("Humidity Update: ");
        LOG1(humidity);
        LOG1("%\n");
      }
    }


  } // loop

};
