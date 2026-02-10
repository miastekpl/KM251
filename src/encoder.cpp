/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja enkodera z kalibracją dystansu
 *
 * Kalibracja: wejdź w tryb, naciśnij START, przejedź dokładnie
 * 10 metrów, naciśnij START ponownie. System obliczy
 * współczynnik impulsy/mm.
 * =============================================================
 */

#include "encoder.h"
#include <ESP32Encoder.h>

WheelEncoder wheelEncoder;

static ESP32Encoder hwEncoder;

WheelEncoder::WheelEncoder()
    : _pulsesPerMM((float)ENC_DEFAULT_PULSES_PER_M / 1000.0f)
    , _calibrated(false)
    , _calState(CalibrationState::IDLE)
    , _calStartPulses(0)
    , _calPulses(0)
    , _distanceStartPulses(0)
    , _speedMS(0)
    , _lastSpeedPulses(0)
    , _lastSpeedTime(0)
{
}

void WheelEncoder::begin()
{
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    hwEncoder.attachFullQuad(PIN_ENC_A, PIN_ENC_B);
    hwEncoder.setCount(0);

    _distanceStartPulses = 0;
    _lastSpeedPulses = 0;
    _lastSpeedTime = millis();
    loadCalibration();

    Serial.printf("[ENCODER] Zainicjalizowano. Skalibrowany: %s, %.4f imp/mm\n",
                  _calibrated ? "TAK" : "NIE", _pulsesPerMM);
}

void WheelEncoder::update()
{
    // Oblicz prędkość co 200ms
    uint32_t now = millis();
    if ((now - _lastSpeedTime) >= 200) {
        int64_t currentPulses = hwEncoder.getCount();
        int64_t deltaPulses = currentPulses - _lastSpeedPulses;
        uint32_t deltaTime = now - _lastSpeedTime;

        if (_pulsesPerMM > 0 && deltaTime > 0) {
            float deltaMM = (float)deltaPulses / _pulsesPerMM;
            _speedMS = (deltaMM / 1000.0f) / (deltaTime / 1000.0f);
        }

        _lastSpeedPulses = currentPulses;
        _lastSpeedTime = now;
    }
}

// =============================================================
// Odczyt dystansu
// =============================================================

uint32_t WheelEncoder::getDistanceMM() const
{
    int64_t pulses = hwEncoder.getCount() - _distanceStartPulses;
    if (pulses < 0) pulses = -pulses;
    if (_pulsesPerMM <= 0) return 0;
    return (uint32_t)((float)pulses / _pulsesPerMM);
}

float WheelEncoder::getDistanceM() const
{
    return getDistanceMM() / 1000.0f;
}

float WheelEncoder::getSpeedKMH() const
{
    return _speedMS * 3.6f;
}

float WheelEncoder::getSpeedMS() const
{
    return _speedMS;
}

int64_t WheelEncoder::getRawPulses() const
{
    return hwEncoder.getCount();
}

void WheelEncoder::resetDistance()
{
    _distanceStartPulses = hwEncoder.getCount();
}

// =============================================================
// KALIBRACJA
// =============================================================

void WheelEncoder::startCalibration()
{
    _calState = CalibrationState::WAITING_START;
    _calPulses = 0;
    Serial.println("[ENCODER] Kalibracja: czekam na START");
}

void WheelEncoder::beginMeasurement()
{
    if (_calState != CalibrationState::WAITING_START) return;

    _calStartPulses = hwEncoder.getCount();
    _calState = CalibrationState::MEASURING;
    Serial.println("[ENCODER] Kalibracja: pomiar rozpoczety - jedz 10 metrow");
}

void WheelEncoder::endMeasurement()
{
    if (_calState != CalibrationState::MEASURING) return;

    int64_t endPulses = hwEncoder.getCount();
    _calPulses = endPulses - _calStartPulses;
    if (_calPulses < 0) _calPulses = -_calPulses;

    if (_calPulses < 10) {
        // Za mało impulsów - błąd
        _calState = CalibrationState::ERROR;
        Serial.println("[ENCODER] Kalibracja: BLAD - za malo impulsow");
        return;
    }

    // Oblicz współczynnik: impulsy / mm
    _pulsesPerMM = (float)_calPulses / (float)ENC_CALIBRATION_DIST_MM;
    _calibrated = true;
    _calState = CalibrationState::COMPLETE;

    Serial.printf("[ENCODER] Kalibracja OK: %lld impulsow / %d mm = %.4f imp/mm\n",
                  (long long)_calPulses, ENC_CALIBRATION_DIST_MM, _pulsesPerMM);

    saveCalibration();
}

void WheelEncoder::cancelCalibration()
{
    _calState = CalibrationState::IDLE;
    Serial.println("[ENCODER] Kalibracja anulowana");
}

// =============================================================
// Zapis/odczyt kalibracji (delegowane do StorageManager)
// =============================================================

void WheelEncoder::saveCalibration()
{
    // Implementacja w storage.cpp — tu tylko placeholder
    // storageManager.saveCalibration(_pulsesPerMM) wywoływany z main
}

void WheelEncoder::loadCalibration()
{
    // Implementacja w storage.cpp — tu tylko placeholder
    // _pulsesPerMM = storageManager.loadCalibration();
}
