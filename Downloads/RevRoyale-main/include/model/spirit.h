#pragma once
#include <array>
#include "model/unit.h"   // UnitType

// ---------------------------------------------------------------------------
// Spirit system — elemental forces that imbue units with boon abilities.
// Spirits are granted through prayer at shrines scattered across the map.
// Each shrine interaction offers 3 boon choices; the player picks one.
//
// NOTE: Effects are framework-only — no mechanical implementation yet.
// ---------------------------------------------------------------------------

enum class SpiritType {
    FLAME,    // fire, aggression, burning
    SHADOW,   // stealth, cunning, ambush
    GALE,     // wind, speed, aerial reach
    MARTIAL,  // discipline, armour, endurance
};

// One specific mechanical effect — 3 per spirit, 12 total.
// Effects are defined but not yet wired to game logic.
enum class BoonEffect {
    // Flame (index 0–2 within spirit)
    FLAME_SEAR,           // attacks apply a lingering burn debuff
    FLAME_BLAZE_AURA,     // nearby allies deal bonus fire damage
    FLAME_IGNITE,         // unit gains extra movement after a kill

    // Shadow (index 3–5)
    SHADOW_VEIL,          // harder to target through fog of war
    SHADOW_STEP,          // bonus movement speed after a kill
    SHADOW_DARK_STRIKE,   // attacks bypass a portion of enemy defense

    // Gale (index 6–8)
    GALE_SWIFTNESS,       // +1 movement range
    GALE_FAR_REACH,       // +1 attack range
    GALE_TUMBLE,          // unit may move after attacking

    // Martial (index 9–11)
    MARTIAL_IRON_SKIN,    // reduce incoming physical damage taken
    MARTIAL_BATTLE_CRY,   // adjacent allies gain +attack on this unit's turn
    MARTIAL_ENDURE,       // survive a lethal blow at 1 HP once per engagement
};

// A boon is a (spirit, effect, targetUnit) triple stored per player.
struct Boon {
    SpiritType spirit;
    BoonEffect effect;
    UnitType   targetUnit;   // which unit class receives the buff
};

// ---------------------------------------------------------------------------
// String helpers
// ---------------------------------------------------------------------------

inline const char* spiritName(SpiritType s) {
    switch (s) {
        case SpiritType::FLAME:   return "Flame";
        case SpiritType::SHADOW:  return "Shadow";
        case SpiritType::GALE:    return "Gale";
        case SpiritType::MARTIAL: return "Martial";
    }
    return "Unknown";
}

inline const char* boonEffectName(BoonEffect e) {
    switch (e) {
        case BoonEffect::FLAME_SEAR:          return "Sear";
        case BoonEffect::FLAME_BLAZE_AURA:    return "Blaze Aura";
        case BoonEffect::FLAME_IGNITE:        return "Ignite";
        case BoonEffect::SHADOW_VEIL:         return "Veil";
        case BoonEffect::SHADOW_STEP:         return "Shadow Step";
        case BoonEffect::SHADOW_DARK_STRIKE:  return "Dark Strike";
        case BoonEffect::GALE_SWIFTNESS:      return "Swiftness";
        case BoonEffect::GALE_FAR_REACH:      return "Far Reach";
        case BoonEffect::GALE_TUMBLE:         return "Tumble";
        case BoonEffect::MARTIAL_IRON_SKIN:   return "Iron Skin";
        case BoonEffect::MARTIAL_BATTLE_CRY:  return "Battle Cry";
        case BoonEffect::MARTIAL_ENDURE:      return "Endure";
    }
    return "Unknown";
}

