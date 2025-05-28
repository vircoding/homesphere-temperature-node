#pragma once

#include <Arduino.h>
#include <esp_now.h>

class NowManager {
 public:
  static constexpr uint32_t SYNC_MODE_TIMEOUT = 30000;
  static constexpr uint8_t NODE_TYPE = 0x1A;
  static constexpr uint8_t FIRMWARE_VERSION[3] = {1, 0, 0};

  enum class MessageType {
    SYNC_BROADCAST = 0x55,
    REGISTRATION = 0xAA,
    CONFIRM_REGISTRATION = 0xCC,
    TEMPERATURE_HUMIDITY = 0x1A,
    PING = 0x11
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

  struct PingMsg {
    uint8_t msgType = static_cast<uint8_t>(MessageType::PING);
  };
#pragma pack(pop)  // Empaquetamiento predeterminado (con padding)

  bool init();
  bool stop();
  bool reset();
  bool registerBroadcastPeer();
  bool registerMasterPeer(const uint8_t* masterMac);
  void onSend(esp_now_send_cb_t callback);
  void unsuscribeOnSend();
  void onReceived(esp_now_recv_cb_t callback);
  void unsuscribeOnReceived();
  static bool validateMessage(MessageType expectedType, const uint8_t* data,
                              size_t length);
  bool isMasterMac(const uint8_t* mac);
  bool sendRegistrationMsg();
  bool sendDataMsg(const float temp, const float hum);
  void setDataTransfer(const bool state);
  void setIsMasterConnected(const bool state);

 private:
  bool _isDataTransferEnabled = false;
  bool _isMasterConnected = false;
  uint8_t _broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t _masterMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  bool _isMasterPeerRegistered = false;
  bool _isBroadcastPeerRegistered = false;

  static size_t _getMessageSize(MessageType type);
};