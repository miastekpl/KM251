/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja procesu malowania pasów drogowych
 *
 * Logika: na podstawie dystansu z enkodera kołowego system
 * włącza/wyłącza pistolety zgodnie z aktywnym wzorcem.
 * Wzorce można zmieniać w locie podczas malowania.
 * =============================================================
 */

#include "painter.h"

PaintProcess paintProcess;

PaintProcess::PaintProcess()
    : _state(PaintState::IDLE)
    , _lastUpdateMs(0)
    , _pauseStartMs(0)
    , _totalPauseMs(0)
{
    memset(&_stats, 0, sizeof(_stats));
}

void PaintProcess::begin()
{
    _state = PaintState::IDLE;
    resetStats();
    Serial.println("[PAINTER] Proces malowania zainicjalizowany");
}

void PaintProcess::update()
{
    if (_state != PaintState::PAINTING) return;

    uint32_t now = millis();
    if ((now - _lastUpdateMs) < 5) return;  // 200 Hz
    _lastUpdateMs = now;

    _updateGuns();
    _updateStats();
}

// =============================================================
// Sterowanie procesem
// =============================================================

void PaintProcess::start()
{
    if (_state != PaintState::IDLE && _state != PaintState::STOPPED) return;

    wheelEncoder.resetDistance();
    _state = PaintState::PAINTING;
    _stats.sessionStartTime = millis();
    _totalPauseMs = 0;

    // Sygnał dźwiękowy start
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);

    Serial.println("[PAINTER] Malowanie rozpoczete");
}

void PaintProcess::pause()
{
    if (_state != PaintState::PAINTING) return;

    gunController.allOff();
    _state = PaintState::PAUSED;
    _pauseStartMs = millis();

    Serial.println("[PAINTER] Malowanie wstrzymane (pistolety OFF)");
}

void PaintProcess::resume()
{
    if (_state != PaintState::PAUSED) return;

    _totalPauseMs += (millis() - _pauseStartMs);
    _state = PaintState::PAINTING;

    digitalWrite(PIN_BUZZER, HIGH);
    delay(50);
    digitalWrite(PIN_BUZZER, LOW);

    Serial.println("[PAINTER] Malowanie wznowione");
}

void PaintProcess::stop()
{
    gunController.allOff();
    _state = PaintState::STOPPED;

    // Podwójny sygnał stop
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
    delay(50);
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);

    Serial.printf("[PAINTER] Malowanie zatrzymane. Dystans: %.1f m\n",
                  getDistance_m());

    _state = PaintState::IDLE;
}

// =============================================================
// Zmiana wzorca w locie
// =============================================================

void PaintProcess::setAxisPattern(PatternID id)
{
    patternManager.setActiveAxisPattern(id);
    // Natychmiastowa aktualizacja pistoletów przy następnym update()
}

void PaintProcess::setEdgePattern(PatternID id)
{
    patternManager.setActiveEdgePattern(id);
}

void PaintProcess::toggleReverse()
{
    patternManager.toggleReversed();
}

// =============================================================
// Odczyt stanu
// =============================================================

const char* PaintProcess::getStateString() const
{
    switch (_state) {
        case PaintState::IDLE:      return "Gotowy";
        case PaintState::PAINTING:  return "Malowanie";
        case PaintState::PAUSED:    return "Wstrzymano";
        case PaintState::STOPPED:   return "Zatrzymano";
        default:                    return "---";
    }
}

uint32_t PaintProcess::getDistance_mm() const
{
    return wheelEncoder.getDistanceMM();
}

float PaintProcess::getDistance_m() const
{
    return wheelEncoder.getDistanceM();
}

float PaintProcess::getSpeed_kmh() const
{
    return wheelEncoder.getSpeedKMH();
}

void PaintProcess::resetStats()
{
    memset(&_stats, 0, sizeof(_stats));
    gunController.resetStats();
}

// =============================================================
// Aktualizacja pistoletów na podstawie wzorca i pozycji
// =============================================================

void PaintProcess::_updateGuns()
{
    // Zabezpieczenie: poniżej MIN_PAINT_SPEED_KMH pistolety nie otworzą się
    float speed = wheelEncoder.getSpeedKMH();
    if (speed < MIN_PAINT_SPEED_KMH) {
        gunController.allOff();
        return;
    }

    uint32_t pos_mm = wheelEncoder.getDistanceMM();

    // Wzorzec osi jezdni
    PatternID axisPat = patternManager.getActiveAxisPattern();
    uint8_t axisMask = patternManager.getGunStateForPosition(pos_mm, axisPat);

    // Wzorzec krawędzi
    PatternID edgePat = patternManager.getActiveEdgePattern();
    uint8_t edgeMask = patternManager.getGunStateForPosition(pos_mm, edgePat);

    // Połącz maski — oś (bity 0-3) i krawędź (bity 4-5)
    uint8_t axisOnly = axisMask & 0x0F;  // P1-P4
    uint8_t edgeOnly = edgeMask & 0x30;  // P5-P6

    uint8_t combined = axisOnly | edgeOnly;
    gunController.setGunMask(combined);
}

// =============================================================
// Aktualizacja statystyk
// =============================================================

void PaintProcess::_updateStats()
{
    _stats.totalDistance_mm = wheelEncoder.getDistanceMM();

    // Oblicz powierzchnię dla obu wzorców
    PatternID axisPat = patternManager.getActiveAxisPattern();
    PatternID edgePat = patternManager.getActiveEdgePattern();

    _stats.totalArea_m2 =
        patternManager.calculatePaintedArea_m2(axisPat, _stats.totalDistance_mm) +
        patternManager.calculatePaintedArea_m2(edgePat, _stats.totalDistance_mm);

    uint32_t now = millis();
    _stats.paintingTime_ms = (now - _stats.sessionStartTime) - _totalPauseMs;

    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        _stats.gunOnTime_ms[i] = gunController.getGunOnTime(static_cast<GunID>(i));
    }
}
