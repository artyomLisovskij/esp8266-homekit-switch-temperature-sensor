#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 12
#define PIN_SWITCH 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

void setup() {
  Serial.begin(115200);
  wifi_connect(); // in wifi_info.h
  
  // homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example
  
  my_homekit_setup();
}

void loop() {
  my_homekit_loop();
  delay(10);
}

//==============================
// Homekit setup and loop
//==============================

// access your homekit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_temperature;
extern "C" homekit_characteristic_t cha_switch;

static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

void cha_switch_on_setter(const homekit_value_t value) {
	bool on = value.bool_value;
	cha_switch.value.bool_value = on;	//sync the value
	LOG_D("Switch: %s", on ? "ON" : "OFF");
	digitalWrite(PIN_SWITCH, on ? LOW : HIGH);
}

void my_homekit_setup() {
  pinMode(PIN_SWITCH, OUTPUT);
	digitalWrite(PIN_SWITCH, HIGH);
  cha_switch.setter = cha_switch_on_setter;
  arduino_homekit_setup(&config);
}

void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > next_report_millis) {
    // report sensor values every 10 seconds
    next_report_millis = t + 10 * 1000;
    my_homekit_report();
  }
  if (t > next_heap_millis) {
    // show heap info every 5 seconds
    next_heap_millis = t + 5 * 1000;
    LOG_D("Free heap: %d, HomeKit clients: %d",
        ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
  }
}

void my_homekit_report() {
  // Get Temperature
  sensors.requestTemperatures();
  float temperature_value = sensors.getTempCByIndex(0); // SETUP read your real sensor here.

  if (temperature_value > 130 or temperature_value < -50) {
    LOG_D("Current temperature %.1f out of range (-50 to 130) set to 0", temperature_value);
    temperature_value = 0;
  }
  
  cha_temperature.value.float_value = temperature_value;
  LOG_D("Current temperature: %.1f", temperature_value);
  homekit_characteristic_notify(&cha_temperature, cha_temperature.value);

}
