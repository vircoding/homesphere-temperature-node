#include <LittleFS.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>

#include "ConfigManager.hpp"
#include "DHTManager.hpp"
#include "IndicatorManager.hpp"
#include "NowManager.hpp"
#include "SyncButtonManager.hpp"
#include "Utils.hpp"

// Pinout
const uint8_t dhtData = 4;
const uint8_t ledPin = 2;
const uint8_t syncButtonPin = 23;

// Task Handlers
TaskHandle_t blinkLEDTaskHandler = NULL;
DHTManager::Data data;

// Timer Handlers
TimerHandle_t syncModeTimeoutTimerHandler = NULL;

// Global variables
ConfigManager config;
IndicatorManager led(ledPin);
SyncButtonManager syncButton(syncButtonPin);
DHTManager dht(dhtData, data);
NowManager now;

// Definitions
void onSendCallback(const uint8_t* mac_addr, esp_now_send_status_t status);
void onSyncReceivedCallback(const uint8_t* mac, const uint8_t* data,
                            int length);
void onConfirmRegistrationReceivedCallback(const uint8_t* mac,
                                           const uint8_t* data, int length);
void readSensorTask(void* parameter);
void blinkLEDTask(void* parameter);
void enterSyncMode();
void endSyncMode();
void onLongButtonPressCallback() { enterSyncMode(); };
void onSimpleButtonPressCallback() { endSyncMode(); };
void syncModeTimeoutCallback(TimerHandle_t xTimer) { endSyncMode(); };

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);

  if (!config.init()) {
    ESP.restart();
  }

  uint8_t mac[6];
  config.copyMasterMac(mac);
  Serial.print("Master MAC en config: ");
  Serial.println(macToString(mac));

  WiFi.mode(WIFI_MODE_STA);

  led.begin();

  syncButton.begin();
  syncButton.on(SyncButtonManager::Event::SIMPLE_PRESS,
                onSimpleButtonPressCallback);
  syncButton.on(SyncButtonManager::Event::LONG_PRESS,
                onLongButtonPressCallback);

  now.init();
  // now.onSend(onSendCallback);

  // uint8_t masterMac[6];
  // config.copyMasterMac(masterMac);
  // now.registerMasterPeer(masterMac);

  // xTaskCreatePinnedToCore(readSensorTask, "Read Sensor", 4096, NULL, 1, NULL,
  //                         0);
}

void loop() {
  syncButton.update();

  if (Serial2.available()) {
    Serial.write(Serial2.read());
  }
}

void onSendCallback(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("Estado de envío a ");
  Serial.println(macToString(mac_addr).c_str());
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? " -> Entregado"
                                                : " -> Fallo");
}

void onSyncReceivedCallback(const uint8_t* mac, const uint8_t* data,
                            int length) {
  if (NowManager::validateMessage(NowManager::MessageType::SYNC_BROADCAST, data,
                                  length)) {
    const NowManager::SyncBroadcastMsg* msg =
        reinterpret_cast<const NowManager::SyncBroadcastMsg*>(data);

    if (verifyCRC8(*msg)) {
      // Registrar nuevo Master Peer y enviar mensaje de registro
      if (now.registerMasterPeer(mac) && now.sendRegistrationMsg()) {
        if (now.sendRegistrationMsg()) {
          Serial.println("Mensaje Registration enviado");
          now.unsuscribeOnReceived();
          now.onReceived(onConfirmRegistrationReceivedCallback);
        } else {
          Serial.println("Error enviando el mensaje Registration");
        }
      }
    } else {
      Serial.println("Error CRC8 inválido");
    }
  }
}

void onConfirmRegistrationReceivedCallback(const uint8_t* mac,
                                           const uint8_t* data, int length) {
  if (NowManager::validateMessage(NowManager::MessageType::CONFIRM_REGISTRATION,
                                  data, length)) {
    if (config.saveMasterMacConfig(mac)) {
      ESP.restart();
    }
  }
}

void readSensorTask(void* parameter) {
  while (1) {
    dht.read();
    vTaskDelay(pdMS_TO_TICKS(10));

    // Print data
    Serial.printf("Temperatura: %fC\n", data.temp);
    Serial.printf("Humedad: %f%\n", data.hum);

    // Send data to master
    // now.sendTemperatureData(data.temp, data.hum);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void blinkLEDTask(void* parameter) {
  while (1) {
    led.set(true);
    vTaskDelay(pdMS_TO_TICKS(500));  // 500ms

    led.set(false);
    vTaskDelay(pdMS_TO_TICKS(500));  // 500ms
  }
}

void enterSyncMode() {
  if (blinkLEDTaskHandler == NULL) {
    if (!now.reset()) ESP.restart();

    if (!now.registerBroadcastPeer()) return;

    now.onReceived(onSyncReceivedCallback);

    xTaskCreatePinnedToCore(blinkLEDTask, "Blink LED", 2048, NULL, 2,
                            &blinkLEDTaskHandler, 1);

    syncModeTimeoutTimerHandler = xTimerCreate(
        "Sync Mode Timeout", pdMS_TO_TICKS(NowManager::SYNC_MODE_TIMEOUT),
        pdFALSE, (void*)0,
        syncModeTimeoutCallback);  // 30s

    if (syncModeTimeoutTimerHandler != NULL)
      xTimerStart(syncModeTimeoutTimerHandler, 0);
  }
}

void endSyncMode() {
  // Detener el timer
  xTimerStop(syncModeTimeoutTimerHandler, 0);

  // Eliminar el timer y liberar memoria
  if (xTimerDelete(syncModeTimeoutTimerHandler, 0) == pdPASS)
    syncModeTimeoutTimerHandler =
        NULL;  // Reasignar a NULL para evitar usos indebidos

  if (blinkLEDTaskHandler != NULL) {
    vTaskDelete(blinkLEDTaskHandler);
    blinkLEDTaskHandler = NULL;
    led.set(false);
  }

  // Reiniciar ESP-NOW
  if (!now.reset()) ESP.restart();
}
