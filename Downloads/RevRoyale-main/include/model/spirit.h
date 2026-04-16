#pragma once
#include <array>
#include "model/unit.h"   // UnitType

// ---------------------------------------------------------------------------
// Spirit system — elemental forces that imbue units with blessing abilities.
// Spirits are granted through prayer at shrines scattered across the map.
// Each shrine interaction offers 3 blessing choices; the player picks one.
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
enum class BlessingEffect {
    // Flame (index 0–2 within spirit)
    FLAME_SEAR,           // attacks apply a lingering burn debuff
    FLAME_BLAZE_AURA,     // nearby allies deal bonus fire damage
    FLAME_IGNITE,         // +10 flat attack damage (Fury)

    // Shadow (index 3–5)
    SHADOW_VEIL,          // harder to target through fog of war
    SHADOW_STEP,          // +2 movement range (Stride)
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

// A blessing is a (spirit, effect, targetUnit) triple stored per player.
// If isMagic is true, this blessing also grants the matching spell ability
// (SpellId index = static_cast<int>(effect)), indicated visually on the card.
struct Blessing {
    SpiritType     spirit;
    BlessingEffect effect;
    UnitType       targetUnit;   // which unit class receives the buff
    bool           isMagic = false;
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

inline const char* blessingEffectName(BlessingEffect e) {
    switch (e) {
        case BlessingEffect::FLAME_SEAR:          return "Sear";
        case BlessingEffect::FLAME_BLAZE_AURA:    return "Blaze Aura";
        case BlessingEffect::FLAME_IGNITE:        return "Fury";
        case BlessingEffect::SHADOW_VEIL:         return "Veil";
        case BlessingEffect::SHADOW_STEP:         return "Stride";
        case BlessingEffect::SHADOW_DARK_STRIKE:  return "Dark Strike";
        case BlessingEffect::GALE_SWIFTNESS:      return "Swiftness";
        case BlessingEffect::GALE_FAR_REACH:      return "Far Reach";
        case BlessingEffect::GALE_TUMBLE:         return "Tumble";
        case BlessingEffect::MARTIAL_IRON_SKIN:   return "Iron Skin";
        case BlessingEffect::MARTIAL_BATTLE_CRY:  return "Battle Cry";
        case BlessingEffect::MARTIAL_ENDURE:      return "Endure";
    }
    return "Unknown";
}

inline const char* blessingDescription(BlessingEffect e) {
    switch (e) {
        case BlessingEffect::FLAME_SEAR:          return "Attacks leave a burning debuff";
        case BlessingEffect::FLAME_BLAZE_AURA:    return "Nearby allies gain bonus fire damage";
        case BlessingEffect::FLAME_IGNITE:        return "+10 attack damage on all strikes";
        case BlessingEffect::SHADOW_VEIL:         return "Harder to target through fog";
        case BlessingEffect::SHADOW_STEP:         return "+2 movement range";
        case BlessingEffect::SHADOW_DARK_STRIKE:  return "Bypass a portion of enemy defense";
        case BlessingEffect::GALE_SWIFTNESS:      return "+1 movement range";
        case BlessingEffect::GALE_FAR_REACH:      return "+1 attack range";
        case BlessingEffect::GALE_TUMBLE:         return "May move after attacking";
        case BlessingEffect::MARTIAL_IRON_SKIN:   return "Reduce incoming physical damage";
        case BlessingEffect::MARTIAL_BATTLE_CRY:  return "Adjacent allies gain +attack";
        case BlessingEffect::MARTIAL_ENDURE:      return "Survive a lethal blow at 1 HP once";
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
// Blessing generation
// Generate 3 distinct-spirit blessing choices deterministically from a seed.
// Called by World::preparePrayChoices(); also usable by tests / UI previews.
// ---------------------------------------------------------------------------

inline std::array<Blessing, 3> generateBlessingChoices(int playerId, int turnNumber) {
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
    static const BlessingEffect flameEffects[]   = { BlessingEffect::FLAME_SEAR,
                                                      BlessingEffect::FLAME_BLAZE_AURA,
                                                      BlessingEffect::FLAME_IGNITE };
    static const BlessingEffect shadowEffects[]  = { BlessingEffect::SHADOW_VEIL,
                                                      BlessingEffect::SHADOW_STEP,
                                                      BlessingEffect::SHADOW_DARK_STRIKE };
    static const BlessingEffect galeEffects[]    = { BlessingEffect::GALE_SWIFTNESS,
                                                      BlessingEffect::GALE_FAR_REACH,
                                                      BlessingEffect::GALE_TUMBLE };
    static const BlessingEffect martialEffects[] = { BlessingEffect::MARTIAL_IRON_SKIN,
                                                      BlessingEffect::MARTIAL_BATTLE_CRY,
                                                      BlessingEffect::MARTIAL_ENDURE };

    auto pickEffect = [&](SpiritType s) -> BlessingEffect {
        int i = randN(3);
        switch (s) {
            case SpiritType::FLAME:   return flameEffects[i];
            case SpiritType::SHADOW:  return shadowEffects[i];
            case SpiritType::GALE:    return galeEffects[i];
            case SpiritType::MARTIAL: return martialEffects[i];
        }
        return BlessingEffect::FLAME_SEAR;
    };

    static const UnitType UNIT_TYPES[] = {
        UnitType::WARRIOR, UnitType::SCOUT, UnitType::RANGER,
        UnitType::CAVALRY, UnitType::MAGE
    };

    std::array<Blessing, 3> choices;
    for (int i = 0; i < 3; ++i) {
        bool magic = (randN(10) < 4);   // 40% chance this card is also a magic ability
        choices[i] = Blessing{ spirits[i], pickEffect(spirits[i]), UNIT_TYPES[randN(5)], magic };
    }
    return choices;
}
