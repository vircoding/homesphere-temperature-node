#pragma once

#include <Arduino.h>
#include <esp_now.h>

class NowManager {
 public:
#pragma pack(push, 1)  // Empaquetamiento estricto sin padding
  struct SyncBroadcastMsg {
    uint8_t msgType = 0x55;  // Mensaje de sincronizacion
    uint32_t pairingCode;    // Codigo unico generado
    uint8_t crc;             // XOR de todos los bytes
  };

  struct RegistrationMsg {
    uint8_t msgType = 0xAA;  // Mensaje de registro
    uint8_t nodeType;
    uint8_t firmwareVersion[3];
    uint8_t crc;
  };
#pragma pack(pop)

  struct TemperatureData {
    float hum;
    float temp;
    int node_id;
  };

  bool init();
  bool registerMasterPeer(const uint8_t* masterMac);
  bool registerSyncPeer();
  void setPMK(const String& key);
  void onSend(esp_now_send_cb_t callback);
  void onReceived(esp_now_recv_cb_t callback);
  bool sendRegistrationData();
  // bool sendTemperatureData(const float temp, const float hum);

 private:
  static constexpr uint32_t SYNC_MODE_TIMEOUT = 30000;

  uint8_t _masterMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
};