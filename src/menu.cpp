/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja systemu menu i nawigacji
 * =============================================================
 */

#include "menu.h"
#include "display.h"
#include "buttons.h"
#include "encoder.h"
#include "patterns.h"
#include "guns.h"
#include "painter.h"
#include "storage.h"

MenuSystem menuSystem;

MenuSystem::MenuSystem()
    : _screen(Screen::SPLASH), _prevScreen(Screen::SPLASH), _needsRedraw(true),
      _itemCount(0), _selected(0), _scrollOff(0), _splashStart(0), _lastRenderMs(0)
{
    memset(_items, 0, sizeof(_items));
}

void MenuSystem::begin()
{
    _splashStart = millis();
    _screen = Screen::SPLASH;
    displayManager.drawSplashScreen();
    Serial.println("[MENU] System menu zainicjalizowany");
}

void MenuSystem::setScreen(Screen scr)
{
    _prevScreen = _screen;
    _screen = scr;
    _selected = 0;
    _scrollOff = 0;
    _needsRedraw = true;

    switch (scr) {
        case Screen::MENU_MAIN:          _buildMainMenu(); break;
        case Screen::MENU_PATTERNS_AXIS: _buildPatternsAxisMenu(); break;
        case Screen::MENU_PATTERNS_EDGE: _buildPatternsEdgeMenu(); break;
        case Screen::MENU_DIAGNOSTICS:   _buildDiagnosticsMenu(); break;
        case Screen::MENU_SETTINGS:      _buildSettingsMenu(); break;
        default: break;
    }
    displayManager.clear();
}

void MenuSystem::update()
{
    switch (_screen) {
        case Screen::SPLASH:              _inputSplash(); break;
        case Screen::HOME:                _inputHome(); break;
        case Screen::PAINTING:            _inputPainting(); break;
        case Screen::PAUSED:              _inputPaused(); break;
        case Screen::MENU_MAIN:           _inputMainMenu(); break;
        case Screen::MENU_PATTERNS_AXIS:  _inputPatternsAxis(); break;
        case Screen::MENU_PATTERNS_EDGE:  _inputPatternsEdge(); break;
        case Screen::MENU_DIAGNOSTICS:    _inputDiagnostics(); break;
        case Screen::MENU_SETTINGS:       _inputSettings(); break;
        case Screen::MENU_INFO:           _inputInfo(); break;
        case Screen::CALIBRATION:         _inputCalibration(); break;
    }
}

void MenuSystem::render()
{
    uint32_t now = millis();
    bool periodic = (now - _lastRenderMs) >= GUI_REFRESH_MS;
    if (!_needsRedraw && !periodic) return;
    _lastRenderMs = now;
    _needsRedraw = false;

    switch (_screen) {
        case Screen::SPLASH:              _renderSplash(); break;
        case Screen::HOME:                _renderHome(); break;
        case Screen::PAINTING:            _renderPainting(); break;
        case Screen::PAUSED:              _renderPaused(); break;
        case Screen::MENU_MAIN:           _renderMainMenu(); break;
        case Screen::MENU_PATTERNS_AXIS:  _renderPatternsAxis(); break;
        case Screen::MENU_PATTERNS_EDGE:  _renderPatternsEdge(); break;
        case Screen::MENU_DIAGNOSTICS:    _renderDiagnostics(); break;
        case Screen::MENU_SETTINGS:       _renderSettings(); break;
        case Screen::MENU_INFO:           _renderInfo(); break;
        case Screen::CALIBRATION:         _renderCalibration(); break;
    }
}

// ===================== INPUT HANDLERS ========================

void MenuSystem::_inputSplash()
{
    if ((millis() - _splashStart) > 2500) setScreen(Screen::HOME);
}

