/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja wzorców malowania pasów drogowych
 *
 * Wzorce wg polskich norm oznakowania drogowego.
 * Każdy wzorzec definiuje: długość linii, przerwy, szerokość
 * oraz przypisanie pistoletów (P1-P6).
 * =============================================================
 */

#include "patterns.h"

PatternManager patternManager;

PatternManager::PatternManager()
    : _activeAxisPattern(PatternID::P1A)
    , _activeEdgePattern(PatternID::P7D)
    , _reversed(false)
{
    memset(_patterns, 0, sizeof(_patterns));
}

void PatternManager::begin()
{
    _initPatterns();
    Serial.println("[PATTERNS] Zaladowano 15 wzorcow malowania");
}

// =============================================================
// Inicjalizacja wszystkich 15 wzorców
// guns[]: {P1, P2, P3, P4, P5, P6}
// =============================================================
void PatternManager::_initPatterns()
{
    // P-1a: Przerywana długa, 4.0m/8.0m, 12cm, P2
    _patterns[0] = {
        PatternID::P1A, "P-1a", "Przerywana dluga",
        PatternType::DASHED, 4000, 8000, 12,
        {0, 1, 0, 0, 0, 0}, false
    };

    // P-1b: Przerywana krótka, 2.0m/4.0m, 12cm, P2
    _patterns[1] = {
        PatternID::P1B, "P-1b", "Przerywana krotka",
        PatternType::DASHED, 2000, 4000, 12,
        {0, 1, 0, 0, 0, 0}, false
    };

    // P-1c: Wydzielająca, 2.0m/2.0m, 12cm, P2
    _patterns[2] = {
        PatternID::P1C, "P-1c", "Wydzielajaca",
        PatternType::DASHED, 2000, 2000, 12,
        {0, 1, 0, 0, 0, 0}, false
    };

    // P-1d: Prowadząca wąska, 1.0m/1.0m, 12cm, P2
    _patterns[3] = {
        PatternID::P1D, "P-1d", "Prowadzaca waska",
        PatternType::DASHED, 1000, 1000, 12,
        {0, 1, 0, 0, 0, 0}, false
    };

    // P-1e: Prowadząca szeroka, 1.0m/1.0m, 24cm, P4
    _patterns[4] = {
        PatternID::P1E, "P-1e", "Prowadzaca szeroka",
        PatternType::DASHED, 1000, 1000, 24,
        {0, 0, 0, 1, 0, 0}, false
    };

    // P-2a: Ciągła wąska, 12cm, P2
    _patterns[5] = {
        PatternID::P2A, "P-2a", "Ciagla waska",
        PatternType::CONTINUOUS, 0, 0, 12,
        {0, 1, 0, 0, 0, 0}, false
    };

    // P-2b: Ciągła szeroka, 24cm, P4
    _patterns[6] = {
        PatternID::P2B, "P-2b", "Ciagla szeroka",
        PatternType::CONTINUOUS, 0, 0, 24,
        {0, 0, 0, 1, 0, 0}, false
    };

    // P-3a: Przekraczalna długa, 4.0m/2.0m, 12cm, P1+P3 (odwracalna)
    // Normalnie: P1=ciągła, P3=przerywana
    _patterns[7] = {
        PatternID::P3A, "P-3a", "Przekraczalna dluga",
        PatternType::DUAL_MIXED, 4000, 2000, 12,
        {1, 0, 1, 0, 0, 0}, true
    };

    // P-3b: Przekraczalna krótka, 1.0m/1.0m, 12cm, P1+P3 (odwracalna)
    // Normalnie: P1=ciągła, P3=przerywana
    _patterns[8] = {
        PatternID::P3B, "P-3b", "Przekraczalna krotka",
        PatternType::DUAL_MIXED, 1000, 1000, 12,
        {1, 0, 1, 0, 0, 0}, true
    };

    // P-4: Podwójna ciągła, 24cm, P1+P3
    _patterns[9] = {
        PatternID::P4, "P-4", "Podwojna ciagla",
        PatternType::DUAL_CONTINUOUS, 0, 0, 24,
        {1, 0, 1, 0, 0, 0}, false
    };

    // P-6: Ostrzegawcza, 4.0m/2.0m, 12cm, P5
    _patterns[10] = {
        PatternID::P6, "P-6", "Ostrzegawcza",
        PatternType::DASHED, 4000, 2000, 12,
        {0, 0, 0, 0, 1, 0}, false
    };

    // P-7a: Krawędziowa przerywana szeroka, 1.0m/1.0m, 24cm, P6
    _patterns[11] = {
        PatternID::P7A, "P-7a", "Krawedz. przeryw. szer.",
        PatternType::DASHED, 1000, 1000, 24,
        {0, 0, 0, 0, 0, 1}, false
    };

    // P-7b: Krawędziowa ciągła szeroka, 24cm, P6
    _patterns[12] = {
        PatternID::P7B, "P-7b", "Krawedz. ciagla szer.",
        PatternType::CONTINUOUS, 0, 0, 24,
        {0, 0, 0, 0, 0, 1}, false
    };

    // P-7c: Krawędziowa przerywana wąska, 1.0m/1.0m, 12cm, P5
    _patterns[13] = {
        PatternID::P7C, "P-7c", "Krawedz. przeryw. waska",
        PatternType::DASHED, 1000, 1000, 12,
        {0, 0, 0, 0, 1, 0}, false
    };

    // P-7d: Krawędziowa ciągła wąska, 12cm, P5
    _patterns[14] = {
        PatternID::P7D, "P-7d", "Krawedz. ciagla waska",
        PatternType::CONTINUOUS, 0, 0, 12,
        {0, 0, 0, 0, 1, 0}, false
    };
}

