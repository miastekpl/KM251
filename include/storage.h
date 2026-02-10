/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Moduł zapisu ustawień do NVS (Preferences)
 * =============================================================
 */

#ifndef KM251_STORAGE_H
#define KM251_STORAGE_H

#include <Arduino.h>
#include "config.h"

// =============================================================
// Klasa obsługi trwałego zapisu
// =============================================================
class StorageManager {
public:
    StorageManager();
    void begin();

    // Kalibracja enkodera
    void saveCalibration(float pulsesPerMM);
    float loadCalibration();
    bool hasCalibration();

    // Statystyki
    void saveTotalDistance(uint32_t distance_mm);
    uint32_t loadTotalDistance();
    void saveTotalArea(float area_m2);
    float loadTotalArea();

    // Jasność ekranu
    void saveBrightness(uint8_t brightness);
    uint8_t loadBrightness();

    // Ostatni wybrany wzorzec
    void saveLastAxisPattern(uint8_t patternIndex);
    uint8_t loadLastAxisPattern();
    void saveLastEdgePattern(uint8_t patternIndex);
    uint8_t loadLastEdgePattern();

    // Reset do domyślnych
    void resetAll();
};

extern StorageManager storageManager;

#endif // KM251_STORAGE_H