void MenuSystem::_inputHome()
{
    ButtonEvent startEvt = buttonManager.getEvent(ButtonID::START_PAUSE);
    if (startEvt == ButtonEvent::CLICK) {
        paintProcess.start();
        setScreen(Screen::PAINTING);
        return;
    }

    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt == ButtonEvent::LONG_PRESS) {
        setScreen(Screen::MENU_MAIN);
        return;
    }

    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) {
        uint8_t cur = static_cast<uint8_t>(patternManager.getActiveAxisPattern());
        cur++;
        if (cur > static_cast<uint8_t>(PatternID::P4)) cur = 0;
        patternManager.setActiveAxisPattern(static_cast<PatternID>(cur));
        storageManager.saveLastAxisPattern(cur);
        _needsRedraw = true;
    }
    if (selEvt == ButtonEvent::LONG_PRESS) {
        patternManager.toggleReversed();
        _needsRedraw = true;
    }

    int32_t delta = wheelEncoder.getUIDelta();
    if (delta != 0) {
        uint8_t cur = static_cast<uint8_t>(patternManager.getActiveEdgePattern());
        int8_t nv = (int8_t)cur + (delta > 0 ? 1 : -1);
        if (nv < 10) nv = 14;
        if (nv > 14) nv = 10;
        patternManager.setActiveEdgePattern(static_cast<PatternID>(nv));
        storageManager.saveLastEdgePattern(nv);
        _needsRedraw = true;
    }
}

void MenuSystem::_inputPainting()
{
    ButtonEvent startEvt = buttonManager.getEvent(ButtonID::START_PAUSE);
    if (startEvt == ButtonEvent::CLICK) { paintProcess.pause(); setScreen(Screen::PAUSED); return; }

    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt == ButtonEvent::CLICK) { paintProcess.stop(); setScreen(Screen::HOME); return; }

    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) {
        uint8_t cur = static_cast<uint8_t>(patternManager.getActiveAxisPattern());
        cur++;
        if (cur > static_cast<uint8_t>(PatternID::P4)) cur = 0;
        paintProcess.setAxisPattern(static_cast<PatternID>(cur));
        _needsRedraw = true;
    }
    if (selEvt == ButtonEvent::LONG_PRESS) {
        paintProcess.toggleReverse();
        _needsRedraw = true;
    }
}

void MenuSystem::_inputPaused()
{
    ButtonEvent startEvt = buttonManager.getEvent(ButtonID::START_PAUSE);
    if (startEvt == ButtonEvent::CLICK) { paintProcess.resume(); setScreen(Screen::PAINTING); return; }

    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt == ButtonEvent::CLICK) { paintProcess.stop(); setScreen(Screen::HOME); return; }
}

void MenuSystem::_inputMainMenu()
{
    int32_t delta = wheelEncoder.getUIDelta();
    if (delta > 0) _navDown(); else if (delta < 0) _navUp();

    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) _navDown();
    if (selEvt == ButtonEvent::LONG_PRESS) { _selectItem(); return; }

    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) setScreen(Screen::HOME);
}

void MenuSystem::_inputPatternsAxis()
{
    int32_t delta = wheelEncoder.getUIDelta();
    if (delta > 0) _navDown(); else if (delta < 0) _navUp();

    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) _navDown();
    if (selEvt == ButtonEvent::LONG_PRESS) {
        if (_selected <= static_cast<int8_t>(PatternID::P4)) {
            patternManager.setActiveAxisPattern(static_cast<PatternID>(_selected));
            storageManager.saveLastAxisPattern(_selected);
            setScreen(Screen::HOME);
        }
        return;
    }

    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) setScreen(Screen::MENU_MAIN);
}

void MenuSystem::_inputPatternsEdge()
{
    int32_t delta = wheelEncoder.getUIDelta();
    if (delta > 0) _navDown(); else if (delta < 0) _navUp();

    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) _navDown();
    if (selEvt == ButtonEvent::LONG_PRESS) {
        uint8_t patIdx = 10 + _selected;
        if (patIdx <= 14) {
            patternManager.setActiveEdgePattern(static_cast<PatternID>(patIdx));
            storageManager.saveLastEdgePattern(patIdx);
            setScreen(Screen::HOME);
        }
        return;
    }

    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) setScreen(Screen::MENU_MAIN);
}

