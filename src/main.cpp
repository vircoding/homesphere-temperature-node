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
const gpio_num_t dhtData = GPIO_NUM_4;
const gpio_num_t ledPin = GPIO_NUM_2;
const gpio_num_t syncButtonPin = GPIO_NUM_23;

// Task Handlers
TaskHandle_t blinkLEDTaskHandler = NULL;
TaskHandle_t sendDataTaskHandler = NULL;

// Timer Handlers
TimerHandle_t syncModeTimeoutTimerHandler = NULL;

// Global variables
bool syncModeState = false;
DHTManager::Data sensorData;

ConfigManager config;
IndicatorManager led(ledPin);
SyncButtonManager syncButton(syncButtonPin);
DHTManager dht(dhtData, sensorData);
NowManager now;

// Definitions
void onReceivedCallback(const uint8_t* mac, const uint8_t* data, int length);
void onSendCallback(const uint8_t* mac, esp_now_send_status_t status);
void onSyncReceivedCallback(const uint8_t* mac, const uint8_t* data,
                            int length);
void onConfirmRegistrationReceivedCallback(const uint8_t* mac,
                                           const uint8_t* data, int length);
void readSensorTask(void* parameter);
void sendDataTask(void* parameter);
void blinkLEDTask(void* parameter);
void enterSyncMode();
void endSyncMode();
void onLongButtonPressCallback() { enterSyncMode(); }
void onSimpleButtonPressCallback() { endSyncMode(); }
void registerMasterNode();
void syncModeTimeoutCallback(TimerHandle_t xTimer) { endSyncMode(); };
void enableDataTransfer();
void disableDataTransfer();

void setup() {
  Serial.begin(115200);

  if (!config.init()) {
    ESP.restart();
  }

  WiFi.mode(WIFI_MODE_STA);

  led.begin();

  syncButton.begin();
  syncButton.on(SyncButtonManager::Event::SIMPLE_PRESS,
                onSimpleButtonPressCallback);
  syncButton.on(SyncButtonManager::Event::LONG_PRESS,
                onLongButtonPressCallback);

  xTaskCreatePinnedToCore(readSensorTask, "Read Sensor", 4096, NULL, 1, NULL,
                          0);

  now.init();
  now.onReceived(onReceivedCallback);
  now.onSend(onSendCallback);
  enableDataTransfer();
  registerMasterNode();
}

void loop() { syncButton.update(); }

void onReceivedCallback(const uint8_t* mac, const uint8_t* data, int length) {
  if (!now.isMasterMac(mac)) return;

  if (NowManager::validateMessage(NowManager::MessageType::PING, data,
                                  length)) {
    const NowManager::PingMsg* msg =
        reinterpret_cast<const NowManager::PingMsg*>(data);

    now.sendDataMsg(sensorData.temp, sensorData.hum);
  }
}

void onSendCallback(const uint8_t* mac, esp_now_send_status_t status) {
  if (now.isMasterMac(mac)) {
    if (status != ESP_NOW_SEND_SUCCESS)
      now.setIsMasterConnected(false);
    else
      now.setIsMasterConnected(true);
  }
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
          now.unsuscribeOnReceived();
          now.onReceived(onConfirmRegistrationReceivedCallback);
        }
      }
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

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void sendDataTask(void* parameter) {
  while (1) {
    now.sendDataMsg(sensorData.temp, sensorData.hum);

    vTaskDelay(pdMS_TO_TICKS(5000));  // 5s
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
  if (!syncModeState) {
    disableDataTransfer();

    if (!now.reset()) ESP.restart();

    if (!now.registerBroadcastPeer()) return;

    now.onReceived(onSyncReceivedCallback);

    if (blinkLEDTaskHandler == NULL) {
      xTaskCreatePinnedToCore(blinkLEDTask, "Blink LED", 2048, NULL, 2,
                              &blinkLEDTaskHandler, 1);
    }

    syncModeTimeoutTimerHandler = xTimerCreate(
        "Sync Mode Timeout", pdMS_TO_TICKS(NowManager::SYNC_MODE_TIMEOUT),
        pdFALSE, (void*)0,
        syncModeTimeoutCallback);  // 30s

    if (syncModeTimeoutTimerHandler != NULL)
      xTimerStart(syncModeTimeoutTimerHandler, 0);

    syncModeState = true;
  }
}

void endSyncMode() {
  if (syncModeState) {
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

    now.init();
    now.onReceived(onReceivedCallback);
    now.onSend(onSendCallback);
    enableDataTransfer;
    registerMasterNode();

    syncModeState = false;
  }
}

void enableDataTransfer() {
  now.setDataTransfer(true);

  if (sendDataTaskHandler == NULL) {
    xTaskCreatePinnedToCore(sendDataTask, "Send Temperature - Humidity", 2048,
                            NULL, 2, &sendDataTaskHandler, 1);
  }
}

void disableDataTransfer() {
  now.setDataTransfer(false);

  if (sendDataTaskHandler != NULL) {
    vTaskDelete(sendDataTaskHandler);
    sendDataTaskHandler = NULL;
  }
}

void registerMasterNode() {
  uint8_t masterMac[6];
  config.copyMasterMac(masterMac);
  if (!now.registerMasterPeer(masterMac)) disableDataTransfer();
}
