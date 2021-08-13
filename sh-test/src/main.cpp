#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>

#include "sensesp_app.h"
#include "sensesp_app_builder.h"
#include "sensors/onewire_temperature.h"
#include "signalk/signalk_output.h"

// 1-Wire data pin on SH-ESP32
#define ONEWIRE_PIN 4

// SDA and SCL pins on SH-ESP32
#define SDA_PIN 16
#define SCL_PIN 17

// CAN bus (NMEA 2000) pins on SH-ESP32
#define CAN_RX_PIN GPIO_NUM_34
#define CAN_TX_PIN GPIO_NUM_32

// OLED display width and height, in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// define temperature display units
#define TEMP_DISPLAY_FUNC KelvinToCelsius
//#define TEMP_DISPLAY_FUNC KelvinToFahrenheit

TwoWire* i2c;
Adafruit_SSD1306* display;

/// Clear a text row on an Adafruit graphics display
void ClearRow(int row) { display->fillRect(0, 8 * row, SCREEN_WIDTH, 8, 0); }

float KelvinToCelsius(float temp) { return temp - 273.15; }

float KelvinToFahrenheit(float temp) { return (temp - 273.15) * 9. / 5. + 32.; }

void PrintTemperature(int row, String title, float temperature) {
  ClearRow(row);
  display->setCursor(0, 8 * row);
  display->printf("%s: %.1f", title.c_str(), TEMP_DISPLAY_FUNC(temperature));
  display->display();
}

ReactESP app([]() {
// Some initialization boilerplate when in debug mode...
#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif

  SensESPAppBuilder builder;

  sensesp_app = builder.set_hostname("temperatures")
                    ->set_standard_sensors(NONE)
                    ->get_app();

  DallasTemperatureSensors* dts = new DallasTemperatureSensors(ONEWIRE_PIN);

  // define three 1-Wire temperature sensors that update every 1000 ms
  // and have specific web UI configuration paths

  auto floor_heating_kitchen_temperature =
      new OneWireTemperature(dts, 1000, "/floorHeatingKitchen/oneWire");
  auto floor_heating_bathroom_in_temperature =
      new OneWireTemperature(dts, 1000, "/floorHeatingBathroomIn/oneWire");
  auto floor_heating_bathroom_out_temperature =
      new OneWireTemperature(dts, 1000, "/floorHeatingBathroomOut/oneWire");
  auto floor_heating_entrance_temperature =
      new OneWireTemperature(dts, 1000, "/floorHeatingEntrance/oneWire");

  // define metadata for sensors

  auto floor_heating_kitchen_temperature_metadata =
      new SKMetadata("K",                       // units
                     "Kitchen Temperature",     // display name
                     "Kitchen Temperature",     // description
                     "Kitchen Temp",              // short name
                     10.                        // timeout, in seconds
      );
  auto floor_heating_bathroom_in_temperature_metadata =
      new SKMetadata("K",                           // units
                     "Bathroom Temperature In",  // display name
                     "Bathroom Temperature In",  // description
                     "Bath Temp In",         // short name
                     10.                            // timeout, in seconds
      );
  auto floor_heating_bathroom_out_temperature_metadata =
      new SKMetadata("K",                           // units
                     "Bathroom Temperature Out",  // display name
                     "Bathroom Temperature Out",  // description
                     "Bath Temp Out",         // short name
                     10.                            // timeout, in seconds
      );
  auto floor_heating_entrance_temperature_metadata =
      new SKMetadata("K",                   // units
                     "Entrance Temperature",  // display name
                     "Entrance Temperature",  // description
                     "Entrance Temperature",         // short name
                     10.                    // timeout, in seconds
      );

  // connect the sensors to Signal K output paths

  floor_heating_kitchen_temperature->connect_to(new SKOutput<float>(
      "environment.kitchen.floorTemperature",
      "/floorHeatingKitchenTemp/skPath",
      floor_heating_kitchen_temperature_metadata));
  floor_heating_bathroom_in_temperature->connectTo(new SKOutput<float>(
      "environment.bathroom.floorTemperatureIn",
      "/floorHeatingBathroomTemp/skPath",
      floor_heating_bathroom_in_temperature_metadata));
  floor_heating_bathroom_out_temperature->connectTo(new SKOutput<float>(
      "environment.bathroom.floorTemperatureOut",
      "/floorHeatingBathroomTemp/skPath",
      floor_heating_bathroom_out_temperature_metadata));
  floor_heating_entrance_temperature->connectTo(new SKOutput<float>(
      "environment.entrance.floorTemperature",
      "/floorHeatingEntranceTemp/skPath",
      floor_heating_entrance_temperature_metadata));

  // initialize the display
  i2c = new TwoWire(0);
  i2c->begin(SDA_PIN, SCL_PIN);
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, i2c, -1);
  if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  delay(100);
  display->setRotation(2);
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);
  display->printf("Host: %s", sensesp_app->get_hostname().c_str());
  display->display();

  // Add display updaters for temperature values
  floor_heating_kitchen_temperature->connect_to(new LambdaConsumer<float>(
      [](float temperature) { PrintTemperature(1, "Kitchen", temperature); }));
  floor_heating_bathroom_in_temperature->connect_to(new LambdaConsumer<float>(
      [](float temperature) { PrintTemperature(2, "Bathroom In", temperature); }));
  floor_heating_bathroom_out_temperature->connect_to(new LambdaConsumer<float>(
      [](float temperature) { PrintTemperature(3, "Bathroom Out", temperature); }));
  floor_heating_entrance_temperature->connect_to(new LambdaConsumer<float>(
      [](float temperature) { PrintTemperature(4, "Entrance", temperature); }));

  sensesp_app->enable();
});
