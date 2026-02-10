/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja obsługi przycisków fizycznych
 * =============================================================
 */

#include "buttons.h"

ButtonManager buttonManager;

ButtonManager::ButtonManager()
    : _hasEvent(false)
{
    memset(_buttons, 0, sizeof(_buttons));
}

void ButtonManager::begin()
{
    _buttons[static_cast<uint8_t>(ButtonID::START_PAUSE)].pin = PIN_BTN_START;
    _buttons[static_cast<uint8_t>(ButtonID::STOP)].pin = PIN_BTN_STOP;
    _buttons[static_cast<uint8_t>(ButtonID::SELECTOR)].pin = PIN_BTN_SELECTOR;

    for (uint8_t i = 0; i < static_cast<uint8_t>(ButtonID::COUNT); i++) {
        pinMode(_buttons[i].pin, INPUT_PULLUP);
        _buttons[i].pressed = false;
        _buttons[i].lastReading = true;
        _buttons[i].longPressHandled = false;
        _buttons[i].lastDebounceTime = 0;
        _buttons[i].pressStartTime = 0;
        _buttons[i].pendingEvent = ButtonEvent::NONE;
    }

    Serial.println("[BUTTONS] Zainicjalizowano 3 przyciski");
}

void ButtonManager::update()
{
    _hasEvent = false;
    for (uint8_t i = 0; i < static_cast<uint8_t>(ButtonID::COUNT); i++) {
        _updateButton(i);
        if (_buttons[i].pendingEvent != ButtonEvent::NONE) {
            _hasEvent = true;
        }
    }
}

void ButtonManager::_updateButton(uint8_t index)
{
    ButtonState& btn = _buttons[index];
    bool reading = !digitalRead(btn.pin);  // INPUT_PULLUP: LOW = pressed
    uint32_t now = millis();

    if (reading != btn.lastReading) {
        btn.lastDebounceTime = now;
    }
    btn.lastReading = reading;

    if ((now - btn.lastDebounceTime) < BTN_DEBOUNCE_MS) {
        return;
    }

    if (reading && !btn.pressed) {
        btn.pressed = true;
        btn.pressStartTime = now;
        btn.longPressHandled = false;
        btn.pendingEvent = ButtonEvent::NONE;
    }
    else if (reading && btn.pressed && !btn.longPressHandled) {
        if ((now - btn.pressStartTime) >= BTN_LONG_PRESS_MS) {
            btn.pendingEvent = ButtonEvent::LONG_PRESS;
            btn.longPressHandled = true;
        }
    }
    else if (!reading && btn.pressed) {
        btn.pressed = false;
        if (!btn.longPressHandled) {
            btn.pendingEvent = ButtonEvent::CLICK;
        }
    }
}

ButtonEvent ButtonManager::getEvent(ButtonID id)
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= static_cast<uint8_t>(ButtonID::COUNT)) return ButtonEvent::NONE;

    ButtonEvent evt = _buttons[idx].pendingEvent;
    _buttons[idx].pendingEvent = ButtonEvent::NONE;
    return evt;
}

bool ButtonManager::isPressed(ButtonID id) const
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= static_cast<uint8_t>(ButtonID::COUNT)) return false;
    return _buttons[idx].pressed;
}

bool ButtonManager::hasAnyEvent() const
{
    return _hasEvent;
}
