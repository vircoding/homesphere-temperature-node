#pragma once

#include <Bounce2.h>

#include <functional>
#include <map>

class ButtonManager {
 public:
  enum class Event {
    SIMPLE_PRESS,
    LONG_PRESS,
  };

  ButtonManager(uint8_t buttonPin);
  void begin();
  void update();
  void on(Event event, std::function<void()> callback);

 private:
  static constexpr uint16_t LONG_PRESS_DURATION = 5000;
  static constexpr uint8_t DEBOUNCE_INTERVAL = 50;

  Bounce _debouncer;

  const uint8_t _PIN;
  unsigned long _pressStartTime;
  bool _longPressDetected;

  // Callbacks de eventos
  std::map<Event, std::function<void()>> _callbacks;

  // Metodos privados
  void _trigger(Event event);
};