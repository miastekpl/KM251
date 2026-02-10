# KM251 - Instrukcja Obsługi

## Sterownik Malowarki Pasów Drogowych
### Firmware v1.0.1

---

## 1. Opis urządzenia

KM251 to sterownik malowarki pasów drogowych oparty na mikrokontrolerze ESP32-S3.
Obsługuje 6 pistoletów malarskich i 15 wzorców malowania zgodnych z polskimi normami
oznakowania drogowego.

## 2. Elementy sterowania

### 2.1 Przyciski fizyczne

| Przycisk | Krótkie naciśnięcie | Długie przytrzymanie (1s) |
|----------|---------------------|--------------------------|
| **Start/Pauza** | Start malowania / Pauza (wstrzymanie pistoletów) | — |
| **Stop** | Zatrzymanie procesu malowania | Wejście w menu główne |
| **Selektor** | Przeskakiwanie po opcjach menu / zmiana wzorca osi | Wejście w podświetlaną funkcję / odwrócenie P-3 |

### 2.2 Enkoder obrotowy

Enkoder służy **wyłącznie do pomiarów** — nie do nawigacji po menu.

- **Pomiar dystansu** — zliczanie impulsów koła (kalibrowany)
- **Pomiar prędkości** — obliczanie km/h na podstawie impulsów
- **Obliczanie powierzchni** — dystans × szerokość aktywnych pistoletów

Nawigacja po menu odbywa się **wyłącznie Selektorem** (krótkie kliknięcie = następna opcja, długie przytrzymanie = wejście w funkcję).

## 3. Wzorce malowania

### 3.1 Wzorce osi jezdni (pistolety P1-P4)

| Kod | Nazwa | Linia | Przerwa | Szer. | Pistolet |
|-----|-------|-------|---------|-------|----------|
| P-1a | Przerywana długa | 4.0m | 8.0m | 12cm | P2 |
| P-1b | Przerywana krótka | 2.0m | 4.0m | 12cm | P2 |
| P-1c | Wydzielająca | 2.0m | 2.0m | 12cm | P2 |
| P-1d | Prowadząca wąska | 1.0m | 1.0m | 12cm | P2 |
| P-1e | Prowadząca szeroka | 1.0m | 1.0m | 24cm | P4 |
| P-2a | Ciągła wąska | ciągła | — | 12cm | P2 |
| P-2b | Ciągła szeroka | ciągła | — | 24cm | P4 |
| P-3a | Przekraczalna długa | 4.0m | 2.0m | 12cm | P1+P3 |
| P-3b | Przekraczalna krótka | 1.0m | 1.0m | 12cm | P1+P3 |
| P-4 | Podwójna ciągła | ciągła | — | 2x12cm | P1+P3 |

### 3.2 Wzorce krawędzi jezdni (pistolety P5-P6)

| Kod | Nazwa | Linia | Przerwa | Szer. | Pistolet |
|-----|-------|-------|---------|-------|----------|
| P-6 | Ostrzegawcza | 4.0m | 2.0m | 12cm | P5 |
| P-7a | Krawędziowa przeryw. szer. | 1.0m | 1.0m | 24cm | P6 |
| P-7b | Krawędziowa ciągła szer. | ciągła | — | 24cm | P6 |
| P-7c | Krawędziowa przeryw. wąska | 1.0m | 1.0m | 12cm | P5 |
| P-7d | Krawędziowa ciągła wąska | ciągła | — | 12cm | P5 |

### 3.3 Wzorce odwracalne (P-3a, P-3b)

Wzorce P-3a i P-3b to linie podwójne: jedna ciągła + jedna przerywana.
- **Normalnie:** P1 = ciągła, P3 = przerywana
- **Odwrócony:** P1 = przerywana, P3 = ciągła
- **Odwracanie:** długie przytrzymanie Selektora

## 4. Pistolety

