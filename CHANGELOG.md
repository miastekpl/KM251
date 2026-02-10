# Changelog - KM251 Sterownik Malowarki Pasów Drogowych

Wszystkie istotne zmiany w projekcie są dokumentowane w tym pliku.

## [1.1.1] - 2026-02-10

### Poprawiono - KRYTYCZNY: Guru Meditation Error przy starcie
- **Problem**: Crash `StoreProhibited` w `TFT_eSPI::init()` (EXCVADDR: 0x00000010)
- **Przyczyna**: `board_build.arduino.memory_type = qio_opi` (Octal PSRAM) rezerwuje GPIO11-14 dla dodatkowych linii danych SPI PSRAM. Piny TFT (MOSI=GPIO11, SCLK=GPIO12, MISO=GPIO13) kolidowały z kontrolerem PSRAM.
- **Rozwiązanie 1**: Zmiana trybu PSRAM z `qio_opi` na `qio_qspi` (Quad SPI) — zwalnia GPIO11-14
- **Rozwiązanie 2**: Jawna inicjalizacja `SPI.begin(12, 13, 11, -1)` przed `_tft.init()` w display.cpp
- **Kompromis**: Przepustowość PSRAM zmniejszona o połowę (OPI→QSPI), bez wpływu na działanie aplikacji

## [1.1.0] - 2026-02-10

### Dodano - Nowy interfejs HUD
- Czarne tło, duże czytelne napisy
- Prawy górny róg: aktualna prędkość (czcionka 7-segment 48px)
- Poniżej prędkości: powierzchnia malowania (m²)
- Lewy górny róg: wybrany wzorzec malowania + krawędź
- 6 prostokątów wizualizujących pistoletów na dole ekranu:
  - Żółty = pistolet użyty we wzorcu
  - Zielony = pistolet aktywnie maluje
  - Żółte miganie = pauza (pistolety wstrzymane)
  - Ciemny = pistolet nieaktywny

### Dodano - Menu serwisowe (STOP długi 1s)
- **Kalibracja enkodera**: START → jedź 10m → START → system oblicza impulsy/mm
- **Pomiar dystansu**: START = start pomiaru, START = pauza, STOP = reset
- **Raporty**: podsumowanie łącznego dystansu, powierzchni, stan karty SD
- **Czyszczenie dysz**: wybierz wzorzec → trzymaj START → dysze otwarte tak długo jak trzymany przycisk

### Dodano - Zabezpieczenie prędkości
- Minimalna prędkość malowania: 3 km/h
- Poniżej 3 km/h pistolety automatycznie się zamykają
- Ostrzeżenie na wyświetlaczu i panelu WWW
- Wyjątek: tryb czyszczenia dysz (pistoletami można sterować przy dowolnej prędkości)

### Dodano - Panel WWW dla smartfona
- WiFi AP: SSID="Trassar", hasło="12345678"
- Responsywny interfejs zoptymalizowany pod ekran telefonu
- Duży wyświetlacz prędkości na górze
- Wizualizacja pistoletów z kolorami (zielony/żółty/ciemny)
- Przyciski sterowania: START, PAUZA, STOP, WZNÓW, ODWRÓĆ P-3
- Wybór wzorców osi jezdni i krawędzi
- Kalibracja enkodera przez przeglądarkę
- Status SD i kalibracji w API JSON

### Zmieniono
- Kompletnie przepisany system menu (menu.h/cpp) — nowa architektura HUD
- Usunięto stary system menu z wieloma podmenu
- Nawigacja uproszczona: Selector = zmień wzorzec, STOP(dl) = serwis
- Logowanie sesji na kartę SD przy zatrzymaniu malowania
- Zapis statystyk do NVS przy zakończeniu sesji
- Panel WWW: dodano maskę wzorca pistoletów w JSON API

## [1.0.2] - 2026-02-10

### Zmieniono
- Serwer WWW przepisany z Arduino WebServer na **ESP-IDF natywny** (`esp_http_server.h`)
- Eliminacja problemu z wykrywaniem biblioteki WebServer przez PlatformIO LDF
- Serwer HTTP działa na osobnym tasku FreeRTOS (bardziej stabilny)
- Handlery HTTP jako wolne funkcje C (brak problemów z lambdami)
- 15 zarejestrowanych endpointów REST API
- Panel WWW bez zmian — pełna kompatybilność wsteczna

## [1.0.1] - 2026-02-10

### Dodano
- Logowanie sesji malowania na kartę SD (CSV) — czytnik zintegrowany z wyświetlaczem
- Backup kalibracji na kartę SD
- Podsumowania sesji z dystansem, powierzchnią i czasem

### Poprawiono
- Enkoder obrotowy służy **wyłącznie do pomiarów** (dystans, prędkość, powierzchnia)
- Usunięto nawigację enkoderem po menu — nawigacja wyłącznie Selektorem
- Usunięto zmianę wzorca krawędzi enkoderem na ekranie głównym
- Wzorzec krawędzi zmienia się teraz przez Menu > Wzorce Krawędź > Selektor
- Naprawiono błędy kompilacji: puType::UP, ledcSetup/ledcAttachPin, const width/height
- Naprawiono brakujący #include patterns.h w storage.cpp
- Naprawiono WebServer (lib_ldf_mode=deep+) i lambda captures w webserver.cpp
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
- Modularna budowa: 12 niezależnych modułów C++
- Platforma PlatformIO z frameworkiem Arduino
- ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM)
- Biblioteki: TFT_eSPI, ESP32Encoder, ArduinoJson
