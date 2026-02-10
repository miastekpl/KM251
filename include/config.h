/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Plik konfiguracyjny - definicje pinów i parametrów
 * Wersja: 1.1.1
 * =============================================================
 */

#ifndef KM251_CONFIG_H
#define KM251_CONFIG_H

#include <Arduino.h>

// =============================================================
// WERSJA FIRMWARE
// =============================================================
#define FW_VERSION_MAJOR    1
#define FW_VERSION_MINOR    1
#define FW_VERSION_PATCH    1
#define FW_VERSION_STRING   "1.1.1"
#define FW_BUILD_DATE       __DATE__
#define FW_BUILD_TIME       __TIME__
#define FW_DEVICE_NAME      "KM251"
#define FW_DEVICE_DESC      "Sterownik Malowarki Pasow Drogowych"

// =============================================================
// PINY - WYŚWIETLACZ ILI9341 (SPI)
// Konfiguracja przez flagi TFT_eSPI w platformio.ini
// =============================================================
// TFT_MOSI  = GPIO11
// TFT_MISO  = GPIO13
// TFT_SCLK  = GPIO12
// TFT_CS    = GPIO10
// TFT_DC    = GPIO9
// TFT_RST   = GPIO8
// TFT_BL    = GPIO7
// TOUCH_CS  = GPIO6
#define PIN_TFT_BL          7

// =============================================================
// PIN - KARTA SD (czytnik zintegrowany z wyswietlaczem ILI9341)
// Wspoldzielona magistrala SPI: MOSI=11, MISO=13, SCK=12
// =============================================================
#define PIN_SD_CS           38   // Chip Select karty SD

// =============================================================
// PINY - ENKODER OBROTOWY (pomiar dystansu, predkosci, powierzchni)
// =============================================================
#define PIN_ENC_A            1   // Enkoder kanał A
#define PIN_ENC_B            2   // Enkoder kanał B

// =============================================================
// PINY - PRZYCISKI FUNKCYJNE
// =============================================================
#define PIN_BTN_START        4   // Start / Pauza
#define PIN_BTN_STOP         5   // Stop
#define PIN_BTN_SELECTOR    42   // Selektor (osobny przycisk)

// =============================================================
// PINY - PISTOLETY (6x przekaźnik)
// P1-P4: pistolety osi jezdni
// P5-P6: pistolety krawędzi jezdni
// =============================================================
#define PIN_GUN_1           14   // P1 - oś jezdni, 12cm
#define PIN_GUN_2           15   // P2 - oś jezdni, 12cm
#define PIN_GUN_3           16   // P3 - oś jezdni, 12cm
#define PIN_GUN_4           17   // P4 - oś jezdni, 24cm
#define PIN_GUN_5           18   // P5 - krawędź jezdni, 12cm
#define PIN_GUN_6           21   // P6 - krawędź jezdni, 24cm
#define NUM_GUNS             6

// Szerokości pistoletów (cm)
#define GUN1_WIDTH_CM       12
#define GUN2_WIDTH_CM       12
#define GUN3_WIDTH_CM       12
#define GUN4_WIDTH_CM       24
#define GUN5_WIDTH_CM       12
#define GUN6_WIDTH_CM       24

// =============================================================
// PINY - SYGNALIZACJA
// =============================================================
#define PIN_BUZZER          47   // Buzzer

// =============================================================
// PARAMETRY PRZYCISKÓW
// =============================================================
#define BTN_DEBOUNCE_MS      30
#define BTN_LONG_PRESS_MS    1000

// =============================================================
// PARAMETRY ENKODERA - KALIBRACJA
// =============================================================
#define ENC_CALIBRATION_DIST_MM   10000  // Dystans kalibracyjny: 10 metrów
#define ENC_DEFAULT_PULSES_PER_M  1000   // Domyślna wartość impulsów/metr

// =============================================================
// BEZPIECZEŃSTWO - MINIMALNA PRĘDKOŚĆ MALOWANIA
// =============================================================
#define MIN_PAINT_SPEED_KMH  3.0f  // Pistolety nie otworzą się poniżej 3 km/h

// =============================================================
// PARAMETRY WYŚWIETLACZA
// =============================================================
#define TFT_ROTATION         1      // 1 = landscape 320x240
#define TFT_BL_CHANNEL       0
#define TFT_BL_FREQ          5000
#define TFT_BL_RESOLUTION    8
#define TFT_BL_DEFAULT       200

// =============================================================
// KOLORY GUI (RGB565) - CZARNE TŁO
// =============================================================
#define COLOR_BG             0x0000   // Czarne tło
#define COLOR_BG_HEADER      0x0000
#define COLOR_BG_MENU        0x10A2
#define COLOR_BG_SELECTED    0x03BF
#define COLOR_BG_ACTIVE      0x07E0
#define COLOR_BG_WARNING     0xFBE0
#define COLOR_BG_ERROR       0xF800
#define COLOR_BG_DISABLED    0x2104

#define COLOR_TEXT_PRIMARY   0xFFFF   // Biały
#define COLOR_TEXT_SECONDARY 0xB5B6   // Szary
#define COLOR_TEXT_ACCENT    0x07FF   // Cyjan
#define COLOR_TEXT_SUCCESS   0x07E0   // Zielony
#define COLOR_TEXT_WARNING   0xFFE0   // Żółty
#define COLOR_TEXT_ERROR     0xF800   // Czerwony

#define COLOR_GUN_IDLE       0x2104   // Ciemny - pistolet nieaktywny
#define COLOR_GUN_PATTERN    0xFFE0   // Żółty - w wybranym wzorcu
#define COLOR_GUN_PAINTING   0x07E0   // Zielony - maluje
#define COLOR_GUN_PAUSED     0xFFE0   // Żółty (miganie) - pauza

#define COLOR_BORDER         0x4A49
#define COLOR_PROGRESS_BG    0x2104
#define COLOR_PROGRESS_FG    0x07E0

// =============================================================
// PARAMETRY GUI
// =============================================================
#define HEADER_HEIGHT        0       // Brak nagłówka - pełnoekranowy layout
#define STATUS_BAR_H         0
#define MENU_ITEM_HEIGHT     32
#define MENU_VISIBLE         5
#define GUI_REFRESH_MS       50
#define GUN_BOX_W            48
#define GUN_BOX_H            30
#define GUN_BOX_GAP          5
#define GUN_BOX_Y            200     // Y startowy prostokątów pistoletów

// =============================================================
// WIFI - Access Point
// =============================================================
#define WIFI_AP_SSID         "Trassar"
#define WIFI_AP_PASS         "12345678"
#define WIFI_AP_CHANNEL      6
#define WEB_SERVER_PORT      80

// =============================================================
// WZORCE MALOWANIA
// =============================================================
#define NUM_PATTERNS         15

// =============================================================
// STATYSTYKI
// =============================================================
#define STATS_SAVE_INTERVAL_MS  60000  // Zapis statystyk co 60s

#endif // KM251_CONFIG_H