void MenuSystem::_inputDiagnostics()
{
    int32_t delta = wheelEncoder.getUIDelta();
    if (delta > 0) _navDown(); else if (delta < 0) _navUp();

    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) _navDown();
    if (selEvt == ButtonEvent::LONG_PRESS && _selected < 6) {
        gunController.testGun(static_cast<GunID>(_selected), 500);
        return;
    }

    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) setScreen(Screen::MENU_MAIN);
}

void MenuSystem::_inputSettings()
{
    int32_t delta = wheelEncoder.getUIDelta();
    if (delta > 0) _navDown(); else if (delta < 0) _navUp();

    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) _navDown();
    if (selEvt == ButtonEvent::LONG_PRESS) {
        switch (_selected) {
            case 0: setScreen(Screen::CALIBRATION); break;
            case 1: paintProcess.resetStats(); break;
            case 2: storageManager.resetAll(); break;
        }
        return;
    }

    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) setScreen(Screen::MENU_MAIN);
}

void MenuSystem::_inputInfo()
{
    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (stopEvt != ButtonEvent::NONE || selEvt != ButtonEvent::NONE) setScreen(Screen::MENU_MAIN);
}

void MenuSystem::_inputCalibration()
{
    CalibrationState cs = wheelEncoder.getCalibrationState();

    ButtonEvent startEvt = buttonManager.getEvent(ButtonID::START_PAUSE);
    if (startEvt == ButtonEvent::CLICK) {
        switch (cs) {
            case CalibrationState::IDLE:
            case CalibrationState::ERROR:
                wheelEncoder.startCalibration(); _needsRedraw = true; break;
            case CalibrationState::WAITING_START:
                wheelEncoder.beginMeasurement(); _needsRedraw = true; break;
            case CalibrationState::MEASURING:
                wheelEncoder.endMeasurement(); _needsRedraw = true; break;
            case CalibrationState::COMPLETE:
                storageManager.saveCalibration(wheelEncoder.getCalibrationFactor());
                setScreen(Screen::MENU_SETTINGS); return;
        }
    }

    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) { wheelEncoder.cancelCalibration(); setScreen(Screen::MENU_SETTINGS); }
}

// =================== RENDER FUNCTIONS ========================

void MenuSystem::_renderSplash()
{
    float p = (float)(millis() - _splashStart) / 2500.0f;
    displayManager.drawProgressBar(40, displayManager.height()/2+70, displayManager.width()-80, 16, p);
}

void MenuSystem::_renderHome()
{
    displayManager.drawHeader("KM251 Malowarka");
    int y = HEADER_HEIGHT + 6;
    char buf[48];

    displayManager.drawKeyValue(8, y, "Stan:", paintProcess.getStateString(),
        paintProcess.getState() == PaintState::IDLE ? COLOR_TEXT_SUCCESS : COLOR_TEXT_WARNING);
    y += 20;

    const PatternDef& ap = patternManager.getActiveAxisDef();
    snprintf(buf, sizeof(buf), "%s %s", ap.code, ap.name);
    displayManager.drawKeyValue(8, y, "Os:", buf, COLOR_TEXT_ACCENT); y += 20;

    const PatternDef& ep = patternManager.getActiveEdgeDef();
    snprintf(buf, sizeof(buf), "%s %s", ep.code, ep.name);
    displayManager.drawKeyValue(8, y, "Krawedz:", buf, COLOR_TEXT_ACCENT); y += 20;

    if (ap.reversible) {
        displayManager.drawKeyValue(8, y, "Kierunek:",
            patternManager.isReversed() ? "ODWROCONY" : "Normalny",
            patternManager.isReversed() ? COLOR_TEXT_WARNING : COLOR_TEXT_PRIMARY);
        y += 20;
    }

    displayManager.drawKeyValue(8, y, "Kalibracja:",
        wheelEncoder.isCalibrated() ? "TAK" : "NIE",
        wheelEncoder.isCalibrated() ? COLOR_TEXT_SUCCESS : COLOR_TEXT_ERROR);
    y += 20;

    snprintf(buf, sizeof(buf), "%.1f km/h", wheelEncoder.getSpeedKMH());
    displayManager.drawKeyValue(8, y, "Predkosc:", buf);

    displayManager.drawStatusBar("START=Maluj STOP(dl)=Menu SEL=wzorzec");
}

