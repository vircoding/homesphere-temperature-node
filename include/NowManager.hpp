#pragma once

#include <Arduino.h>
#include <esp_now.h>

class NowManager {
 public:
  enum class MessageType {
    SYNC_BROADCAST = 0x55,
    REGISTRATION = 0xAA,
    CONFIRM_REGISTRATION = 0xCC
  };

#pragma pack(push, 1)  // Empaquetamiento estricto sin padding
  struct SyncBroadcastMsg {
    uint8_t msgType = static_cast<uint8_t>(
        MessageType::SYNC_BROADCAST);  // Mensaje de sincronizacion
    uint32_t pairingCode;              // Codigo unico generado
    uint8_t crc;                       // XOR de todos los bytes
  };

  struct RegistrationMsg {
    uint8_t msgType =
        static_cast<uint8_t>(MessageType::REGISTRATION);  // Mensaje de registro
    uint8_t nodeType;
    uint8_t firmwareVersion[3];
    uint8_t crc;
  };

  struct ConfirmRegistrationMsg {
    uint8_t msgType = static_cast<uint8_t>(MessageType::CONFIRM_REGISTRATION);
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
  void unsuscribeOnSend();
  void onReceived(esp_now_recv_cb_t callback);
  void unsuscribeOnReceived();
  bool sendRegistrationMsg();
  static bool validateMessage(MessageType expectedType, const uint8_t* data,
                              size_t length);
  // bool sendTemperatureData(const float temp, const float hum);

 private:
  static constexpr uint32_t SYNC_MODE_TIMEOUT = 30000;

  uint8_t _masterMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  static size_t _getMessageSize(MessageType type);
};