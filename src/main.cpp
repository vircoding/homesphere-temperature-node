#include <WiFi.h>
#include <freertos/FreeRTOS.h>

#include "ButtonManager.hpp"
#include "DHTManager.hpp"
#include "NowManager.hpp"

const uint8_t dhtData = 4;
const uint8_t ledPin = 2;
const uint8_t buttonPin = 23;

// Dirección MAC del nodo master
// uint8_t masterMac[] = {0xD4, 0x8A, 0xFC, 0xC5, 0xC1, 0x80};
uint8_t masterMac[] = {0xD4, 0x8A, 0xFC, 0xAA, 0x1E, 0x44};

DHTManager::Data data;

DHTManager dht(dhtData, data);
NowManager now(masterMac);
ButtonManager button(buttonPin);

void onSendCallback(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("Estado de envío a ");
  Serial.println(NowManager::getMacStr(mac_addr).c_str());
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? " -> Entregado"
                                                : " -> Fallo");
}

void onSimplePressCallback() { Serial.println("Pulsacion corta"); }

void onLongPressCallback() { Serial.println("Pulsacion larga"); }

void readTask(void* parameter) {
  while (1) {
    dht.read();
    vTaskDelay(pdMS_TO_TICKS(10));

    // Print data
    Serial.printf("Temperatura: %fC\n", data.temp);
    Serial.printf("Humedad: %f%\n", data.hum);

    // Send data to master
    now.sendTemperatureData(data.temp, data.hum);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);

  button.begin();
  button.on(ButtonManager::Event::SIMPLE_PRESS, onSimplePressCallback);
  button.on(ButtonManager::Event::LONG_PRESS, onLongPressCallback);

  // now.init();
  // now.onSend(onSendCallback);
  // now.registerMasterPeer();

  // xTaskCreatePinnedToCore(readTask, "Read", 4096, NULL, 1, NULL, 0);
}

void loop() { button.update(); }