void MenuSystem::_renderPainting()
{
    displayManager.drawHeader("MALOWANIE", COLOR_BG_ACTIVE);
    int y = HEADER_HEIGHT + 6;
    char buf[48];

    snprintf(buf, sizeof(buf), "%.1f m", paintProcess.getDistance_m());
    displayManager.drawKeyValue(8, y, "Dystans:", buf, COLOR_TEXT_ACCENT); y += 20;

    snprintf(buf, sizeof(buf), "%.1f km/h", paintProcess.getSpeed_kmh());
    displayManager.drawKeyValue(8, y, "Predkosc:", buf); y += 20;

    const PatternDef& ap = patternManager.getActiveAxisDef();
    displayManager.drawKeyValue(8, y, "Wzorzec os:", ap.code, COLOR_TEXT_ACCENT); y += 20;

    const PatternDef& ep = patternManager.getActiveEdgeDef();
    displayManager.drawKeyValue(8, y, "Wzorzec kr:", ep.code, COLOR_TEXT_ACCENT); y += 20;

    snprintf(buf, sizeof(buf), "%.2f m2", paintProcess.getStats().totalArea_m2);
    displayManager.drawKeyValue(8, y, "Powierzchnia:", buf); y += 24;

    int gx = 8;
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        displayManager.drawGunIndicator(gx, y, i, gunController.isGunActive(static_cast<GunID>(i)));
        gx += 42;
    }

    displayManager.drawStatusBar("START=Pauza STOP=Stop SEL=wzorzec", StatusIcon::PLAY);
}

void MenuSystem::_renderPaused()
{
    displayManager.drawHeader("WSTRZYMANO", COLOR_BG_WARNING);
    int y = HEADER_HEIGHT + 20;
    char buf[32];

    snprintf(buf, sizeof(buf), "Dystans: %.1f m", paintProcess.getDistance_m());
    displayManager.drawCenteredText(buf, y, 2); y += 30;
    displayManager.drawCenteredText("Pistolety WYLACZONE", y, 2, COLOR_TEXT_WARNING); y += 30;
    displayManager.drawCenteredText("START = Wznow", y, 2, COLOR_TEXT_SUCCESS); y += 22;
    displayManager.drawCenteredText("STOP = Zakoncz", y, 2, COLOR_TEXT_ERROR);
    displayManager.drawStatusBar("PAUZA", StatusIcon::PAUSE);
}

void MenuSystem::_renderMainMenu()
{
    displayManager.drawHeader("Menu Glowne");
    _drawMenuList();
    displayManager.drawStatusBar("Sel:nawiguj Przytrzym:wybierz");
}

void MenuSystem::_renderPatternsAxis()
{
    displayManager.drawHeader("Wzorce - Os Jezdni");
    _drawMenuList();
    char buf[40];
    snprintf(buf, sizeof(buf), "Aktywny: %s", patternManager.getActiveAxisDef().code);
    displayManager.drawStatusBar(buf);
}

void MenuSystem::_renderPatternsEdge()
{
    displayManager.drawHeader("Wzorce - Krawedz");
    _drawMenuList();
    char buf[40];
    snprintf(buf, sizeof(buf), "Aktywny: %s", patternManager.getActiveEdgeDef().code);
    displayManager.drawStatusBar(buf);
}

void MenuSystem::_renderDiagnostics()
{
    displayManager.drawHeader("Diagnostyka Pistoletow");
    _drawMenuList();
    displayManager.drawStatusBar("Przytrzym=test 0.5s STOP=cofnij");
}

void MenuSystem::_renderSettings()
{
    displayManager.drawHeader("Ustawienia");
    _drawMenuList();
    displayManager.drawStatusBar("Przytrzym=wybierz STOP=cofnij");
}