| Nr | Pin | Kategoria | Szerokość | Zastosowanie |
|----|-----|-----------|-----------|-------------|
| P1 | GPIO14 | Oś jezdni | 12cm | P-3a, P-3b, P-4 (lewa) |
| P2 | GPIO15 | Oś jezdni | 12cm | P-1a...P-1d, P-2a |
| P3 | GPIO16 | Oś jezdni | 12cm | P-3a, P-3b, P-4 (prawa) |
| P4 | GPIO17 | Oś jezdni | 24cm | P-1e, P-2b |
| P5 | GPIO18 | Krawędź | 12cm | P-6, P-7c, P-7d |
| P6 | GPIO21 | Krawędź | 24cm | P-7a, P-7b |

## 5. Kalibracja enkodera

Przed rozpoczęciem pracy malowarka musi mieć skalibrowany enkoder kołowy.

### Procedura:
1. Wejdź w **Menu > Ustawienia > Kalibracja enkodera**
2. Naciśnij **START** — system przejdzie w tryb gotowości
3. Ustaw malowarkę na początku odcinka **dokładnie 10 metrów**
4. Naciśnij **START** — system rozpocznie zliczanie impulsów
5. Przejedź dokładnie **10 metrów**
6. Naciśnij **START** — system obliczy współczynnik
7. Naciśnij **START** ponownie — zapis do pamięci

Kalibracja zapisuje się trwale i przetrwa restart urządzenia.

## 6. Panel WWW

Po włączeniu urządzenie tworzy sieć WiFi:
- **SSID:** KM251-Malowarka
- **Hasło:** km251admin
- **Adres:** http://192.168.4.1

Panel umożliwia:
- Podgląd stanu na żywo (dystans, prędkość, pistolety)
- Wybór wzorców malowania (przyciski dla każdego wzorca)
- Sterowanie: Start, Pauza, Wznów, Stop
- Odwracanie wzorców P-3a/P-3b
- Kalibrację enkodera

## 7. Obsługa — szybki start

1. Włącz urządzenie
2. Wybierz wzorzec osi (Selektor — krótkie kliknięcia)
3. Wybierz wzorzec krawędzi (Stop długie → Menu → Wzorce Krawędź → Selektor)
4. Naciśnij **START** — malowanie rozpoczęte
5. Naciśnij **START** ponownie — pauza (pistolety wyłączone)
6. Naciśnij **STOP** — zakończenie sesji

## 8. Schemat połączeń

```
ESP32-S3 N16R8
┌─────────────────────────┐
│                         │
│  GPIO1  ────── Enkoder A│
│  GPIO2  ────── Enkoder B│
│  GPIO4  ──── Start/Pauza│
│  GPIO5  ──── Stop       │
│  GPIO42 ──── Selektor   │
│                         │
│  GPIO14 ──── P1 (relay) │
│  GPIO15 ──── P2 (relay) │
│  GPIO16 ──── P3 (relay) │
│  GPIO17 ──── P4 (relay) │
│  GPIO18 ──── P5 (relay) │
│  GPIO21 ──── P6 (relay) │
│  GPIO47 ──── Buzzer     │
│                         │
│  GPIO7  ──── TFT BL     │
│  GPIO8  ──── TFT RST    │
│  GPIO9  ──── TFT DC     │
│  GPIO10 ──── TFT CS     │
│  GPIO11 ──── TFT MOSI   │
│  GPIO12 ──── TFT SCK    │
│  GPIO13 ──── TFT MISO   │
│  GPIO6  ──── Touch CS   │
│                         │
│  3V3    ──── VCC TFT    │
│  GND    ──── GND        │
└─────────────────────────┘
```

### Podłączenie przekaźników pistoletów:
- Każdy GPIO → tranzystor NPN (np. BC547) → cewka przekaźnika 5V
- Dioda zabezpieczająca (1N4007) równolegle do cewki przekaźnika
- Styk przekaźnika steruje zaworem elektromagnetycznym pistoletu

### Podłączenie przycisków:
- Jeden pin przycisku → GPIO (INPUT_PULLUP)
- Drugi pin przycisku → GND
- Wewnętrzny pullup ESP32 — nie trzeba dodatkowych rezystorów

### Podłączenie enkodera:
- A → GPIO1, B → GPIO2
- Wewnętrzne pullupy ESP32
- GND → GND enkodera