inline const char* boonDescription(BoonEffect e) {
    switch (e) {
        case BoonEffect::FLAME_SEAR:          return "Attacks leave a burning debuff";
        case BoonEffect::FLAME_BLAZE_AURA:    return "Nearby allies gain bonus fire damage";
        case BoonEffect::FLAME_IGNITE:        return "Gain extra movement after a kill";
        case BoonEffect::SHADOW_VEIL:         return "Harder to target through fog";
        case BoonEffect::SHADOW_STEP:         return "Gain movement speed after a kill";
        case BoonEffect::SHADOW_DARK_STRIKE:  return "Bypass a portion of enemy defense";
        case BoonEffect::GALE_SWIFTNESS:      return "+1 movement range";
        case BoonEffect::GALE_FAR_REACH:      return "+1 attack range";
        case BoonEffect::GALE_TUMBLE:         return "May move after attacking";
        case BoonEffect::MARTIAL_IRON_SKIN:   return "Reduce incoming physical damage";
        case BoonEffect::MARTIAL_BATTLE_CRY:  return "Adjacent allies gain +attack";
        case BoonEffect::MARTIAL_ENDURE:      return "Survive a lethal blow at 1 HP once";
    }
    return "";
}

inline const char* unitTypeName(UnitType u) {
    switch (u) {
        case UnitType::WARRIOR: return "Warrior";
        case UnitType::SCOUT:   return "Scout";
        case UnitType::RANGER:  return "Ranger";
        case UnitType::CAVALRY: return "Cavalry";
        case UnitType::MAGE:    return "Mage";
    }
    return "Unit";
}

// ---------------------------------------------------------------------------
// Boon generation
// Generate 3 distinct-spirit boon choices deterministically from a seed.
// Called by World::preparePrayChoices(); also usable by tests / UI previews.
// ---------------------------------------------------------------------------

inline std::array<Boon, 3> generateBoonChoices(int playerId, int turnNumber) {
    // Xorshift32 — deterministic, no stdlib random needed
    unsigned int seed = static_cast<unsigned int>(playerId  * 2654435761u)
                      ^ static_cast<unsigned int>(turnNumber * 40503u + 1234567u);
    if (seed == 0) seed = 0xDEADBEEFu;

    auto rng = [&]() -> unsigned int {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return seed;
    };
    auto randN = [&](int n) -> int {
        return static_cast<int>(rng() % static_cast<unsigned int>(n));
    };

    // Pick 3 of 4 spirits — one of each to guarantee variety
    SpiritType spirits[4] = {
        SpiritType::FLAME, SpiritType::SHADOW, SpiritType::GALE, SpiritType::MARTIAL
    };
    for (int i = 3; i > 0; --i) {
        int j = randN(i + 1);
        SpiritType tmp = spirits[i]; spirits[i] = spirits[j]; spirits[j] = tmp;
    }

    // 3 effect buckets per spirit
    static const BoonEffect flameEffects[]   = { BoonEffect::FLAME_SEAR,
                                                  BoonEffect::FLAME_BLAZE_AURA,
                                                  BoonEffect::FLAME_IGNITE };
    static const BoonEffect shadowEffects[]  = { BoonEffect::SHADOW_VEIL,
                                                  BoonEffect::SHADOW_STEP,
                                                  BoonEffect::SHADOW_DARK_STRIKE };
    static const BoonEffect galeEffects[]    = { BoonEffect::GALE_SWIFTNESS,
                                                  BoonEffect::GALE_FAR_REACH,
                                                  BoonEffect::GALE_TUMBLE };
    static const BoonEffect martialEffects[] = { BoonEffect::MARTIAL_IRON_SKIN,
                                                  BoonEffect::MARTIAL_BATTLE_CRY,
                                                  BoonEffect::MARTIAL_ENDURE };

    auto pickEffect = [&](SpiritType s) -> BoonEffect {
        int i = randN(3);
        switch (s) {
            case SpiritType::FLAME:   return flameEffects[i];
            case SpiritType::SHADOW:  return shadowEffects[i];
            case SpiritType::GALE:    return galeEffects[i];
            case SpiritType::MARTIAL: return martialEffects[i];
        }
        return BoonEffect::FLAME_SEAR;
    };

    static const UnitType UNIT_TYPES[] = {
        UnitType::WARRIOR, UnitType::SCOUT, UnitType::RANGER,
        UnitType::CAVALRY, UnitType::MAGE
    };

    std::array<Boon, 3> choices;
    for (int i = 0; i < 3; ++i) {
        choices[i] = Boon{ spirits[i], pickEffect(spirits[i]), UNIT_TYPES[randN(5)] };
    }
    return choices;
}
