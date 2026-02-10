/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Moduł obsługi przycisków fizycznych
 * =============================================================
 */

#ifndef KM251_BUTTONS_H
#define KM251_BUTTONS_H

#include <Arduino.h>
#include "config.h"

// =============================================================
// Zdarzenia przycisków
// =============================================================
enum class ButtonEvent : uint8_t {
    NONE = 0,
    CLICK,          // Krótkie kliknięcie (< 1s)
    LONG_PRESS,     // Długie naciśnięcie (>= 1s)
};

// =============================================================
// Identyfikatory przycisków
// =============================================================
enum class ButtonID : uint8_t {
    START_PAUSE = 0,
    STOP,
    SELECTOR,
    COUNT
};

// =============================================================
// Struktura wewnętrzna stanu przycisku
// =============================================================
struct ButtonState {
    uint8_t     pin;
    bool        pressed;
    bool        lastReading;
    bool        longPressHandled;
    uint32_t    lastDebounceTime;
    uint32_t    pressStartTime;
    ButtonEvent pendingEvent;
};

// =============================================================
// Klasa zarządzania przyciskami
// =============================================================
class ButtonManager {
public:
    ButtonManager();
    void begin();
    void update();

    // Pobierz zdarzenie (jednorazowe, konsumuje zdarzenie)
    ButtonEvent getEvent(ButtonID id);

    // Czy przycisk aktualnie wciśnięty
    bool isPressed(ButtonID id) const;

    // Czy jest jakiekolwiek zdarzenie
    bool hasAnyEvent() const;

private:
    ButtonState _buttons[static_cast<uint8_t>(ButtonID::COUNT)];
    bool _hasEvent;

    void _updateButton(uint8_t index);
};

extern ButtonManager buttonManager;

#endif // KM251_BUTTONS_H