// =============================================================
// Dostęp do wzorców
// =============================================================

const PatternDef& PatternManager::getPattern(PatternID id) const
{
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= NUM_PATTERNS) idx = 0;
    return _patterns[idx];
}

const PatternDef& PatternManager::getPatternByIndex(uint8_t index) const
{
    if (index >= NUM_PATTERNS) index = 0;
    return _patterns[index];
}

// =============================================================
// Aktywne wzorce
// =============================================================

void PatternManager::setActiveAxisPattern(PatternID id)
{
    _activeAxisPattern = id;
    Serial.printf("[PATTERNS] Wzorzec osi: %s\n", getPattern(id).code);
}

const PatternDef& PatternManager::getActiveAxisDef() const
{
    return getPattern(_activeAxisPattern);
}

void PatternManager::setActiveEdgePattern(PatternID id)
{
    _activeEdgePattern = id;
    Serial.printf("[PATTERNS] Wzorzec krawedzi: %s\n", getPattern(id).code);
}

const PatternDef& PatternManager::getActiveEdgeDef() const
{
    return getPattern(_activeEdgePattern);
}

// =============================================================
// Odwracanie P-3a / P-3b
// =============================================================

void PatternManager::setReversed(bool reversed)
{
    _reversed = reversed;
    Serial.printf("[PATTERNS] Odwrocenie: %s\n", reversed ? "TAK" : "NIE");
}

void PatternManager::toggleReversed()
{
    setReversed(!_reversed);
}

