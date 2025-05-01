#include "NowManager.hpp"

NowManager::NowManager(const uint8_t masterMac[6]) {
  memcpy(_masterMac, masterMac, 6);
}

bool NowManager::init() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return false;
  }

  Serial.println("ESP-NOW inicializado...");
  return true;
}

void NowManager::onSend(esp_now_send_cb_t callback) {
  esp_now_register_send_cb(callback);
}

String NowManager::getMacStr(const uint8_t mac[6]) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(macStr);
}

bool NowManager::registerMasterPeer() {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, _masterMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error registrando peer");
    return false;
  }

  Serial.println("Master peer registrado. Enviando datos...");
  return true;
}

bool NowManager::sendTemperatureData(const float temp, const float hum) {
  NowManager::TemperatureData data = {
    hum : hum,
    temp : temp,
    node_id : 2,
  };

  // Enviar mensaje
  if (esp_now_send(_masterMac, (uint8_t*)&data, sizeof(data)) == ESP_OK) {
    Serial.println("Mensaje enviado");
  } else {
    Serial.println("Error enviando mensaje");
  }

  return false;
}
