// DHTXX_example.cpp

#include <Arduino.h>

//#define SERIAL_DEBUG_DISABLED

#define USE_LIB_WEBSOCKET true

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

#include "sensesp_app.h"
#include "signalk/signalk_output.h"
#include "dhtxx.h"

ReactESP app([] () {
  #ifndef SERIAL_DEBUG_DISABLED
  Serial.begin(115200);

  // A small arbitrary delay is required to let the
  // serial port catch up

  delay(100);
  Debug.setSerialEnabled(true);
  #endif

  // Create the global SensESPApp() object.
  sensesp_app = new SensESPApp();

  // Tell SensESP where the sensor is connected to the board
  // ESP8266 pins are specified as DX
  // ESP32 pins are specified as just the X in GPIOX
#ifdef ESP8266
  uint8_t pin = D6;
#elif defined(ESP32)
  uint8_t pin = 4;
#endif

  // Create a DHTxx object, which represents the physical sensor.
  auto* pDHTxx = new DHTxx(pin, DHTTYPE);

  // Define the read_delays you're going to use:
  const uint read_delay = 2000; // once every other second

  // Create a DHTvalue, which is used to read a specific value from the DHTxx sensor,
  // and send its output to SignalK as a number (float). This one is for the temperature reading.
  auto* pDHTtemperature = new DHTValue(pDHTxx, DHTValue::temperature, read_delay, "Outside/Temperature");
      
      pDHTtemperature->connectTo(new SKOutputNumber("environment.outside.temperature"));


  // Do the same for the humidity value.
  auto* pDHThumidity = new DHTValue(pDHTxx, DHTValue::humidity,  read_delay, "Outside/Humidity");
      
      pDHThumidity->connectTo(new SKOutputNumber("environment.outside.humidity"));      


  sensesp_app->enable();
});
