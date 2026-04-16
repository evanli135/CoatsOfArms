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

    // Scout special (index 12)
    SHADOW_POUNCE,        // Scout vanishes for one turn; next attack deals double damage

    // Cavalry special (index 13)
    FLAME_CHARGE,         // Cavalry charges in a straight line, leaving fire trail
};

// ---------------------------------------------------------------------------
// isActivatedAbility — true if an effect requires explicit casting (costs magic
// and appears in the CAST menu); false if it is an always-on passive buff.
// ---------------------------------------------------------------------------
inline bool isActivatedAbility(BlessingEffect e) {
    switch (e) {
        // ── Actives (require clicking CAST, cost magic) ──────────────────────
        case BlessingEffect::FLAME_SEAR:          return true;   // apply burn to enemy
        case BlessingEffect::FLAME_BLAZE_AURA:    return true;   // buff nearby allies
        case BlessingEffect::SHADOW_VEIL:         return true;   // enter concealment
        case BlessingEffect::SHADOW_DARK_STRIKE:  return true;   // buff next attack
        case BlessingEffect::MARTIAL_IRON_SKIN:   return true;   // reduce damage this turn
        case BlessingEffect::MARTIAL_BATTLE_CRY:  return true;   // buff adjacent allies
        case BlessingEffect::SHADOW_POUNCE:       return true;   // Scout special
        case BlessingEffect::FLAME_CHARGE:        return true;   // Cavalry special (activated charge)

        // ── Passives (automatic — no activation, no magic cost) ───────────────
        case BlessingEffect::FLAME_IGNITE:        return false;  // +10 flat attack (Fury)
        case BlessingEffect::SHADOW_STEP:         return false;  // +2 movement (Stride)
        case BlessingEffect::GALE_SWIFTNESS:      return false;  // +1 movement
        case BlessingEffect::GALE_FAR_REACH:      return false;  // +1 attack range
        case BlessingEffect::GALE_TUMBLE:         return false;  // move again after attacking
        case BlessingEffect::MARTIAL_ENDURE:      return false;  // survive lethal blow once (auto)
    }
    return false;
}

// A blessing is a (spirit, effect, targetUnit) triple stored per player.
// isMagic is derived directly from the effect — active abilities cost magic and
// show in the CAST menu; passive buffs apply automatically every turn.
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
        case BlessingEffect::SHADOW_POUNCE:       return "Shadow Pounce";
        case BlessingEffect::FLAME_CHARGE:        return "Flame Charge";
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
        case BlessingEffect::SHADOW_POUNCE:       return "Scout vanishes; next attack deals 2x damage";
        case BlessingEffect::FLAME_CHARGE:        return "Cavalry charges in a line, leaving a fire trail";
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

    // Effect buckets per spirit (FLAME has 4: 3 general + FLAME_CHARGE Cavalry-only)
    static const BlessingEffect flameEffects[]   = { BlessingEffect::FLAME_SEAR,
                                                      BlessingEffect::FLAME_BLAZE_AURA,
                                                      BlessingEffect::FLAME_IGNITE,
                                                      BlessingEffect::FLAME_CHARGE };
    static const BlessingEffect shadowEffects[]  = { BlessingEffect::SHADOW_VEIL,
                                                      BlessingEffect::SHADOW_STEP,
                                                      BlessingEffect::SHADOW_DARK_STRIKE,
                                                      BlessingEffect::SHADOW_POUNCE };
    static const BlessingEffect galeEffects[]    = { BlessingEffect::GALE_SWIFTNESS,
                                                      BlessingEffect::GALE_FAR_REACH,
                                                      BlessingEffect::GALE_TUMBLE };
    static const BlessingEffect martialEffects[] = { BlessingEffect::MARTIAL_IRON_SKIN,
                                                      BlessingEffect::MARTIAL_BATTLE_CRY,
                                                      BlessingEffect::MARTIAL_ENDURE };

    auto pickEffect = [&](SpiritType s) -> BlessingEffect {
        switch (s) {
        case SpiritType::FLAME:  { int i = randN(4); return flameEffects[i]; }
        case SpiritType::SHADOW: { int i = randN(4); return shadowEffects[i]; }
        default: break;
        }
        int i = randN(3);
        switch (s) {
            case SpiritType::GALE:    return galeEffects[i];
            case SpiritType::MARTIAL: return martialEffects[i];
            default: break;
        }
        return BlessingEffect::FLAME_SEAR;
    };

    static const UnitType UNIT_TYPES[] = {
        UnitType::WARRIOR, UnitType::SCOUT, UnitType::RANGER,
        UnitType::CAVALRY, UnitType::MAGE
    };

    std::array<Blessing, 3> choices;
    for (int i = 0; i < 3; ++i) {
        BlessingEffect effect = pickEffect(spirits[i]);

        // Unit type: FLAME_CHARGE is Cavalry-only; SHADOW_POUNCE is Scout-only; rest are random.
        UnitType targetUnit =
            (effect == BlessingEffect::FLAME_CHARGE)  ? UnitType::CAVALRY :
            (effect == BlessingEffect::SHADOW_POUNCE) ? UnitType::SCOUT
                                                      : UNIT_TYPES[randN(5)];

        // isMagic is determined by the effect — never random.
        bool magic = isActivatedAbility(effect);

        choices[i] = Blessing{ spirits[i], effect, targetUnit, magic };
    }
    return choices;
}