// =============================================================
// Obliczanie stanu pistoletów dla danej pozycji
// Zwraca maskę bitową: bit0=P1 ... bit5=P6
// =============================================================
uint8_t PatternManager::getGunStateForPosition(uint32_t position_mm, PatternID pattern) const
{
    const PatternDef& pat = getPattern(pattern);
    uint8_t mask = 0;

    switch (pat.type) {
        case PatternType::CONTINUOUS: {
            // Wszystkie przypisane pistolety zawsze włączone
            for (uint8_t i = 0; i < NUM_GUNS; i++) {
                if (pat.guns[i]) {
                    mask |= (1 << i);
                }
            }
            break;
        }

        case PatternType::DASHED: {
            // Oblicz pozycję w cyklu (linia + przerwa)
            uint32_t cycleLen = pat.lineLength_mm + pat.gapLength_mm;
            uint32_t posInCycle = position_mm % cycleLen;

            if (posInCycle < pat.lineLength_mm) {
                // W segmencie linii - pistolety włączone
                for (uint8_t i = 0; i < NUM_GUNS; i++) {
                    if (pat.guns[i]) {
                        mask |= (1 << i);
                    }
                }
            }
            // W przerwie - pistolety wyłączone (mask=0)
            break;
        }

        case PatternType::DUAL_MIXED: {
            // P1 = ciągła (lub przerywana jeśli odwrócony)
            // P3 = przerywana (lub ciągła jeśli odwrócony)
            uint32_t cycleLen = pat.lineLength_mm + pat.gapLength_mm;
            uint32_t posInCycle = position_mm % cycleLen;
            bool inLine = (posInCycle < pat.lineLength_mm);

            if (!_reversed) {
                // Normalnie: P1=ciągła, P3=przerywana
                mask |= (1 << 0);  // P1 zawsze ON
                if (inLine) {
                    mask |= (1 << 2);  // P3 ON w linii
                }
            } else {
                // Odwrócone: P1=przerywana, P3=ciągła
                mask |= (1 << 2);  // P3 zawsze ON
                if (inLine) {
                    mask |= (1 << 0);  // P1 ON w linii
                }
            }
            break;
        }

        case PatternType::DUAL_CONTINUOUS: {
            // Oba pistolety (P1 i P3) zawsze włączone
            mask |= (1 << 0);  // P1
            mask |= (1 << 2);  // P3
            break;
        }
    }

    return mask;
}

// =============================================================
// Obliczanie powierzchni malowania (m²)
// =============================================================
float PatternManager::calculatePaintedArea_m2(PatternID pattern, uint32_t distance_mm) const
{
    const PatternDef& pat = getPattern(pattern);
    float distance_m = distance_mm / 1000.0f;
    float width_m = pat.widthCm / 100.0f;

    // Oblicz efektywną szerokość (sumę szerokości aktywnych pistoletów)
    static const uint8_t gunWidths[NUM_GUNS] = {
        GUN1_WIDTH_CM, GUN2_WIDTH_CM, GUN3_WIDTH_CM,
        GUN4_WIDTH_CM, GUN5_WIDTH_CM, GUN6_WIDTH_CM
    };

    float totalWidth_m = 0;
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        if (pat.guns[i]) {
            totalWidth_m += gunWidths[i] / 100.0f;
        }
    }

    switch (pat.type) {
        case PatternType::CONTINUOUS:
        case PatternType::DUAL_CONTINUOUS:
            // Powierzchnia = dystans * suma szerokości pistoletów
            return distance_m * totalWidth_m;

        case PatternType::DASHED: {
            // Stosunek malowania: lineLength / (lineLength + gapLength)
            float cycleLen = pat.lineLength_mm + pat.gapLength_mm;
            float ratio = (cycleLen > 0) ? (pat.lineLength_mm / cycleLen) : 0;
            return distance_m * totalWidth_m * ratio;
        }

        case PatternType::DUAL_MIXED: {
            // P1 (lub P3) ciągła + P3 (lub P1) przerywana
            float continuousWidth = gunWidths[0] / 100.0f;  // P1 lub P3, oba 12cm
            float dashedWidth = gunWidths[2] / 100.0f;
            float cycleLen = pat.lineLength_mm + pat.gapLength_mm;
            float ratio = (cycleLen > 0) ? (pat.lineLength_mm / cycleLen) : 0;
            return distance_m * (continuousWidth + dashedWidth * ratio);
        }
    }

    return 0;
}