void MenuSystem::_renderInfo()
{
    displayManager.drawHeader("Informacje");
    int y = HEADER_HEIGHT + 8;
    char buf[40];

    displayManager.drawKeyValue(8, y, "Urzadzenie:", FW_DEVICE_NAME); y += 20;
    snprintf(buf, sizeof(buf), "v%s", FW_VERSION_STRING);
    displayManager.drawKeyValue(8, y, "Firmware:", buf); y += 20;
    displayManager.drawKeyValue(8, y, "Data:", FW_BUILD_DATE); y += 20;
    displayManager.drawKeyValue(8, y, "MCU:", "ESP32-S3 N16R8"); y += 20;
    snprintf(buf, sizeof(buf), "%lu KB", (unsigned long)(ESP.getFreeHeap() / 1024));
    displayManager.drawKeyValue(8, y, "Wolna RAM:", buf); y += 20;
    snprintf(buf, sizeof(buf), "%.4f imp/mm", wheelEncoder.getCalibrationFactor());
    displayManager.drawKeyValue(8, y, "Kalibracja:", buf);
    displayManager.drawStatusBar("STOP=cofnij");
}

void MenuSystem::_renderCalibration()
{
    displayManager.drawHeader("Kalibracja Enkodera");
    int y = HEADER_HEIGHT + 12;
    CalibrationState cs = wheelEncoder.getCalibrationState();
    char buf[40];

    switch (cs) {
        case CalibrationState::IDLE:
            displayManager.drawCenteredText("Nacisnij START", y, 2, COLOR_TEXT_PRIMARY);
            y += 20;
            displayManager.drawCenteredText("aby rozpoczac kalibracje", y, 2, COLOR_TEXT_SECONDARY);
            break;
        case CalibrationState::WAITING_START:
            displayManager.drawCenteredText("Ustaw na poczatku 10m", y, 2, COLOR_TEXT_WARNING);
            y += 25;
            displayManager.drawCenteredText("Nacisnij START i jedz!", y, 2, COLOR_TEXT_SUCCESS);
            break;
        case CalibrationState::MEASURING:
            displayManager.drawCenteredText("POMIAR - jedz 10m", y, 2, COLOR_TEXT_SUCCESS);
            y += 25;
            snprintf(buf, sizeof(buf), "Impulsy: %lld", (long long)wheelEncoder.getRawPulses());
            displayManager.drawCenteredText(buf, y, 2, COLOR_TEXT_ACCENT);
            y += 25;
            displayManager.drawCenteredText("START po 10 metrach!", y, 2, COLOR_TEXT_WARNING);
            break;
        case CalibrationState::COMPLETE:
            displayManager.drawCenteredText("KALIBRACJA OK!", y, 2, COLOR_TEXT_SUCCESS);
            y += 25;
            snprintf(buf, sizeof(buf), "%.4f imp/mm", wheelEncoder.getCalibrationFactor());
            displayManager.drawCenteredText(buf, y, 2, COLOR_TEXT_ACCENT);
            y += 25;
            displayManager.drawCenteredText("START = Zapisz", y, 2, COLOR_TEXT_SUCCESS);
            break;
        case CalibrationState::ERROR:
            displayManager.drawCenteredText("BLAD! Za malo impulsow", y, 2, COLOR_TEXT_ERROR);
            y += 25;
            displayManager.drawCenteredText("START = Ponow probe", y, 2, COLOR_TEXT_WARNING);
            break;
    }
    displayManager.drawStatusBar("START=akcja STOP=anuluj", StatusIcon::CALIBRATE);
}

// ================ BUDOWANIE MENU ==============================

void MenuSystem::_buildMainMenu()
{
    _itemCount = 0;
    _items[_itemCount++] = {"Wzorce - Os jezdni", nullptr, true};
    _items[_itemCount++] = {"Wzorce - Krawedz", nullptr, true};
    _items[_itemCount++] = {"Diagnostyka pistoletow", nullptr, true};
    _items[_itemCount++] = {"Ustawienia", nullptr, true};
    _items[_itemCount++] = {"Informacje", nullptr, false};
}

