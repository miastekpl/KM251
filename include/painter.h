/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Moduł sterowania procesem malowania pasów drogowych
 * =============================================================
 */

#ifndef KM251_PAINTER_H
#define KM251_PAINTER_H

#include <Arduino.h>
#include "config.h"
#include "patterns.h"
#include "guns.h"
#include "encoder.h"

// =============================================================
// Stan procesu malowania
// =============================================================
enum class PaintState : uint8_t {
    IDLE,           // Gotowy do pracy
    PAINTING,       // Malowanie w toku
    PAUSED,         // Wstrzymane (pistolety wyłączone)
    STOPPED,        // Zatrzymane
};

// =============================================================
// Statystyki sesji
// =============================================================
struct PaintStats {
    uint32_t totalDistance_mm;       // Łączny dystans (mm)
    float    totalArea_m2;           // Łączna powierzchnia (m²)
    uint32_t sessionStartTime;      // Czas rozpoczęcia sesji
    uint32_t paintingTime_ms;       // Czas efektywnego malowania
    uint32_t gunOnTime_ms[NUM_GUNS]; // Czas pracy każdego pistoletu
};

// =============================================================
// Klasa procesu malowania
// =============================================================
class PaintProcess {
public:
    PaintProcess();
    void begin();
    void update();  // Wywoływać w każdej iteracji loop()

    // Sterowanie
    void start();
    void pause();
    void resume();
    void stop();

    // Zmiana wzorca w locie
    void setAxisPattern(PatternID id);
    void setEdgePattern(PatternID id);
    void toggleReverse();   // Odwracanie P-3a/P-3b

    // Odczyt stanu
    PaintState getState() const { return _state; }
    const char* getStateString() const;
    uint32_t getDistance_mm() const;
    float getDistance_m() const;
    float getSpeed_kmh() const;
    const PaintStats& getStats() const { return _stats; }

    // Reset statystyk
    void resetStats();

private:
    PaintState _state;
    PaintStats _stats;
    uint32_t _lastUpdateMs;
    uint32_t _pauseStartMs;
    uint32_t _totalPauseMs;

    void _updateGuns();
    void _updateStats();
};

extern PaintProcess paintProcess;

#endif // KM251_PAINTER_H
