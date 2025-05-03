#include "ButtonManager.hpp"

ButtonManager::ButtonManager(uint8_t buttonPin) : _PIN(buttonPin) {}

void ButtonManager::begin() {
  _debouncer.attach(_PIN, INPUT_PULLUP);
  _debouncer.interval(DEBOUNCE_INTERVAL);
}

void ButtonManager::update() {
  _debouncer.update();

  if (_debouncer.fell()) {
    _pressStartTime = millis();
    _longPressDetected = false;
    Serial.println("Presionado - comenzando a medir el tiempo");
  }

  if (_debouncer.read() == LOW) {
    unsigned long elapsed = millis() - _pressStartTime;

    if (elapsed >= LONG_PRESS_DURATION && !_longPressDetected) {
      _longPressDetected = true;
      _trigger(Event::LONG_PRESS);
    }
  }

  if (_debouncer.rose()) {
    unsigned long elapsed = millis() - _pressStartTime;

    // Solo ejecutar una accion corta si no se detecto ya una larga
    if (elapsed < LONG_PRESS_DURATION && !_longPressDetected) {
      _trigger(Event::SIMPLE_PRESS);
    }

    Serial.print("Tiempo de pulsacion: ");
    Serial.print(elapsed);
    Serial.println("ms");
  }
}

void ButtonManager::on(Event event, std::function<void()> callback) {
  _callbacks[event] = callback;
}

void ButtonManager::_trigger(Event event) {
  auto it = _callbacks.find(event);

  if (it != _callbacks.end() && it->second) {
    it->second();
  }
}
