#include "DHTManager.hpp"

DHTManager::DHTManager(const gpio_num_t dataPin, Data& data)
    : _dht(dataPin, DHT11), _data(data) {}

void DHTManager::read() {
  const float readedTemp = _dht.readTemperature();
  const float readedHum = _dht.readHumidity();

  if (!isnan(readedTemp)) _data.temp = readedTemp;
  if (!isnan(readedHum)) _data.hum = readedHum;
}