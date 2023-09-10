#include <AMMSutils.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

String AMMS_TOKEN = "STATION_TOKEN_HERE";

WeatherSensor weatherSensor;
WiFiManager wifiManager;

AMMSutils amms;

void onDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifiManager.autoConnect("Bresser Gateway");
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Firmware version: 1.2");

  weatherSensor.begin();

  WiFi.mode(WIFI_STA);
  WiFi.onEvent(onDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

	// Connecting to Wi-Fi
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(30);
  Serial.printf("Starting execution...\n");
  wifiManager.autoConnect("Bresser Gateway");

  amms.begin(AMMS_TOKEN);
}

void loop() {
  // This example uses only a single slot in the sensor data array
  int const i = 0;

  // Clear all sensor data
  weatherSensor.clearSlots();

  // Tries to receive radio message (non-blocking) and to decode it.
  // Timeout occurs after a small multiple of expected time-on-air.
  int decode_status = weatherSensor.getMessage();

  if (decode_status == DECODE_OK) {
    Serial.printf("Id: [%8X] Typ: [%X] Battery: [%s] ",
                  weatherSensor.sensor[i].sensor_id,
                  weatherSensor.sensor[i].s_type,
                  weatherSensor.sensor[i].battery_ok ? "OK " : "Low");
    if (weatherSensor.sensor[i].temp_ok) {
      Serial.printf("Temp: [%5.1fC] ", weatherSensor.sensor[i].temp_c);
      amms.setSensorField("temperature", weatherSensor.sensor[i].temp_c);
    } else {
      Serial.printf("Temp: [---.-C] ");
    }

    if (weatherSensor.sensor[i].humidity_ok) {
      Serial.printf("Hum: [%3d%%] ", weatherSensor.sensor[i].humidity);

      amms.setSensorField("humidity", weatherSensor.sensor[i].humidity);
    } else {
      Serial.printf("Hum: [---%%] ");
    }

    if (weatherSensor.sensor[i].wind_ok) {
      Serial.printf("Wind max: [%4.1fm/s] Wind avg: [%4.1fm/s] Wind dir: [%5.1fdeg] ",
                    weatherSensor.sensor[i].wind_gust_meter_sec,
                    weatherSensor.sensor[i].wind_avg_meter_sec,
                    weatherSensor.sensor[i].wind_direction_deg);

      amms.setSensorField("wind_speed", weatherSensor.sensor[i].wind_avg_meter_sec);
      amms.setSensorField("wind_direction", weatherSensor.sensor[i].wind_direction_deg);
    } else {
      Serial.printf("Wind max: [--.-m/s] Wind avg: [--.-m/s] Wind dir: [---.-deg] ");
    }

    if (weatherSensor.sensor[i].rain_ok) {
      Serial.printf("Rain: [%7.1fmm] ", weatherSensor.sensor[i].rain_mm);
      amms.setSensorField("precipitation", weatherSensor.sensor[i].rain_mm);

    } else {
      Serial.printf("Rain: [-----.-mm] ");
    }

    if (weatherSensor.sensor[i].moisture_ok) {
      Serial.printf("Moisture: [%2d%%] ", weatherSensor.sensor[i].moisture);
    } else {
      Serial.printf("Moisture: [--%%] ");
    }

    Serial.printf("RSSI: [%5.1fdBm]\n", weatherSensor.sensor[i].rssi);
    Serial.println("Sending data");
    amms.sendData();
  }

  delay(100);
}