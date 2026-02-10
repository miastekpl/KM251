/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * System menu i nawigacji na wyświetlaczu
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
    HOME,               // Ekran główny (status malowarki)
    PAINTING,           // Ekran malowania (aktywny proces)
    PAUSED,             // Malowanie wstrzymane
    MENU_MAIN,          // Menu główne
    MENU_PATTERNS_AXIS, // Wybór wzorca osi jezdni
    MENU_PATTERNS_EDGE, // Wybór wzorca krawędzi
    MENU_DIAGNOSTICS,   // Diagnostyka pistoletów
    MENU_SETTINGS,      // Ustawienia
    MENU_INFO,          // Informacje o urządzeniu
    CALIBRATION,        // Ekran kalibracji enkodera
};

// =============================================================
// Element listy menu
// =============================================================
#define MAX_MENU_ITEMS  16

struct MenuItem {
    const char* label;
    const char* value;
    bool hasSubmenu;
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

    // Wymuszenie przerysowania
    void forceRedraw() { _needsRedraw = true; }

private:
    Screen _screen;
    Screen _prevScreen;
    bool _needsRedraw;

    // Menu
    MenuItem _items[MAX_MENU_ITEMS];
    uint8_t _itemCount;
    int8_t _selected;
    int8_t _scrollOff;

    // Timery
    uint32_t _splashStart;
    uint32_t _lastRenderMs;

    // Obsługa ekranów - input
    void _inputSplash();
    void _inputHome();
    void _inputPainting();
    void _inputPaused();
    void _inputMainMenu();
    void _inputPatternsAxis();
    void _inputPatternsEdge();
    void _inputDiagnostics();
    void _inputSettings();
    void _inputInfo();
    void _inputCalibration();

    // Renderowanie ekranów
    void _renderSplash();
    void _renderHome();
    void _renderPainting();
    void _renderPaused();
    void _renderMainMenu();
    void _renderPatternsAxis();
    void _renderPatternsEdge();
    void _renderDiagnostics();
    void _renderSettings();
    void _renderInfo();
    void _renderCalibration();

    // Budowanie menu
    void _buildMainMenu();
    void _buildPatternsAxisMenu();
    void _buildPatternsEdgeMenu();
    void _buildDiagnosticsMenu();
    void _buildSettingsMenu();

    // Nawigacja
    void _navUp();
    void _navDown();
    void _selectItem();
    void _drawMenuList();
};

extern MenuSystem menuSystem;

#endif // KM251_MENU_H
