/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Program główny - integracja wszystkich modułów
 *
 * Firmware v1.1.0
 * Platforma: ESP32-S3 N16R8
 * =============================================================
 */

#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "buttons.h"
#include "encoder.h"
#include "patterns.h"
#include "guns.h"
#include "painter.h"
#include "menu.h"
#include "webserver.h"
#include "storage.h"
#include "sdlogger.h"

// =============================================================
// SETUP
// =============================================================
void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println("==============================================");
    Serial.printf("  %s v%s\n", FW_DEVICE_NAME, FW_VERSION_STRING);
    Serial.println("  Sterownik Malowarki Pasow Drogowych");
    Serial.printf("  Build: %s %s\n", FW_BUILD_DATE, FW_BUILD_TIME);
    Serial.println("==============================================");

    // Buzzer - sygnał startu
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, HIGH);
    delay(50);
    digitalWrite(PIN_BUZZER, LOW);

    // Inicjalizacja modułów
    Serial.println("[INIT] Inicjalizacja modulow...");

    storageManager.begin();
    displayManager.begin();
    buttonManager.begin();
    wheelEncoder.begin();
    patternManager.begin();
    gunController.begin();
    paintProcess.begin();
    menuSystem.begin();
    sdLogger.begin();
    webServer.begin();

    // Wczytaj ostatnie wzorce z pamięci
    uint8_t lastAxis = storageManager.loadLastAxisPattern();
    uint8_t lastEdge = storageManager.loadLastEdgePattern();
    if (lastAxis < NUM_PATTERNS) {
        patternManager.setActiveAxisPattern(static_cast<PatternID>(lastAxis));
    }
    if (lastEdge >= 10 && lastEdge <= 14) {
        patternManager.setActiveEdgePattern(static_cast<PatternID>(lastEdge));
    }

    // Wczytaj kalibrację
    if (storageManager.hasCalibration()) {
        // Kalibracja zostanie wczytana automatycznie przez wheelEncoder.begin()
        // ale aktualizujemy flagę
        Serial.println("[INIT] Kalibracja enkodera wczytana z pamieci");
    } else {
        Serial.println("[INIT] UWAGA: Enkoder nie jest skalibrowany!");
    }

    // Jasność ekranu
    uint8_t brightness = storageManager.loadBrightness();
    displayManager.setBrightness(brightness);

    // Status karty SD
    if (sdLogger.isReady()) {
        Serial.printf("[INIT] Karta SD: %llu MB\n", (unsigned long long)sdLogger.getCardSizeMB());
    } else {
        Serial.println("[INIT] Karta SD: niedostepna (logi wylaczone)");
    }

    Serial.println("[INIT] Wszystkie moduly zainicjalizowane");
    Serial.printf("[INIT] WiFi AP: %s  IP: %s\n", WIFI_AP_SSID, webServer.getIPAddress().c_str());
    Serial.printf("[INIT] Wolna RAM: %lu KB  PSRAM: %lu KB\n",
                  (unsigned long)(ESP.getFreeHeap() / 1024),
                  (unsigned long)(ESP.getFreePsram() / 1024));
    Serial.println("==============================================");
}

// =============================================================
// LOOP
// =============================================================
void loop()
{
    // 1. Odczyt wejść
    buttonManager.update();
    wheelEncoder.update();

    // 2. Aktualizacja logiki
    paintProcess.update();
    gunController.update();

    // 3. Obsługa menu (wejścia -> stan ekranu)
    menuSystem.update();

    // 4. Renderowanie GUI
    menuSystem.render();

    // 5. Logowanie na kartę SD (podczas malowania)
    if (paintProcess.getState() == PaintState::PAINTING && sdLogger.isReady()) {
        sdLogger.logPaintingData(
            paintProcess.getDistance_m(),
            paintProcess.getSpeed_kmh(),
            paintProcess.getStats().totalArea_m2,
            patternManager.getActiveAxisDef().code,
            patternManager.getActiveEdgeDef().code,
            gunController.getGunMask()
        );
    }

    // 6. Obsługa serwera WWW
    webServer.update();

    // 7. Yield dla watchdoga
    yield();
}
