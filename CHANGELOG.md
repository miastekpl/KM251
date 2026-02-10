# Changelog - KM251 Sterownik Malowarki Pasów Drogowych

Wszystkie istotne zmiany w projekcie są dokumentowane w tym pliku.

## [1.0.1] - 2026-02-10

### Poprawiono
- Enkoder obrotowy służy **wyłącznie do pomiarów** (dystans, prędkość, powierzchnia)
- Usunięto nawigację enkoderem po menu — nawigacja wyłącznie Selektorem
- Usunięto zmianę wzorca krawędzi enkoderem na ekranie głównym
- Wzorzec krawędzi zmienia się teraz przez Menu > Wzorce Krawędź > Selektor
- Zaktualizowano dokumentację i instrukcję obsługi

## [1.0.0] - 2026-02-10

### Dodano
- Pierwsza wersja firmware v1.0.0
- Obsługa 15 wzorców malowania pasów drogowych (P-1a...P-7d)
- Sterowanie 6 pistoletami (P1-P4 oś jezdni, P5-P6 krawędź)
- Wzorce odwracalne P-3a / P-3b (ciągła + przerywana, zamiana stron)
- Wzorzec P-4 podwójna ciągła
- Zmiana wzorców w locie podczas aktywnego malowania
- Enkoder kołowy z kalibracją dystansu (procedura 10m)
- Pomiar prędkości w km/h i dystansu w metrach
- 3 przyciski fizyczne: Start/Pauza, Stop, Selektor
- Wyświetlacz TFT ILI9341 2.8" 320x240 z pełnym GUI
- System menu z nawigacją selektorem
- Ekran główny z podglądem aktywnych wzorców
- Ekran malowania z wizualizacją stanu pistoletów
- Ekran kalibracji enkodera z procedurą krok po kroku
- Diagnostyka pistoletów (test każdego P1-P6)
- Serwer WWW na WiFi AP (http://192.168.4.1)
- Panel WWW z przyciskami wzorców, sterowaniem i statusem na żywo
- Strona kalibracji enkodera w panelu WWW
- Automatyczny zapis ustawień do NVS (pamięć nieulotna)
- Zapamiętywanie ostatnich wzorców osi i krawędzi
- Obliczanie powierzchni malowania w m²
- Statystyki sesji (dystans, powierzchnia, czas pracy pistoletów)
- Sygnalizacja dźwiękowa (buzzer) — start, pauza, stop
- Podświetlanie ekranu PWM z regulacją jasności

### Architektura
- Modularna budowa: 11 niezależnych modułów C++
- Platforma PlatformIO z frameworkiem Arduino
- ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM)
- Biblioteki: TFT_eSPI, ESP32Encoder, ArduinoJson
