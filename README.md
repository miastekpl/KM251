# KM251 - Sterownik Malowarki Pasów Drogowych

Firmware dla ESP32-S3 N16R8 sterujący malowarką pasów drogowych z 6 pistoletami,
wyświetlaczem TFT ILI9341 i panelem WWW.

## Funkcje

- **15 wzorców malowania** wg polskich norm (P-1a...P-7d)
- **6 pistoletów** na przekaźnikach (P1-P4 oś jezdni, P5-P6 krawędź)
- **Zmiana wzorców w locie** podczas malowania
- **Kalibracja enkodera** — przejazd 10m ustala współczynnik
- **Serwer WWW** (WiFi AP) — zdalna obsługa z telefonu/tabletu
- **Wyświetlacz TFT 2.8"** ILI9341 320x240 z pełnym GUI
- **3 przyciski** — Start/Pauza, Stop, Selektor
- **Enkoder obrotowy** — pomiar dystansu, prędkości i powierzchni
- **Zapis ustawień** do NVS (pamięć nieulotna ESP32)

## Sprzęt

| Element | Model |
|---------|-------|
| Kontroler | ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM) |
| Wyświetlacz | ILI9341 2.8" TFT SPI 240x320 |
| Enkoder | Obrotowy z przyciskiem (pomiar dystansu) |
| Przyciski | BS-33B monostabilne x2 (Start/Pauza, Stop) + Selektor |
| Wyjścia | 6x przekaźnik (pistolety P1-P6) + buzzer |

## Pinout ESP32-S3

| Pin | Funkcja |
|-----|---------|
| GPIO1 | Enkoder A |
| GPIO2 | Enkoder B |
| GPIO4 | Przycisk Start/Pauza |
| GPIO5 | Przycisk Stop |
| GPIO42 | Przycisk Selektor |
| GPIO14 | Pistolet P1 (oś 12cm) |
| GPIO15 | Pistolet P2 (oś 12cm) |
| GPIO16 | Pistolet P3 (oś 12cm) |
| GPIO17 | Pistolet P4 (oś 24cm) |
| GPIO18 | Pistolet P5 (kraw. 12cm) |
| GPIO21 | Pistolet P6 (kraw. 24cm) |
| GPIO47 | Buzzer |
| GPIO7-13 | Wyświetlacz TFT (SPI) |
| GPIO6 | Touch CS |

## WiFi

- **SSID:** `KM251-Malowarka`
- **Hasło:** `km251admin`
- **Adres:** `http://192.168.4.1`

## Kompilacja

Wymaga [PlatformIO](https://platformio.org/).

```bash
pio run              # kompilacja
pio run -t upload    # wgranie na ESP32-S3
pio device monitor   # monitor szeregowy
```

## Struktura projektu

```
KM251/
├── include/          # Nagłówki (.h)
│   ├── config.h      # Konfiguracja pinów i parametrów
│   ├── patterns.h    # Wzorce malowania
│   ├── guns.h        # Sterowanie pistoletami
│   ├── encoder.h     # Enkoder z kalibracją
│   ├── buttons.h     # Obsługa przycisków
│   ├── display.h     # Wyświetlacz ILI9341
│   ├── painter.h     # Proces malowania
│   ├── menu.h        # System menu
│   ├── webserver.h   # Serwer WWW
│   └── storage.h     # Zapis NVS
├── src/              # Implementacje (.cpp)
├── docs/             # Dokumentacja
├── platformio.ini    # Konfiguracja PlatformIO
├── CHANGELOG.md      # Historia zmian
└── README.md         # Ten plik
```

## Licencja

Projekt prywatny. Wszelkie prawa zastrzeżone.
