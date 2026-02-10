/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja wyświetlacza ILI9341
 * =============================================================
 */

#include "display.h"

DisplayManager displayManager;

DisplayManager::DisplayManager()
    : _tft()
    , _brightness(TFT_BL_DEFAULT)
{
}

void DisplayManager::begin()
{
    _tft.init();
    _tft.setRotation(TFT_ROTATION);
    _tft.fillScreen(COLOR_BG);
    _tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    _tft.setTextDatum(TL_DATUM);

    ledcSetup(TFT_BL_CHANNEL, TFT_BL_FREQ, TFT_BL_RESOLUTION);
    ledcAttachPin(PIN_TFT_BL, TFT_BL_CHANNEL);
    setBrightness(_brightness);

    Serial.println("[DISPLAY] Zainicjalizowano ILI9341 320x240");
}

void DisplayManager::setBrightness(uint8_t brightness)
{
    _brightness = brightness;
    ledcWrite(TFT_BL_CHANNEL, _brightness);
}

void DisplayManager::clear()
{
    _tft.fillScreen(COLOR_BG);
}

int16_t DisplayManager::width()
{
    return _tft.width();
}

int16_t DisplayManager::height()
{
    return _tft.height();
}

void DisplayManager::drawHeader(const char* title, uint16_t color)
{
    _tft.fillRect(0, 0, width(), 28, color);
    _tft.setTextColor(COLOR_TEXT_PRIMARY, color);
    _tft.setTextDatum(MC_DATUM);
    _tft.setFreeFont(nullptr);
    _tft.setTextSize(1);
    _tft.drawString(title, width() / 2, 14, 4);
}

void DisplayManager::drawStatusBar(const char* status, StatusIcon icon)
{
    int y = height() - 18;
    _tft.fillRect(0, y, width(), 18, COLOR_BG);
    _tft.drawFastHLine(0, y, width(), COLOR_BORDER);

    if (icon != StatusIcon::NONE) {
        drawIcon(4, y + 2, icon);
    }

    int textX = (icon != StatusIcon::NONE) ? 24 : 4;
    _tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    _tft.setTextDatum(ML_DATUM);
    _tft.drawString(status, textX, y + 9, 1);
}

void DisplayManager::drawProgressBar(int x, int y, int w, int h, float progress,
                                     uint16_t fgColor, uint16_t bgColor)
{
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    _tft.drawRect(x, y, w, h, COLOR_BORDER);
    _tft.fillRect(x + 1, y + 1, w - 2, h - 2, bgColor);

    int fillW = (int)((w - 2) * progress);
    if (fillW > 0) {
        _tft.fillRect(x + 1, y + 1, fillW, h - 2, fgColor);
    }

    char pctStr[8];
    snprintf(pctStr, sizeof(pctStr), "%d%%", (int)(progress * 100));
    _tft.setTextColor(COLOR_TEXT_PRIMARY, (progress > 0.5f) ? fgColor : bgColor);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(pctStr, x + w / 2, y + h / 2, 1);
}

void DisplayManager::drawMenuItem(int y, const char* label, bool selected,
                                  bool hasSubmenu, const char* value)
{
    uint16_t bgColor = selected ? COLOR_BG_SELECTED : COLOR_BG;
    uint16_t txtColor = selected ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY;

    _tft.fillRect(0, y, width(), MENU_ITEM_HEIGHT, bgColor);

    if (selected) {
        _tft.fillRect(0, y, 3, MENU_ITEM_HEIGHT, COLOR_TEXT_ACCENT);
    }

    _tft.setTextColor(txtColor, bgColor);
    _tft.setTextDatum(ML_DATUM);
    _tft.drawString(label, 10, y + MENU_ITEM_HEIGHT / 2, 2);

    if (value != nullptr) {
        _tft.setTextColor(COLOR_TEXT_ACCENT, bgColor);
        _tft.setTextDatum(MR_DATUM);
        _tft.drawString(value, width() - 10, y + MENU_ITEM_HEIGHT / 2, 2);
    } else if (hasSubmenu) {
        _tft.setTextColor(COLOR_TEXT_SECONDARY, bgColor);
        _tft.setTextDatum(MR_DATUM);
        _tft.drawString(">", width() - 6, y + MENU_ITEM_HEIGHT / 2, 2);
    }

    _tft.drawFastHLine(8, y + MENU_ITEM_HEIGHT - 1, width() - 16, COLOR_BORDER);
}

void DisplayManager::drawKeyValue(int x, int y, const char* key, const char* value,
                                  uint16_t valueColor)
{
    _tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    _tft.setTextDatum(ML_DATUM);
    _tft.drawString(key, x, y, 2);

    _tft.setTextColor(valueColor, COLOR_BG);
    _tft.setTextDatum(MR_DATUM);
    _tft.drawString(value, width() - x, y, 2);
}

