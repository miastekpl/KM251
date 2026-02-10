/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja zapisu ustawień do NVS (Preferences)
 * =============================================================
 */

#include "storage.h"
#include <Preferences.h>

StorageManager storageManager;

static Preferences prefs;
static const char* NVS_NAMESPACE = "km251";

StorageManager::StorageManager()
{
}

void StorageManager::begin()
{
    Serial.println("[STORAGE] Menedzer pamieci NVS gotowy");
}

// =============================================================
// Kalibracja
// =============================================================

void StorageManager::saveCalibration(float pulsesPerMM)
{
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putFloat("cal_ppmm", pulsesPerMM);
    prefs.putBool("cal_done", true);
    prefs.end();
    Serial.printf("[STORAGE] Zapisano kalibracje: %.4f imp/mm\n", pulsesPerMM);
}

float StorageManager::loadCalibration()
{
    prefs.begin(NVS_NAMESPACE, true);
    float val = prefs.getFloat("cal_ppmm", (float)ENC_DEFAULT_PULSES_PER_M / 1000.0f);
    prefs.end();
    return val;
}

bool StorageManager::hasCalibration()
{
    prefs.begin(NVS_NAMESPACE, true);
    bool done = prefs.getBool("cal_done", false);
    prefs.end();
    return done;
}

// =============================================================
// Statystyki
// =============================================================

void StorageManager::saveTotalDistance(uint32_t distance_mm)
{
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUInt("tot_dist", distance_mm);
    prefs.end();
}

uint32_t StorageManager::loadTotalDistance()
{
    prefs.begin(NVS_NAMESPACE, true);
    uint32_t val = prefs.getUInt("tot_dist", 0);
    prefs.end();
    return val;
}

void StorageManager::saveTotalArea(float area_m2)
{
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putFloat("tot_area", area_m2);
    prefs.end();
}

float StorageManager::loadTotalArea()
{
    prefs.begin(NVS_NAMESPACE, true);
    float val = prefs.getFloat("tot_area", 0.0f);
    prefs.end();
    return val;
}

// =============================================================
// Jasność
// =============================================================

void StorageManager::saveBrightness(uint8_t brightness)
{
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar("bright", brightness);
    prefs.end();
}

uint8_t StorageManager::loadBrightness()
{
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t val = prefs.getUChar("bright", TFT_BL_DEFAULT);
    prefs.end();
    return val;
}

// =============================================================
// Ostatnie wzorce
// =============================================================

void StorageManager::saveLastAxisPattern(uint8_t patternIndex)
{
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar("last_axis", patternIndex);
    prefs.end();
}

uint8_t StorageManager::loadLastAxisPattern()
{
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t val = prefs.getUChar("last_axis", 0);
    prefs.end();
    return val;
}

void StorageManager::saveLastEdgePattern(uint8_t patternIndex)
{
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar("last_edge", patternIndex);
    prefs.end();
}

uint8_t StorageManager::loadLastEdgePattern()
{
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t val = prefs.getUChar("last_edge", static_cast<uint8_t>(PatternID::P7D));
    prefs.end();
    return val;
}

// =============================================================
// Reset
// =============================================================

void StorageManager::resetAll()
{
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
    Serial.println("[STORAGE] Wszystkie ustawienia zresetowane");
}
