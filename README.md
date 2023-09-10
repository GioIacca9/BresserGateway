# Gateway IoT per stazioni meteo Bresser

- [Gateway IoT per stazioni meteo Bresser](#gateway-iot-per-stazioni-meteo-bresser)
  - [Introduzione](#introduzione)
  - [Il problema](#il-problema)
  - [Gateway](#gateway)
  - [Programmazione del Gateway](#programmazione-del-gateway)
  - [Configurazione](#configurazione)
          - [Link Utili](#link-utili)

## Introduzione

Tra le diverse tipologie di stazioni meteo che stiamo sviluppando per la rete di monitoraggio ambientale, sono presenti le stazioni [Bresser 5 in 1](https://www.bresser.de/it/Ora-Tempo/Centro-meteo/BRESSER-Stazione-meteo-5-in-1-nero.html). Questo modello di stazione ha un ottimo rapporto qualità/prezzo ed è equipaggiato con diversi sensori di buona precisione. Queste caratteristiche rendono la stazione Bresser 5 in 1 un'ottima scelta per chi sta decidendo di entrare nel mondo della meteorologia.

## Il problema

La stazione *Bresser 5 in 1* è composta da due unità:

- Stazione meteo con sensori di temperatura, umidità, pressione, velocità e direzione del vento e precipitazioni
- Display di controllo remoto

Queste due unità comunicano tra di loro via radio sulla frequenza libera **868 MHz**. Non permettendo quindi di inviare i dati tramite rete Internet al server centrale del progetto AMMS per la raccolta dei dati meteorologici, abbiamo dovuto sviluppare un **Gateway** che fosse in grado di ricevere i dati inviati dalla stazione meteo tramite radio e trasmetterli via Internet al server centrale per l'analisi e l'immagazzinamento.


## Gateway

Lo sviluppo del Gateway inizia dalla scelta di un modulo radio in grado di comunicare a **868 MHz**. Prendendo spunto dal progetto Open Source [BresserWeatherSensorReceiver](https://github.com/matthias-bs/BresserWeatherSensorReceiver) di _Matthias Prinke_, abbiamo deciso di utilizzare il modulo [RFM95W-915S2](https://mou.sr/3ErU9NC) controllato da un **ESP32 D1 Mini**, ottimo per le sue dimensioni ridotte.

![RFM95W-915S2](https://delucalabs.com/assets/img/media/posts/bresser-gateway/RFM95W-915S2.jpg)

![ESP32 D1 Mini](https://delucalabs.com/assets/img/media/posts/bresser-gateway/esp32-d1-mini.jpg)

Si procede poi con i collegamenti tra ESP32 e il modulo RFM95 come segue:

|PIN RFM95W|PIN ESP32 D1 Mini|
|:-|:-|
|`RESET`|`IO32`|
|`NSS`|`IO27`|
|`SCK`|`IO18`|
|`MOSI`|`IO23`|
|`MISO`|`IO19`|
|`GND`|`GND`|
|`3.3V`|`3.3V`|
|`DIO0`|`IO21`|
|`DIO1`|`IO33`|

![Collegamenti RFM95-ESP32](https://delucalabs.com/assets/img/media/posts/bresser-gateway/Collegamenti-RFM95W.jpg)

Nel PIN `ANT` del modulo RFM95 va collegata l'antenna, facilmente realizzabile saldando un filo di rame solido lungo 82mm.

![Antenna](https://delucalabs.com/assets/img/media/posts/bresser-gateway/Antenna.jpg)

Ora l'ESP32 è pronto per ricevere i dati.

## Programmazione del Gateway

Il programma principale del Gateway è un fork dello [sketch d'esempio](https://github.com/matthias-bs/BresserWeatherSensorReceiver/blob/main/examples/BresserWeatherSensorBasic/BresserWeatherSensorBasic.ino) di [_Matthias Prinke_](https://github.com/matthias-bs/) con l'aggiunta della nostra libreria per l'invio dei dati meteorologici al server AMMS.

```cpp
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
```

L'unico parametro di configurazione da impostare nel programma è il **`TOKEN`** della stazione che permette al Gateway di autenticarsi con il server AMMS:

```cpp
String AMMS_TOKEN = "STATION_TOKEN_HERE";
```

All'avvio del ESP32, il programma inizializza il modulo radio

```cpp
weatherSensor.begin();
```

e cerca di connettersi alla rete Wi-Fi. Qualora non ci fosse nessuna rete salvata, renderà disponibile la rete Wi-Fi _Bresser Gateway_ che permetterà all'utente di configurare i parametri per la connessione ad Internet.

```cpp
wifiManager.autoConnect("Bresser Gateway");
```

Una volta connesso ad Internet, il Gateway si mette in ascolto per nuovi messaggi in ingresso dal modulo radio.
```cpp
int decode_status = weatherSensor.getMessage();
```

Ricevuto un nuovo messaggio, procede stampando il contenuto nella seriale

```cpp
Serial.printf("Temp: [%5.1fC] ", weatherSensor.sensor[i].temp_c);
```

e aggiungendo i dati dei sensori alla coda d'invio del server AMMS.

```cpp
amms.setSensorField("temperature", weatherSensor.sensor[i].temp_c);
```

Finito il ciclo di raccolta e stampa, procede con l'invio dei dati al server.

```cpp
amms.sendData();
```


## Configurazione

Il Bresser Gateway può essere facilmente configurato tramite l'interfaccia Web che rende disponibile al primo avvio. Per raggiungere la pagina di configurazione è sufficiente connettersi alla rete Wi-Fi _Bresser Gateway_.

Una volta connessi si verrà reindirizzati automaticamente alla pagina di configurazione, dove sarà possibile inserire le credenziali della rete Wi-Fi alla quale si intende collegare il Gateway.

###### Link Utili

[BresserGateway su Github](https://github.com/GioIacca9/BresserGateway)
 
*[AMMS]: Autonomous Meteorologic and Monitoring System