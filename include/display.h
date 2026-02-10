/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Moduł wyświetlacza ILI9341 240x320
 * =============================================================
 */

#ifndef KM251_DISPLAY_H
#define KM251_DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// =============================================================
// Ikony statusu
// =============================================================
enum class StatusIcon : uint8_t {
    NONE = 0,
    PLAY,
    PAUSE,
    STOP,
    WARNING,
    ERROR_ICON,
    OK,
    GUN_ON,
    GUN_OFF,
    CALIBRATE,
};

// =============================================================
// Klasa wyświetlacza
// =============================================================
class DisplayManager {
public:
    DisplayManager();
    void begin();
    void setBrightness(uint8_t brightness);
    void clear();

    // Rysowanie elementów GUI
    void drawHeader(const char* title, uint16_t color = COLOR_BG_HEADER);
    void drawStatusBar(const char* status, StatusIcon icon = StatusIcon::NONE);
    void drawProgressBar(int x, int y, int w, int h, float progress,
                         uint16_t fgColor = COLOR_PROGRESS_FG,
                         uint16_t bgColor = COLOR_PROGRESS_BG);
    void drawMenuItem(int y, const char* label, bool selected = false,
                      bool hasSubmenu = false, const char* value = nullptr);
    void drawKeyValue(int x, int y, const char* key, const char* value,
                      uint16_t valueColor = COLOR_TEXT_PRIMARY);
    void drawCenteredText(const char* text, int y, uint8_t font = 2,
                          uint16_t color = COLOR_TEXT_PRIMARY);
    void drawIcon(int x, int y, StatusIcon icon, uint16_t color = COLOR_TEXT_PRIMARY);
    void drawGunIndicator(int x, int y, uint8_t gunIndex, bool active);
    void drawSplashScreen();

    // Nowy GUI v1.1 - prostokąty pistoletów
    void drawGunBox(int x, int y, uint8_t gunIndex, uint16_t bgColor);

    // Dostęp do TFT
    TFT_eSPI& tft() { return _tft; }
    int16_t width();
    int16_t height();

private:
    TFT_eSPI _tft;
    uint8_t _brightness;
};

extern DisplayManager displayManager;

#endif // KM251_DISPLAY_H
