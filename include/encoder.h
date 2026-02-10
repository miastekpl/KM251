/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Moduł enkodera z funkcją kalibracji dystansu
 * =============================================================
 */

#ifndef KM251_ENCODER_H
#define KM251_ENCODER_H

#include <Arduino.h>
#include "config.h"

// =============================================================
// Stan kalibracji
// =============================================================
enum class CalibrationState : uint8_t {
    IDLE,               // Kalibracja nieaktywna
    WAITING_START,      // Czeka na START (początek pomiaru)
    MEASURING,          // Pomiar w toku (jedź 10m)
    COMPLETE,           // Pomiar zakończony, obliczono współczynnik
    ERROR               // Błąd kalibracji
};

// =============================================================
// Klasa enkodera z kalibracją
// =============================================================
class WheelEncoder {
public:
    WheelEncoder();
    void begin();
    void update();

    // Odczyt dystansu
    uint32_t getDistanceMM() const;     // Dystans od resetu (mm)
    float getDistanceM() const;          // Dystans od resetu (m)
    float getSpeedKMH() const;           // Prędkość (km/h)
    float getSpeedMS() const;            // Prędkość (m/s)
    int64_t getRawPulses() const;        // Surowe impulsy

    // Reset licznika dystansu
    void resetDistance();

    // === KALIBRACJA ===
    // Wejście w tryb kalibracji
    void startCalibration();
    // Rozpoczęcie pomiaru (naciśnięcie START - jedź)
    void beginMeasurement();
    // Zakończenie pomiaru (naciśnięcie START po 10m)
    void endMeasurement();
    // Anulowanie kalibracji
    void cancelCalibration();

    CalibrationState getCalibrationState() const { return _calState; }
    int64_t getCalibrationPulses() const { return _calPulses; }
    float getCalibrationFactor() const { return _pulsesPerMM; }
    bool isCalibrated() const { return _calibrated; }

    // Zapis/odczyt kalibracji
    void saveCalibration();
    void loadCalibration();

private:
    // Kalibracja
    float _pulsesPerMM;             // Impulsy na milimetr
    bool _calibrated;
    CalibrationState _calState;
    int64_t _calStartPulses;
    int64_t _calPulses;

    // Pomiar dystansu
    int64_t _distanceStartPulses;

    // Pomiar prędkości
    float _speedMS;
    int64_t _lastSpeedPulses;
    uint32_t _lastSpeedTime;

};

extern WheelEncoder wheelEncoder;

#endif // KM251_ENCODER_H
