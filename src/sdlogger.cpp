/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja logowania na kartę SD
 *
 * Czytnik SD zintegrowany z wyświetlaczem ILI9341.
 * Współdzielona magistrala SPI (MOSI=11, MISO=13, SCK=12).
 * Osobny CS na GPIO38.
 *
 * Struktura plików na SD:
 *   /km251/sessions/sesja_XXXX.csv  — logi sesji malowania
 *   /km251/calibration.txt          — backup kalibracji
 * =============================================================
 */

#include "sdlogger.h"
#include <SD.h>
#include <SPI.h>

SDLogger sdLogger;

SDLogger::SDLogger()
    : _cardReady(false)
    , _sessionId(0)
    , _lastLogMs(0)
{
}

void SDLogger::begin()
{
    if (!SD.begin(PIN_SD_CS)) {
        _cardReady = false;
        Serial.println("[SD] Brak karty SD lub blad inicjalizacji");
        return;
    }

    _cardReady = true;
    uint8_t cardType = SD.cardType();
    const char* typeStr = "NIEZNANY";
    if (cardType == CARD_MMC)       typeStr = "MMC";
    else if (cardType == CARD_SD)   typeStr = "SD";
    else if (cardType == CARD_SDHC) typeStr = "SDHC";

    Serial.printf("[SD] Karta %s, rozmiar: %llu MB\n", typeStr,
                  (unsigned long long)(SD.cardSize() / (1024 * 1024)));

    _ensureDirectory("/km251");
    _ensureDirectory("/km251/sessions");

    // Znajdź kolejny numer sesji
    _sessionId = 1;
    while (SD.exists(_getSessionFilename().c_str()) && _sessionId < 9999) {
        _sessionId++;
    }

    Serial.printf("[SD] Nastepna sesja: %04lu\n", (unsigned long)_sessionId);
}

void SDLogger::logPaintingData(float distance_m, float speed_kmh, float area_m2,
                                const char* axisPattern, const char* edgePattern,
                                uint8_t gunMask)
{
    if (!_cardReady) return;

    // Loguj max co 500ms
    uint32_t now = millis();
    if ((now - _lastLogMs) < 500) return;
    _lastLogMs = now;

    File f = SD.open(_getSessionFilename().c_str(), FILE_APPEND);
    if (!f) return;

    // Jeśli plik jest nowy, dodaj nagłówek CSV
    if (f.size() == 0) {
        f.println("czas_ms;dystans_m;predkosc_kmh;powierzchnia_m2;wzorzec_os;wzorzec_kraw;maski_pist");
    }

    char line[128];
    snprintf(line, sizeof(line), "%lu;%.3f;%.1f;%.4f;%s;%s;0x%02X",
             (unsigned long)now, distance_m, speed_kmh, area_m2,
             axisPattern, edgePattern, gunMask);
    f.println(line);
    f.close();
}

void SDLogger::logSessionSummary(float totalDistance_m, float totalArea_m2,
                                  uint32_t durationSec)
{
    if (!_cardReady) return;

    File f = SD.open("/km251/sessions/podsumowania.csv", FILE_APPEND);
    if (!f) return;

    if (f.size() == 0) {
        f.println("sesja;dystans_m;powierzchnia_m2;czas_s");
    }

    char line[64];
    snprintf(line, sizeof(line), "%04lu;%.1f;%.2f;%lu",
             (unsigned long)_sessionId, totalDistance_m, totalArea_m2,
             (unsigned long)durationSec);
    f.println(line);
    f.close();

    _sessionId++;

    Serial.printf("[SD] Zapisano podsumowanie sesji\n");
}

void SDLogger::saveCalibrationBackup(float pulsesPerMM)
{
    if (!_cardReady) return;

    File f = SD.open("/km251/calibration.txt", FILE_WRITE);
    if (!f) return;

    f.printf("# KM251 Kalibracja Enkodera\n");
    f.printf("pulses_per_mm=%.6f\n", pulsesPerMM);
    f.close();

    Serial.printf("[SD] Backup kalibracji: %.4f imp/mm\n", pulsesPerMM);
}

uint64_t SDLogger::getCardSizeMB() const
{
    if (!_cardReady) return 0;
    return SD.cardSize() / (1024 * 1024);
}

uint64_t SDLogger::getFreeSpaceMB() const
{
    if (!_cardReady) return 0;
    return (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024);
}

void SDLogger::_ensureDirectory(const char* path)
{
    if (!SD.exists(path)) {
        SD.mkdir(path);
    }
}

String SDLogger::_getSessionFilename()
{
    char buf[40];
    snprintf(buf, sizeof(buf), "/km251/sessions/sesja_%04lu.csv",
             (unsigned long)_sessionId);
    return String(buf);
}
