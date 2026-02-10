/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja systemu menu i nawigacji
 * v1.1.0 - Nowy HUD + menu serwisowe
 *
 * Ekran główny (HUD):
 *   - Lewy górny: wybrany wzorzec malowania
 *   - Prawy górny: aktualna prędkość (duża czcionka)
 *   - Poniżej prędkości: powierzchnia malowania
 *   - Dół: 6 prostokątów pistoletów (żółty/zielony/miganie)
 *
 * Menu serwisowe (STOP długi 1s):
 *   1. Kalibracja enkodera
 *   2. Pomiar dystansu
 *   3. Raporty
 *   4. Czyszczenie dysz
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
#include "sdlogger.h"

MenuSystem menuSystem;

// Nazwy opcji menu serwisowego
static const char* SERVICE_MENU_ITEMS[] = {
    "Kalibracja enkodera",
    "Pomiar dystansu",
    "Raporty",
    "Czyszczenie dysz"
};
static const uint8_t SERVICE_MENU_COUNT = 4;

// Pozycje X dla 6 prostokątów pistoletów
// Łączna szerokość: 6*48 + 5*5 = 313, startX = (320-313)/2 = 3
static const int GUN_BOX_X_START = 4;

MenuSystem::MenuSystem()
    : _screen(Screen::SPLASH), _prevScreen(Screen::SPLASH), _needsRedraw(true),
      _serviceMenuSelected(0),
      _distMeasureRunning(false), _distMeasureStartPulses(0), _distMeasureSaved(0),
      _nozzleCleaningActive(false),
      _splashStart(0), _lastRenderMs(0)
{
}

void MenuSystem::begin()
{
    _splashStart = millis();
    _screen = Screen::SPLASH;
    displayManager.drawSplashScreen();
    Serial.println("[MENU] System menu zainicjalizowany (v1.1)");
}

void MenuSystem::setScreen(Screen scr)
{
    _prevScreen = _screen;
    _screen = scr;
    _needsRedraw = true;

    // Resetuj stany specyficzne dla ekranów
    if (scr == Screen::SERVICE_MENU) {
        _serviceMenuSelected = 0;
    }
    if (scr == Screen::DISTANCE_MEASURE) {
        _distMeasureRunning = false;
        _distMeasureStartPulses = wheelEncoder.getRawPulses();
        _distMeasureSaved = 0;
    }
    if (scr == Screen::NOZZLE_CLEANING) {
        _nozzleCleaningActive = false;
        gunController.allOff();
    }

    displayManager.clear();
}

void MenuSystem::update()
{
    switch (_screen) {
        case Screen::SPLASH:            _inputSplash(); break;
        case Screen::HOME:              _inputHome(); break;
        case Screen::PAINTING:          _inputPainting(); break;
        case Screen::PAUSED:            _inputPaused(); break;
        case Screen::SERVICE_MENU:      _inputServiceMenu(); break;
        case Screen::CALIBRATION:       _inputCalibration(); break;
        case Screen::DISTANCE_MEASURE:  _inputDistanceMeasure(); break;
        case Screen::REPORTS:           _inputReports(); break;
        case Screen::NOZZLE_CLEANING:   _inputNozzleCleaning(); break;
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
        case Screen::SPLASH:            _renderSplash(); break;
        case Screen::HOME:              _renderHUD(false, false); break;
        case Screen::PAINTING:          _renderHUD(true, false); break;
        case Screen::PAUSED:            _renderHUD(false, true); break;
        case Screen::SERVICE_MENU:      _renderServiceMenu(); break;
        case Screen::CALIBRATION:       _renderCalibration(); break;
        case Screen::DISTANCE_MEASURE:  _renderDistanceMeasure(); break;
        case Screen::REPORTS:           _renderReports(); break;
        case Screen::NOZZLE_CLEANING:   _renderNozzleCleaning(); break;
    }
}

// ===================== INPUT HANDLERS ========================

void MenuSystem::_inputSplash()
{
    if ((millis() - _splashStart) > 2500) setScreen(Screen::HOME);
}

