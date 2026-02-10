/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Moduł sterowania pistoletami (6x przekaźnik)
 * =============================================================
 */

#ifndef KM251_GUNS_H
#define KM251_GUNS_H

#include <Arduino.h>
#include "config.h"

// =============================================================
// Identyfikatory pistoletów
// =============================================================
enum class GunID : uint8_t {
    P1 = 0,    // Oś jezdni, 12cm
    P2,        // Oś jezdni, 12cm
    P3,        // Oś jezdni, 12cm
    P4,        // Oś jezdni, 24cm
    P5,        // Krawędź jezdni, 12cm
    P6,        // Krawędź jezdni, 24cm
    COUNT
};

// =============================================================
// Kategoria pistoletu
// =============================================================
enum class GunCategory : uint8_t {
    AXIS,       // Oś jezdni (P1-P4)
    EDGE        // Krawędź jezdni (P5-P6)
};

// =============================================================
// Stan pistoletu
// =============================================================
struct GunState {
    uint8_t     pin;
    uint8_t     widthCm;
    GunCategory category;
    bool        active;         // Czy aktualnie strzela
    bool        enabled;        // Czy włączony (np. wyłączenie ręczne)
    uint32_t    totalOnTime_ms; // Łączny czas pracy (statystyki)
    uint32_t    lastOnTime;     // Czas ostatniego włączenia
};

// =============================================================
// Klasa sterowania pistoletami
// =============================================================
class GunController {
public:
    GunController();
    void begin();
    void update();

    // Sterowanie pojedynczym pistoletem
    void setGun(GunID id, bool on);
    void enableGun(GunID id, bool enable);

    // Sterowanie z maski bitowej (bit0=P1...bit5=P6)
    void setGunMask(uint8_t mask);
    uint8_t getGunMask() const;

    // Wyłączenie wszystkich
    void allOff();

    // Odczyt stanu
    bool isGunActive(GunID id) const;
    bool isGunEnabled(GunID id) const;
    bool isAnyGunActive() const;
    const GunState& getGunState(GunID id) const;

    // Statystyki
    uint32_t getGunOnTime(GunID id) const;
    void resetStats();

    // Diagnostyka
    void testGun(GunID id, uint32_t duration_ms);
    void testAllGuns(uint32_t duration_ms);

private:
    GunState _guns[NUM_GUNS];
    void _setPin(GunID id, bool on);
};

extern GunController gunController;

#endif // KM251_GUNS_H
