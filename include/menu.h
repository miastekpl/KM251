/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * System menu i nawigacji na wyświetlaczu
 * v1.1.0 - Nowy HUD + menu serwisowe
 * =============================================================
 */

#ifndef KM251_MENU_H
#define KM251_MENU_H

#include <Arduino.h>
#include "config.h"

// =============================================================
// Stany aplikacji / ekrany
// =============================================================
enum class Screen : uint8_t {
    SPLASH,
    HOME,               // Ekran główny HUD (prędkość, wzorzec, pistolety)
    PAINTING,           // Malowanie aktywne
    PAUSED,             // Malowanie wstrzymane (miganie)
    SERVICE_MENU,       // Menu serwisowe (4 opcje)
    CALIBRATION,        // Kalibracja enkodera
    DISTANCE_MEASURE,   // Pomiar dystansu
    REPORTS,            // Raporty z pracy
    NOZZLE_CLEANING,    // Czyszczenie dysz
};

// =============================================================
// System menu (singleton)
// =============================================================
class MenuSystem {
public:
    MenuSystem();
    void begin();
    void update();      // Obsługa wejść
    void render();      // Rysowanie ekranu

    Screen getScreen() const { return _screen; }
    void setScreen(Screen scr);

    void forceRedraw() { _needsRedraw = true; }

    // Czy jesteśmy w trybie czyszczenia dysz (wyjątek od limitu prędkości)
    bool isNozzleCleaning() const { return _nozzleCleaningActive; }

private:
    Screen _screen;
    Screen _prevScreen;
    bool _needsRedraw;

    // Menu serwisowe
    int8_t _serviceMenuSelected;

    // Pomiar dystansu
    bool _distMeasureRunning;
    int64_t _distMeasureStartPulses;
    float _distMeasureSaved;

    // Czyszczenie dysz
    bool _nozzleCleaningActive;

    // Timery
    uint32_t _splashStart;
    uint32_t _lastRenderMs;

    // Obsługa ekranów - input
    void _inputSplash();
    void _inputHome();
    void _inputPainting();
    void _inputPaused();
    void _inputServiceMenu();
    void _inputCalibration();
    void _inputDistanceMeasure();
    void _inputReports();
    void _inputNozzleCleaning();

    // Renderowanie ekranów
    void _renderSplash();
    void _renderHUD(bool painting, bool paused);
    void _renderServiceMenu();
    void _renderCalibration();
    void _renderDistanceMeasure();
    void _renderReports();
    void _renderNozzleCleaning();

    // Pomocnicze
    void _drawGunBoxes(uint8_t patternMask, uint8_t activeMask, bool blinking);
    uint8_t _getPatternGunMask();
    void _logSessionStop();
};

extern MenuSystem menuSystem;

#endif // KM251_MENU_H