void MenuSystem::_inputHome()
{
    // START = rozpocznij malowanie
    ButtonEvent startEvt = buttonManager.getEvent(ButtonID::START_PAUSE);
    if (startEvt == ButtonEvent::CLICK) {
        paintProcess.start();
        setScreen(Screen::PAINTING);
        return;
    }

    // STOP długi = menu serwisowe
    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt == ButtonEvent::LONG_PRESS) {
        setScreen(Screen::SERVICE_MENU);
        return;
    }

    // Selector klik = zmień wzorzec osi
    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) {
        uint8_t cur = static_cast<uint8_t>(patternManager.getActiveAxisPattern());
        cur++;
        if (cur > static_cast<uint8_t>(PatternID::P4)) cur = 0;
        patternManager.setActiveAxisPattern(static_cast<PatternID>(cur));
        storageManager.saveLastAxisPattern(cur);
        _needsRedraw = true;
    }
    // Selector długi = odwróć P-3a/P-3b
    if (selEvt == ButtonEvent::LONG_PRESS) {
        patternManager.toggleReversed();
        _needsRedraw = true;
    }
}

void MenuSystem::_inputPainting()
{
    // START = pauza
    ButtonEvent startEvt = buttonManager.getEvent(ButtonID::START_PAUSE);
    if (startEvt == ButtonEvent::CLICK) {
        paintProcess.pause();
        setScreen(Screen::PAUSED);
        return;
    }

    // STOP = zatrzymaj malowanie
    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt == ButtonEvent::CLICK) {
        _logSessionStop();
        paintProcess.stop();
        setScreen(Screen::HOME);
        return;
    }

    // Selector klik = zmień wzorzec osi w locie
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
    // START = wznów
    ButtonEvent startEvt = buttonManager.getEvent(ButtonID::START_PAUSE);
    if (startEvt == ButtonEvent::CLICK) {
        paintProcess.resume();
        setScreen(Screen::PAINTING);
        return;
    }

    // STOP = zakończ
    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt == ButtonEvent::CLICK) {
        _logSessionStop();
        paintProcess.stop();
        setScreen(Screen::HOME);
        return;
    }
}

void MenuSystem::_inputServiceMenu()
{
    // Selector klik = nawigacja w dół (cyklicznie)
    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) {
        _serviceMenuSelected++;
        if (_serviceMenuSelected >= SERVICE_MENU_COUNT) _serviceMenuSelected = 0;
        _needsRedraw = true;
    }

    // Selector długi = wejdź w wybraną opcję
    if (selEvt == ButtonEvent::LONG_PRESS) {
        switch (_serviceMenuSelected) {
            case 0: setScreen(Screen::CALIBRATION); break;
            case 1: setScreen(Screen::DISTANCE_MEASURE); break;
            case 2: setScreen(Screen::REPORTS); break;
            case 3: setScreen(Screen::NOZZLE_CLEANING); break;
        }
        return;
    }

    // STOP = powrót do HOME
    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) {
        setScreen(Screen::HOME);
    }
}

void MenuSystem::_inputCalibration()
{
    CalibrationState cs = wheelEncoder.getCalibrationState();

    ButtonEvent startEvt = buttonManager.getEvent(ButtonID::START_PAUSE);
    if (startEvt == ButtonEvent::CLICK) {
        switch (cs) {
            case CalibrationState::IDLE:
            case CalibrationState::ERROR:
                wheelEncoder.startCalibration();
                _needsRedraw = true;
                break;
            case CalibrationState::WAITING_START:
                wheelEncoder.beginMeasurement();
                _needsRedraw = true;
                break;
            case CalibrationState::MEASURING:
                wheelEncoder.endMeasurement();
                _needsRedraw = true;
                break;
            case CalibrationState::COMPLETE:
                storageManager.saveCalibration(wheelEncoder.getCalibrationFactor());
                if (sdLogger.isReady()) {
                    sdLogger.saveCalibrationBackup(wheelEncoder.getCalibrationFactor());
                }
                setScreen(Screen::SERVICE_MENU);
                return;
        }
    }

    // STOP = anuluj i wróć
    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) {
        wheelEncoder.cancelCalibration();
        setScreen(Screen::SERVICE_MENU);
    }
}

