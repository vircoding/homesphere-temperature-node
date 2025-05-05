#include "NowManager.hpp"

#include <WiFi.h>

#include "Utils.hpp"

bool NowManager::init() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return false;
  }

  return true;
}

void NowManager::onSend(esp_now_send_cb_t callback) {
  esp_now_register_send_cb(callback);
}

void NowManager::unsuscribeOnSend() { esp_now_unregister_send_cb(); }

void NowManager::onReceived(esp_now_recv_cb_t callback) {
  esp_now_register_recv_cb(callback);
}

void NowManager::unsuscribeOnReceived() { esp_now_unregister_recv_cb(); }

bool NowManager::sendRegistrationMsg() {
  NowManager::RegistrationMsg msg;
  msg.nodeType = 0x1A;
  msg.firmwareVersion[0] = 1;
  msg.firmwareVersion[1] = 0;
  msg.firmwareVersion[2] = 0;

  // Generate CRC8
  addCRC8(msg);

  return esp_now_send(_masterMac, (uint8_t*)&msg, sizeof(msg)) == ESP_OK;
}

bool NowManager::validateMessage(MessageType expectedType, const uint8_t* data,
                                 size_t length) {
  // Evitar mensajes vacíos
  if (length < 1) return false;

  auto msgType = static_cast<MessageType>(data[0]);
  return (msgType == expectedType) && (length == _getMessageSize(expectedType));
}

size_t NowManager::_getMessageSize(MessageType type) {
  switch (type) {
    case MessageType::SYNC_BROADCAST:
      return sizeof(SyncBroadcastMsg);

    case MessageType::REGISTRATION:
      return sizeof(RegistrationMsg);

    case MessageType::CONFIRM_REGISTRATION:
      return sizeof(ConfirmRegistrationMsg);

    default:
      return 0;  // Tipo desconocido
  }
}

bool NowManager::registerMasterPeer(const uint8_t* masterMac) {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, masterMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error registrando peer");
    return false;
  }

  memcpy(_masterMac, masterMac, 6);

  return true;
}

bool NowManager::registerSyncPeer() {
  const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastMac, 6);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error registrando peer");
    return false;
  }

  return true;
}

void NowManager::setPMK(const String& key) {
  esp_now_set_pmk((uint8_t*)key.c_str());
}

// bool NowManager::sendTemperatureData(const float temp, const float hum) {
//   NowManager::TemperatureData data = {
//     hum : hum,
//     temp : temp,
//     node_id : 2,
//   };

//   // Enviar mensaje
//   if (esp_now_send(_masterMac, (uint8_t*)&data, sizeof(data)) == ESP_OK) {
//     Serial.print("Mensaje enviado a: ");
//     Serial.println(macToString(_masterMac));

//     return true;
//   } else {
//     Serial.print("Error enviando mensaje a: ");
//     Serial.println(macToString(_masterMac));

//     return false;
//   }
// }