void MenuSystem::_buildPatternsAxisMenu()
{
    _itemCount = 0;
    for (uint8_t i = 0; i <= static_cast<uint8_t>(PatternID::P4); i++) {
        const PatternDef& p = patternManager.getPatternByIndex(i);
        bool active = (patternManager.getActiveAxisPattern() == p.id);
        _items[_itemCount++] = {p.code, active ? "[*]" : nullptr, false};
        if (_itemCount >= MAX_MENU_ITEMS) break;
    }
}

void MenuSystem::_buildPatternsEdgeMenu()
{
    _itemCount = 0;
    for (uint8_t i = 10; i <= 14; i++) {
        const PatternDef& p = patternManager.getPatternByIndex(i);
        bool active = (patternManager.getActiveEdgePattern() == p.id);
        _items[_itemCount++] = {p.code, active ? "[*]" : nullptr, false};
    }
}

void MenuSystem::_buildDiagnosticsMenu()
{
    _itemCount = 0;
    _items[_itemCount++] = {"Test P1 (os 12cm)", nullptr, false};
    _items[_itemCount++] = {"Test P2 (os 12cm)", nullptr, false};
    _items[_itemCount++] = {"Test P3 (os 12cm)", nullptr, false};
    _items[_itemCount++] = {"Test P4 (os 24cm)", nullptr, false};
    _items[_itemCount++] = {"Test P5 (kraw 12cm)", nullptr, false};
    _items[_itemCount++] = {"Test P6 (kraw 24cm)", nullptr, false};
}

void MenuSystem::_buildSettingsMenu()
{
    _itemCount = 0;
    static char calStr[24];
    snprintf(calStr, sizeof(calStr), "%s", wheelEncoder.isCalibrated() ? "OK" : "Wymagana!");
    _items[_itemCount++] = {"Kalibracja enkodera", calStr, false};
    _items[_itemCount++] = {"Reset statystyk", nullptr, false};
    _items[_itemCount++] = {"Reset ustawien", nullptr, false};
}

void MenuSystem::_navUp()
{
    if (_selected > 0) { _selected--; if (_selected < _scrollOff) _scrollOff = _selected; _needsRedraw = true; }
}

void MenuSystem::_navDown()
{
    if (_selected < _itemCount - 1) { _selected++; if (_selected >= _scrollOff + MENU_VISIBLE) _scrollOff = _selected - MENU_VISIBLE + 1; _needsRedraw = true; }
}

void MenuSystem::_selectItem()
{
    if (_selected >= _itemCount) return;
    if (_screen == Screen::MENU_MAIN) {
        switch (_selected) {
            case 0: setScreen(Screen::MENU_PATTERNS_AXIS); break;
            case 1: setScreen(Screen::MENU_PATTERNS_EDGE); break;
            case 2: setScreen(Screen::MENU_DIAGNOSTICS); break;
            case 3: setScreen(Screen::MENU_SETTINGS); break;
            case 4: setScreen(Screen::MENU_INFO); break;
        }
    }
}

void MenuSystem::_drawMenuList()
{
    int startY = HEADER_HEIGHT + 1;
    for (int i = 0; i < MENU_VISIBLE && (i + _scrollOff) < _itemCount; i++) {
        int idx = i + _scrollOff;
        displayManager.drawMenuItem(startY + i * MENU_ITEM_HEIGHT,
            _items[idx].label, (idx == _selected), _items[idx].hasSubmenu, _items[idx].value);
    }
    if (_itemCount > MENU_VISIBLE) {
        int sbH = MENU_VISIBLE * MENU_ITEM_HEIGHT;
        int tH = max(10, sbH * MENU_VISIBLE / _itemCount);
        int tY = startY + (sbH - tH) * _scrollOff / max(1, _itemCount - MENU_VISIBLE);
        displayManager.tft().fillRect(displayManager.width()-3, startY, 3, sbH, COLOR_BG);
        displayManager.tft().fillRect(displayManager.width()-3, tY, 3, tH, COLOR_TEXT_ACCENT);
    }
}