void MenuSystem::_inputDistanceMeasure()
{
    // START = start/pauza pomiaru
    ButtonEvent startEvt = buttonManager.getEvent(ButtonID::START_PAUSE);
    if (startEvt == ButtonEvent::CLICK) {
        if (!_distMeasureRunning) {
            // Rozpocznij lub wznów pomiar
            _distMeasureStartPulses = wheelEncoder.getRawPulses();
            _distMeasureRunning = true;
        } else {
            // Zapauzuj - zapisz aktualny dystans
            float pulsesPerMM = wheelEncoder.getCalibrationFactor();
            if (pulsesPerMM > 0) {
                int64_t delta = wheelEncoder.getRawPulses() - _distMeasureStartPulses;
                if (delta < 0) delta = -delta;
                _distMeasureSaved += (float)delta / pulsesPerMM / 1000.0f;
            }
            _distMeasureRunning = false;
        }
        _needsRedraw = true;
    }

    // STOP = reset i powrót
    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) {
        _distMeasureRunning = false;
        _distMeasureSaved = 0;
        setScreen(Screen::SERVICE_MENU);
    }
}

void MenuSystem::_inputReports()
{
    // STOP lub Selector = powrót
    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (stopEvt != ButtonEvent::NONE || selEvt != ButtonEvent::NONE) {
        setScreen(Screen::SERVICE_MENU);
    }
}

void MenuSystem::_inputNozzleCleaning()
{
    // Selector klik = zmień wzorzec
    ButtonEvent selEvt = buttonManager.getEvent(ButtonID::SELECTOR);
    if (selEvt == ButtonEvent::CLICK) {
        uint8_t cur = static_cast<uint8_t>(patternManager.getActiveAxisPattern());
        cur++;
        if (cur > static_cast<uint8_t>(PatternID::P4)) cur = 0;
        patternManager.setActiveAxisPattern(static_cast<PatternID>(cur));
        storageManager.saveLastAxisPattern(cur);
        _needsRedraw = true;
    }

    // Trzymaj START = otwarcie pistoletów (bez limitu prędkości)
    if (buttonManager.isPressed(ButtonID::START_PAUSE)) {
        if (!_nozzleCleaningActive) {
            _nozzleCleaningActive = true;
            // Aktywuj pistolety wg wybranego wzorca
            uint8_t mask = _getPatternGunMask();
            gunController.setGunMask(mask);
            _needsRedraw = true;
        }
    } else {
        if (_nozzleCleaningActive) {
            _nozzleCleaningActive = false;
            gunController.allOff();
            _needsRedraw = true;
        }
    }

    // STOP = wyjdź
    ButtonEvent stopEvt = buttonManager.getEvent(ButtonID::STOP);
    if (stopEvt != ButtonEvent::NONE) {
        _nozzleCleaningActive = false;
        gunController.allOff();
        setScreen(Screen::SERVICE_MENU);
    }
}

// =================== RENDER FUNCTIONS ========================

void MenuSystem::_renderSplash()
{
    float p = (float)(millis() - _splashStart) / 2500.0f;
    displayManager.drawProgressBar(40, displayManager.height() / 2 + 70,
                                   displayManager.width() - 80, 16, p);
}

