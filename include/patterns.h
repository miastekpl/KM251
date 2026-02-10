/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Moduł wzorców malowania pasów drogowych
 * =============================================================
 */

#ifndef KM251_PATTERNS_H
#define KM251_PATTERNS_H

#include <Arduino.h>
#include "config.h"

// =============================================================
// Typ wzorca
// =============================================================
enum class PatternType : uint8_t {
    DASHED,         // Przerywana (linia + przerwa)
    CONTINUOUS,     // Ciągła
    DUAL_MIXED,     // Podwójna: ciągła + przerywana (P-3a, P-3b)
    DUAL_CONTINUOUS // Podwójna ciągła (P-4)
};

// =============================================================
// Identyfikatory wzorców
// =============================================================
enum class PatternID : uint8_t {
    P1A = 0,    // Przerywana długa          4.0/8.0m  12cm  P2
    P1B,        // Przerywana krótka         2.0/4.0m  12cm  P2
    P1C,        // Wydzielająca              2.0/2.0m  12cm  P2
    P1D,        // Prowadząca wąska          1.0/1.0m  12cm  P2
    P1E,        // Prowadząca szeroka        1.0/1.0m  24cm  P4
    P2A,        // Ciągła wąska              ciągła    12cm  P2
    P2B,        // Ciągła szeroka            ciągła    24cm  P4
    P3A,        // Przekraczalna długa       4.0/2.0m  12cm  P1+P3 (odwracalna)
    P3B,        // Przekraczalna krótka      1.0/1.0m  12cm  P1+P3 (odwracalna)
    P4,         // Podwójna ciągła           ciągła    24cm  P1+P3
    P6,         // Ostrzegawcza              4.0/2.0m  12cm  P5
    P7A,        // Krawędziowa przeryw.szer. 1.0/1.0m  24cm  P6
    P7B,        // Krawędziowa ciągła szer.  ciągła    24cm  P6
    P7C,        // Krawędziowa przeryw.wąska 1.0/1.0m  12cm  P5
    P7D,        // Krawędziowa ciągła wąska  ciągła    12cm  P5
    COUNT
};

// =============================================================
// Definicja wzorca malowania
// =============================================================
struct PatternDef {
    PatternID   id;
    const char* code;           // Kod wzorca (np. "P-1a")
    const char* name;           // Nazwa opisowa
    PatternType type;           // Typ wzorca
    uint16_t    lineLength_mm;  // Długość linii (mm), 0 = ciągła
    uint16_t    gapLength_mm;   // Długość przerwy (mm), 0 = ciągła
    uint8_t     widthCm;        // Szerokość linii (cm)
    uint8_t     guns[NUM_GUNS]; // Maska pistoletów (1=aktywny, 0=nieaktywny)
    bool        reversible;     // Czy wzorzec odwracalny (P-3a, P-3b)
};

// =============================================================
// Klasa zarządzania wzorcami
// =============================================================
class PatternManager {
public:
    PatternManager();
    void begin();

    // Pobranie definicji wzorca
    const PatternDef& getPattern(PatternID id) const;
    const PatternDef& getPatternByIndex(uint8_t index) const;
    uint8_t getPatternCount() const { return static_cast<uint8_t>(PatternID::COUNT); }

    // Aktywny wzorzec osi jezdni
    void setActiveAxisPattern(PatternID id);
    PatternID getActiveAxisPattern() const { return _activeAxisPattern; }
    const PatternDef& getActiveAxisDef() const;

    // Aktywny wzorzec krawędzi
    void setActiveEdgePattern(PatternID id);
    PatternID getActiveEdgePattern() const { return _activeEdgePattern; }
    const PatternDef& getActiveEdgeDef() const;

    // Odwracanie wzorców P-3a / P-3b
    void setReversed(bool reversed);
    bool isReversed() const { return _reversed; }
    void toggleReversed();

    // Sprawdzenie stanu pistoletów dla aktualnej pozycji
    // Zwraca maskę bitową: bit0=P1, bit1=P2, ... bit5=P6
    uint8_t getGunStateForPosition(uint32_t position_mm, PatternID pattern) const;

    // Obliczanie powierzchni malowania
    // Zwraca powierzchnię w m² dla danego dystansu
    float calculatePaintedArea_m2(PatternID pattern, uint32_t distance_mm) const;

private:
    PatternDef _patterns[NUM_PATTERNS];
    PatternID _activeAxisPattern;
    PatternID _activeEdgePattern;
    bool _reversed;

    void _initPatterns();
};

extern PatternManager patternManager;

#endif // KM251_PATTERNS_H
