#include "ConfigManager.hpp"

#include <LittleFS.h>

bool ConfigManager::init() {
  if (!LittleFS.begin()) return false;
  return _loadConfig();
}

bool ConfigManager::_loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) return false;

  JsonDocument doc;
  if (deserializeJson(doc, configFile)) {
    configFile.close();
    return false;
  }

  _meshConfig.ssid = doc["mesh_ssid"].as<String>();
  _meshConfig.password = doc["mesh_password"].as<String>();

  configFile.close();
  return true;
}