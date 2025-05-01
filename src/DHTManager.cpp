#include "DHTManager.hpp"

DHTManager::DHTManager(uint8_t dataPin, Data& data)
    : _dht(dataPin, DHT11), _data(data) {}

void DHTManager::read() {
  _data.temp = _dht.readTemperature();
  _data.hum = _dht.readHumidity();
}