/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja sterowania pistoletami (6x przekaźnik)
 * =============================================================
 */

#include "guns.h"

GunController gunController;

static const uint8_t GUN_PINS[NUM_GUNS] = {
    PIN_GUN_1, PIN_GUN_2, PIN_GUN_3,
    PIN_GUN_4, PIN_GUN_5, PIN_GUN_6
};

static const uint8_t GUN_WIDTHS[NUM_GUNS] = {
    GUN1_WIDTH_CM, GUN2_WIDTH_CM, GUN3_WIDTH_CM,
    GUN4_WIDTH_CM, GUN5_WIDTH_CM, GUN6_WIDTH_CM
};

static const GunCategory GUN_CATEGORIES[NUM_GUNS] = {
    GunCategory::AXIS, GunCategory::AXIS, GunCategory::AXIS,
    GunCategory::AXIS, GunCategory::EDGE, GunCategory::EDGE
};

GunController::GunController()
{
    memset(_guns, 0, sizeof(_guns));
}

void GunController::begin()
{
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        _guns[i].pin = GUN_PINS[i];
        _guns[i].widthCm = GUN_WIDTHS[i];
        _guns[i].category = GUN_CATEGORIES[i];
        _guns[i].active = false;
        _guns[i].enabled = true;
        _guns[i].totalOnTime_ms = 0;
        _guns[i].lastOnTime = 0;

        pinMode(_guns[i].pin, OUTPUT);
        digitalWrite(_guns[i].pin, LOW);
    }

    Serial.println("[GUNS] Zainicjalizowano 6 pistoletow");
}

void GunController::update()
{
    uint32_t now = millis();
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        if (_guns[i].active && _guns[i].lastOnTime > 0) {
            _guns[i].totalOnTime_ms += (now - _guns[i].lastOnTime);
            _guns[i].lastOnTime = now;
        }
    }
}

// =============================================================
// Sterowanie pojedynczym pistoletem
// =============================================================

void GunController::setGun(GunID id, bool on)
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= NUM_GUNS) return;

    if (on && !_guns[idx].enabled) return;

    if (on && !_guns[idx].active) {
        _guns[idx].lastOnTime = millis();
    } else if (!on && _guns[idx].active) {
        _guns[idx].totalOnTime_ms += (millis() - _guns[idx].lastOnTime);
        _guns[idx].lastOnTime = 0;
    }

    _guns[idx].active = on;
    _setPin(id, on);
}

void GunController::enableGun(GunID id, bool enable)
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= NUM_GUNS) return;

    _guns[idx].enabled = enable;
    if (!enable && _guns[idx].active) {
        setGun(id, false);
    }
}

// =============================================================
// Sterowanie z maski bitowej
// =============================================================

void GunController::setGunMask(uint8_t mask)
{
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        bool on = (mask >> i) & 1;
        setGun(static_cast<GunID>(i), on);
    }
}

uint8_t GunController::getGunMask() const
{
    uint8_t mask = 0;
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        if (_guns[i].active) {
            mask |= (1 << i);
        }
    }
    return mask;
}

// =============================================================
// Wyłączenie wszystkich
// =============================================================

void GunController::allOff()
{
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        setGun(static_cast<GunID>(i), false);
    }
}

// =============================================================
// Odczyt stanu
// =============================================================

bool GunController::isGunActive(GunID id) const
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= NUM_GUNS) return false;
    return _guns[idx].active;
}

bool GunController::isGunEnabled(GunID id) const
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= NUM_GUNS) return false;
    return _guns[idx].enabled;
}

bool GunController::isAnyGunActive() const
{
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        if (_guns[i].active) return true;
    }
    return false;
}

const GunState& GunController::getGunState(GunID id) const
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= NUM_GUNS) idx = 0;
    return _guns[idx];
}

// =============================================================
// Statystyki
// =============================================================

uint32_t GunController::getGunOnTime(GunID id) const
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= NUM_GUNS) return 0;
    return _guns[idx].totalOnTime_ms;
}

void GunController::resetStats()
{
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        _guns[i].totalOnTime_ms = 0;
    }
}

// =============================================================
// Diagnostyka
// =============================================================

void GunController::testGun(GunID id, uint32_t duration_ms)
{
    setGun(id, true);
    delay(duration_ms);
    setGun(id, false);
}

void GunController::testAllGuns(uint32_t duration_ms)
{
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        testGun(static_cast<GunID>(i), duration_ms);
        delay(200);
    }
}

// =============================================================
// Sterowanie sprzętowe
// =============================================================

void GunController::_setPin(GunID id, bool on)
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= NUM_GUNS) return;
    digitalWrite(_guns[idx].pin, on ? HIGH : LOW);
}