// =============================================================
// Główny ekran HUD - używany przez HOME, PAINTING, PAUSED
// Layout 320x240 (landscape):
//   Lewy góra: wzorzec (Font 4 + Font 2)
//   Prawy góra: prędkość (Font 7 - duże cyfry 7-segment)
//   Prawy środek: powierzchnia (Font 4)
//   Środek: status
//   Dół: 6 prostokątów pistoletów
// =============================================================
void MenuSystem::_renderHUD(bool painting, bool paused)
{
    TFT_eSPI& tft = displayManager.tft();
    char buf[40];

    // --- Lewy górny róg: wzorzec ---
    const PatternDef& axisPat = patternManager.getActiveAxisDef();
    tft.setTextColor(COLOR_TEXT_ACCENT, COLOR_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextPadding(120);
    tft.drawString(axisPat.code, 5, 5, 4);
    tft.setTextPadding(140);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    tft.drawString(axisPat.name, 5, 35, 2);

    // Krawędź (mała etykieta)
    const PatternDef& edgePat = patternManager.getActiveEdgeDef();
    snprintf(buf, sizeof(buf), "Kr: %s", edgePat.code);
    tft.setTextPadding(100);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    tft.drawString(buf, 5, 55, 2);

    // Odwrócenie (jeśli dotyczy)
    if (axisPat.reversible) {
        tft.setTextPadding(80);
        tft.setTextColor(patternManager.isReversed() ? COLOR_TEXT_WARNING : COLOR_TEXT_SECONDARY, COLOR_BG);
        tft.drawString(patternManager.isReversed() ? "ODWROCONY" : "", 5, 75, 2);
    } else {
        tft.fillRect(5, 75, 80, 16, COLOR_BG);
    }

    // --- Prawy górny róg: prędkość (duża czcionka) ---
    float speed = wheelEncoder.getSpeedKMH();
    snprintf(buf, sizeof(buf), "%.1f", speed);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    tft.setTextPadding(160);
    tft.drawString(buf, 315, 2, 7);  // Font 7 = 48px 7-segment

    tft.setTextPadding(50);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    tft.drawString("km/h", 315, 52, 2);

    // --- Poniżej prędkości: powierzchnia ---
    float area = paintProcess.getStats().totalArea_m2;
    snprintf(buf, sizeof(buf), "%.2f m2", area);
    tft.setTextPadding(140);
    tft.setTextColor(COLOR_TEXT_SUCCESS, COLOR_BG);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(buf, 315, 75, 4);

    // --- Dystans (gdy malowanie) ---
    if (painting || paused) {
        snprintf(buf, sizeof(buf), "Dyst: %.1f m", paintProcess.getDistance_m());
        tft.setTextPadding(130);
        tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
        tft.setTextDatum(TL_DATUM);
        tft.drawString(buf, 5, 95, 2);
    }

    // --- Status ---
    const char* statusText;
    uint16_t statusColor;
    if (paused) {
        statusText = "PAUZA";
        statusColor = COLOR_TEXT_WARNING;
    } else if (painting) {
        statusText = "MALOWANIE";
        statusColor = COLOR_TEXT_SUCCESS;
    } else {
        statusText = "GOTOWY";
        statusColor = COLOR_TEXT_ACCENT;
    }

    tft.setTextPadding(200);
    displayManager.drawCenteredText(statusText, 118, 4, statusColor);

    // Ostrzeżenie o prędkości
    if (painting && speed < MIN_PAINT_SPEED_KMH) {
        tft.setTextPadding(220);
        displayManager.drawCenteredText("! Za mala predkosc !", 148, 2, COLOR_TEXT_ERROR);
    } else if (painting) {
        tft.fillRect(50, 148, 220, 16, COLOR_BG);
    }

    // --- Kalibracja ---
    if (!wheelEncoder.isCalibrated()) {
        tft.setTextPadding(180);
        displayManager.drawCenteredText("Enkoder nieskalibrowany!", 165, 1, COLOR_TEXT_ERROR);
    }

    // --- Prostokąty pistoletów na dole ---
    uint8_t patMask = _getPatternGunMask();
    uint8_t activeMask = (painting) ? gunController.getGunMask() : 0;
    _drawGunBoxes(patMask, activeMask, paused);

    // --- Pasek podpowiedzi ---
    tft.setTextPadding(0);
    if (painting) {
        displayManager.drawStatusBar("START=Pauza STOP=Stop SEL=wzorzec", StatusIcon::PLAY);
    } else if (paused) {
        displayManager.drawStatusBar("START=Wznow STOP=Zakoncz", StatusIcon::PAUSE);
    } else {
        displayManager.drawStatusBar("START=Maluj STOP(dl)=Serwis SEL=wzorzec");
    }
}

// =============================================================
// Menu serwisowe
// =============================================================
void MenuSystem::_renderServiceMenu()
{
    displayManager.drawHeader("MENU SERWISOWE", COLOR_BG_MENU);

    for (uint8_t i = 0; i < SERVICE_MENU_COUNT; i++) {
        int y = 35 + i * MENU_ITEM_HEIGHT;
        displayManager.drawMenuItem(y, SERVICE_MENU_ITEMS[i],
                                    (i == _serviceMenuSelected), true);
    }

    displayManager.drawStatusBar("SEL=nawiguj SEL(dl)=wejdz STOP=cofnij");
}

// =============================================================
// Kalibracja enkodera
// =============================================================
void MenuSystem::_renderCalibration()
{
    displayManager.drawHeader("KALIBRACJA ENKODERA");
    int y = 40;
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

// =============================================================
// Pomiar dystansu
// =============================================================
void MenuSystem::_renderDistanceMeasure()
{
    displayManager.drawHeader("POMIAR DYSTANSU");
    TFT_eSPI& tft = displayManager.tft();
    char buf[32];

    // Oblicz aktualny dystans
    float dist = _distMeasureSaved;
    if (_distMeasureRunning) {
        float pulsesPerMM = wheelEncoder.getCalibrationFactor();
        if (pulsesPerMM > 0) {
            int64_t delta = wheelEncoder.getRawPulses() - _distMeasureStartPulses;
            if (delta < 0) delta = -delta;
            dist += (float)delta / pulsesPerMM / 1000.0f;
        }
    }

    // Duży dystans na środku
    snprintf(buf, sizeof(buf), "%.2f", dist);
    tft.setTextColor(COLOR_TEXT_PRIMARY, COLOR_BG);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(200);
    tft.drawString(buf, 160, 90, 7);  // Font 7 = 48px

    tft.setTextPadding(50);
    tft.setTextColor(COLOR_TEXT_SECONDARY, COLOR_BG);
    tft.drawString("metrow", 160, 130, 2);

    // Status pomiaru
    tft.setTextPadding(150);
    if (_distMeasureRunning) {
        displayManager.drawCenteredText("POMIAR AKTYWNY", 160, 2, COLOR_TEXT_SUCCESS);
    } else if (_distMeasureSaved > 0) {
        displayManager.drawCenteredText("WSTRZYMANY", 160, 2, COLOR_TEXT_WARNING);
    } else {
        displayManager.drawCenteredText("Nacisnij START", 160, 2, COLOR_TEXT_SECONDARY);
    }

    // Aktualna prędkość
    snprintf(buf, sizeof(buf), "Predkosc: %.1f km/h", wheelEncoder.getSpeedKMH());
    tft.setTextPadding(160);
    displayManager.drawCenteredText(buf, 185, 2, COLOR_TEXT_SECONDARY);

    tft.setTextPadding(0);
    displayManager.drawStatusBar("START=Start/Pauza STOP=Reset+cofnij");
}

// =============================================================
// Raporty z pracy
// =============================================================
void MenuSystem::_renderReports()
{
    displayManager.drawHeader("RAPORTY");
    int y = 40;
    char buf[48];

    // Statystyki z NVS
    uint32_t totalDist = storageManager.loadTotalDistance();
    float totalArea = storageManager.loadTotalArea();

    snprintf(buf, sizeof(buf), "%.1f m", totalDist / 1000.0f);
    displayManager.drawKeyValue(8, y, "Laczny dystans:", buf, COLOR_TEXT_ACCENT);
    y += 22;

    snprintf(buf, sizeof(buf), "%.2f m2", totalArea);
    displayManager.drawKeyValue(8, y, "Lacznie pow.:", buf, COLOR_TEXT_ACCENT);
    y += 22;

    // Kalibracja
    snprintf(buf, sizeof(buf), "%.4f imp/mm", wheelEncoder.getCalibrationFactor());
    displayManager.drawKeyValue(8, y, "Kalibracja:", buf, COLOR_TEXT_ACCENT);
    y += 22;

    // Karta SD
    if (sdLogger.isReady()) {
        snprintf(buf, sizeof(buf), "%llu MB", (unsigned long long)sdLogger.getCardSizeMB());
        displayManager.drawKeyValue(8, y, "Karta SD:", buf, COLOR_TEXT_SUCCESS);
        y += 22;
        snprintf(buf, sizeof(buf), "%llu MB", (unsigned long long)sdLogger.getFreeSpaceMB());
        displayManager.drawKeyValue(8, y, "Wolne:", buf, COLOR_TEXT_SUCCESS);
    } else {
        displayManager.drawKeyValue(8, y, "Karta SD:", "Niedostepna", COLOR_TEXT_ERROR);
    }
    y += 28;

    displayManager.drawCenteredText("Logi sesji na karcie SD", y, 2, COLOR_TEXT_SECONDARY);
    y += 18;
    displayManager.drawCenteredText("Szczegoly: http://192.168.4.1", y, 1, COLOR_TEXT_SECONDARY);

    displayManager.drawStatusBar("STOP=cofnij");
}

// =============================================================
// Czyszczenie dysz
// =============================================================
void MenuSystem::_renderNozzleCleaning()
{
    displayManager.drawHeader("CZYSZCZENIE DYSZ");
    TFT_eSPI& tft = displayManager.tft();
    char buf[40];

    // Aktualny wzorzec
    const PatternDef& pat = patternManager.getActiveAxisDef();
    snprintf(buf, sizeof(buf), "Wzorzec: %s", pat.code);
    tft.setTextPadding(200);
    displayManager.drawCenteredText(buf, 40, 4, COLOR_TEXT_ACCENT);

    tft.setTextPadding(180);
    displayManager.drawCenteredText(pat.name, 70, 2, COLOR_TEXT_SECONDARY);

    // Instrukcje
    if (_nozzleCleaningActive) {
        tft.setTextPadding(220);
        displayManager.drawCenteredText("DYSZE OTWARTE!", 100, 4, COLOR_TEXT_SUCCESS);
        displayManager.drawCenteredText("Zwolnij START aby zamknac", 135, 2, COLOR_TEXT_WARNING);
    } else {
        tft.setTextPadding(220);
        displayManager.drawCenteredText("Trzymaj START", 100, 4, COLOR_TEXT_PRIMARY);
        displayManager.drawCenteredText("aby otworzyc dysze", 135, 2, COLOR_TEXT_SECONDARY);
    }

    // Pistolety
    uint8_t patMask = _getPatternGunMask();
    uint8_t activeMask = _nozzleCleaningActive ? patMask : 0;
    _drawGunBoxes(patMask, activeMask, false);

    tft.setTextPadding(0);
    displayManager.drawStatusBar("Trzymaj START=sprysk SEL=wzorzec STOP=cofnij");
}

// =================== HELPER FUNCTIONS ========================

// Rysuje 6 prostokątów pistoletów
// patternMask - bity oznaczające pistolety w wybranym wzorcu
// activeMask  - bity oznaczające aktualnie strzelające pistolety
// blinking    - true = stan PAUZA (miganie żółtym)
void MenuSystem::_drawGunBoxes(uint8_t patternMask, uint8_t activeMask, bool blinking)
{
    bool blinkOn = (millis() / 500) % 2 == 0;

    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        int x = GUN_BOX_X_START + i * (GUN_BOX_W + GUN_BOX_GAP);
        bool inPattern = (patternMask >> i) & 1;
        bool active = (activeMask >> i) & 1;

        uint16_t color;
        if (active) {
            color = COLOR_GUN_PAINTING;     // Zielony - maluje
        } else if (inPattern && blinking) {
            color = blinkOn ? COLOR_GUN_PAUSED : COLOR_GUN_IDLE;  // Miganie żółty/ciemny
        } else if (inPattern) {
            color = COLOR_GUN_PATTERN;      // Żółty - w wybranym wzorcu
        } else {
            color = COLOR_GUN_IDLE;         // Ciemny - nieaktywny
        }

        displayManager.drawGunBox(x, GUN_BOX_Y, i, color);
    }
}

// Zwraca maskę bitową pistoletów używanych w aktywnych wzorcach (oś + krawędź)
uint8_t MenuSystem::_getPatternGunMask()
{
    const PatternDef& axisDef = patternManager.getActiveAxisDef();
    const PatternDef& edgeDef = patternManager.getActiveEdgeDef();

    uint8_t mask = 0;
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        if (axisDef.guns[i] || edgeDef.guns[i]) {
            mask |= (1 << i);
        }
    }
    return mask;
}

// Logowanie podsumowania sesji na kartę SD
void MenuSystem::_logSessionStop()
{
    if (sdLogger.isReady()) {
        const PaintStats& stats = paintProcess.getStats();
        sdLogger.logSessionSummary(
            paintProcess.getDistance_m(),
            stats.totalArea_m2,
            stats.paintingTime_ms / 1000
        );
    }

    // Zapisz statystyki do NVS
    storageManager.saveTotalDistance(paintProcess.getStats().totalDistance_mm);
    storageManager.saveTotalArea(paintProcess.getStats().totalArea_m2);
}
