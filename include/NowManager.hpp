#pragma once

#include <Arduino.h>
#include <esp_now.h>

class NowManager {
 public:
  static constexpr uint32_t SYNC_MODE_TIMEOUT = 30000;

  enum class NodeType {
    TEMPERATURE = 0x1A,
    HUMIDITY = 0x02,
  };

  enum class MessageType {
    SYNC_BROADCAST = 0x55,
    REGISTRATION = 0xAA,
    CONFIRM_REGISTRATION = 0xCC,
    TEMPERATURE_HUMIDITY = 0x1A
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

  struct TemperatureHumidityMsg {
    uint8_t msgType = static_cast<uint8_t>(MessageType::TEMPERATURE_HUMIDITY);
    float temp;
    float hum;
    uint8_t crc;
  };
#pragma pack(pop)

  bool init();
  bool stop();
  bool reset();
  bool registerBroadcastPeer();
  bool registerMasterPeer(const uint8_t* masterMac);
  void onSend(esp_now_send_cb_t callback);
  void unsuscribeOnSend();
  void onReceived(esp_now_recv_cb_t callback);
  void unsuscribeOnReceived();
  bool sendRegistrationMsg();
  static bool validateMessage(MessageType expectedType, const uint8_t* data,
                              size_t length);
  bool sendTemperatureHumidityMsg(const float temp, const float hum);

 private:
  uint8_t _broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t _masterMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  bool _isMasterPeerRegistered = false;
  bool _isBroadcastPeerRegistered = false;

  static size_t _getMessageSize(MessageType type);
};