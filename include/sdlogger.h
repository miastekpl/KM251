/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Moduł logowania na kartę SD (czytnik zintegrowany z TFT)
 * =============================================================
 */

#ifndef KM251_SDLOGGER_H
#define KM251_SDLOGGER_H

#include <Arduino.h>
#include "config.h"

// =============================================================
// Klasa obsługi karty SD
// =============================================================
class SDLogger {
public:
    SDLogger();
    void begin();
    bool isReady() const { return _cardReady; }

    // Logowanie sesji malowania (CSV)
    void logPaintingData(float distance_m, float speed_kmh, float area_m2,
                         const char* axisPattern, const char* edgePattern,
                         uint8_t gunMask);

    // Zapis podsumowania sesji
    void logSessionSummary(float totalDistance_m, float totalArea_m2,
                           uint32_t durationSec);

    // Zapis kalibracji (backup)
    void saveCalibrationBackup(float pulsesPerMM);

    // Informacje o karcie
    uint64_t getCardSizeMB() const;
    uint64_t getFreeSpaceMB() const;

private:
    bool _cardReady;
    uint32_t _sessionId;
    uint32_t _lastLogMs;

    void _ensureDirectory(const char* path);
    String _getSessionFilename();
};

extern SDLogger sdLogger;

#endif // KM251_SDLOGGER_H