void DisplayManager::drawCenteredText(const char* text, int y, uint8_t font,
                                      uint16_t color)
{
    _tft.setTextColor(color, COLOR_BG);
    _tft.setTextDatum(TC_DATUM);
    _tft.drawString(text, width() / 2, y, font);
}

void DisplayManager::drawIcon(int x, int y, StatusIcon icon, uint16_t color)
{
    switch (icon) {
        case StatusIcon::PLAY:
            _tft.fillTriangle(x, y, x, y + 14, x + 12, y + 7, COLOR_TEXT_SUCCESS);
            break;
        case StatusIcon::PAUSE:
            _tft.fillRect(x, y, 5, 14, COLOR_TEXT_WARNING);
            _tft.fillRect(x + 8, y, 5, 14, COLOR_TEXT_WARNING);
            break;
        case StatusIcon::STOP:
            _tft.fillRect(x, y, 14, 14, COLOR_TEXT_ERROR);
            break;
        case StatusIcon::WARNING:
            _tft.fillTriangle(x + 7, y, x, y + 14, x + 14, y + 14, COLOR_TEXT_WARNING);
            break;
        case StatusIcon::ERROR_ICON:
            _tft.fillCircle(x + 7, y + 7, 7, COLOR_TEXT_ERROR);
            break;
        case StatusIcon::OK:
            _tft.fillCircle(x + 7, y + 7, 7, COLOR_TEXT_SUCCESS);
            break;
        case StatusIcon::GUN_ON:
            _tft.fillRect(x, y + 2, 10, 10, COLOR_TEXT_SUCCESS);
            _tft.fillRect(x + 10, y + 4, 4, 6, COLOR_TEXT_SUCCESS);
            break;
        case StatusIcon::GUN_OFF:
            _tft.fillRect(x, y + 2, 10, 10, COLOR_TEXT_SECONDARY);
            _tft.fillRect(x + 10, y + 4, 4, 6, COLOR_TEXT_SECONDARY);
            break;
        case StatusIcon::CALIBRATE:
            _tft.drawCircle(x + 7, y + 7, 7, COLOR_TEXT_ACCENT);
            _tft.drawLine(x + 7, y + 2, x + 7, y + 7, COLOR_TEXT_ACCENT);
            break;
        default:
            break;
    }
}

void DisplayManager::drawGunIndicator(int x, int y, uint8_t gunIndex, bool active)
{
    char label[4];
    snprintf(label, sizeof(label), "P%d", gunIndex + 1);

    uint16_t bgCol = active ? COLOR_BG_ACTIVE : COLOR_BG_DISABLED;
    uint16_t txtCol = active ? TFT_BLACK : COLOR_TEXT_SECONDARY;

    _tft.fillRoundRect(x, y, 36, 18, 3, bgCol);
    _tft.drawRoundRect(x, y, 36, 18, 3, COLOR_BORDER);
    _tft.setTextColor(txtCol, bgCol);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(label, x + 18, y + 9, 1);
}

// =============================================================
// Nowy GUI v1.1 - prostokąt pistoletu (48x30)
// =============================================================
void DisplayManager::drawGunBox(int x, int y, uint8_t gunIndex, uint16_t bgColor)
{
    char label[4];
    snprintf(label, sizeof(label), "P%d", gunIndex + 1);

    _tft.fillRoundRect(x, y, GUN_BOX_W, GUN_BOX_H, 4, bgColor);
    _tft.drawRoundRect(x, y, GUN_BOX_W, GUN_BOX_H, 4, COLOR_BORDER);

    uint16_t txtCol = (bgColor == COLOR_GUN_IDLE) ? COLOR_TEXT_SECONDARY : TFT_BLACK;
    _tft.setTextColor(txtCol, bgColor);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(label, x + GUN_BOX_W / 2, y + GUN_BOX_H / 2, 2);
}

void DisplayManager::drawSplashScreen()
{
    clear();

    _tft.setTextColor(COLOR_TEXT_ACCENT);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(FW_DEVICE_NAME, width() / 2, height() / 2 - 40, 7);

    _tft.setTextColor(COLOR_TEXT_SECONDARY);
    _tft.drawString(FW_DEVICE_DESC, width() / 2, height() / 2 + 20, 2);

    char verStr[32];
    snprintf(verStr, sizeof(verStr), "Firmware v%s", FW_VERSION_STRING);
    _tft.setTextColor(COLOR_TEXT_SECONDARY);
    _tft.drawString(verStr, width() / 2, height() / 2 + 50, 1);

    drawProgressBar(40, height() / 2 + 70, width() - 80, 16, 0.0f);
}
