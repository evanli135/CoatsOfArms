#pragma once

#include <optional>
#include <vector>

#include "model/spirit.h"       // BlessingEffect
#include "model/util.h"         // Position
#include "model/player.h"       // Player
#include "controller/error.h"   // PlayerError

class World;

// ---------------------------------------------------------------------------
// SpellId — every castable ability in the magic system.
//
// Each spell corresponds 1-to-1 with a BlessingEffect; when blessings are
// fully wired, holding the matching blessing will be a prerequisite to cast.
// Effects are framework-only — no mechanical implementation yet.
// ---------------------------------------------------------------------------
enum class SpellId {
    // Flame
    SEAR,          // apply lingering burn to a target unit
    BLAZE_AURA,    // nearby allies gain bonus fire damage this turn
    IGNITE,        // gain extra movement after next kill

    // Shadow
    VEIL,          // enter magical concealment until end of turn
    SHADOW_STEP,   // reposition after defeating an enemy
    DARK_STRIKE,   // next attack bypasses a portion of enemy defense

    // Gale
    SWIFTNESS,     // gain +1 movement range this turn
    FAR_REACH,     // gain +1 attack range this turn
    TUMBLE,        // may move again after attacking this turn

    // Martial
    IRON_SKIN,     // reduce incoming physical damage this turn
    BATTLE_CRY,    // adjacent allies gain an attack bonus this turn
    ENDURE,        // survive a lethal blow at 1 HP once per engagement

    // Scout special
    SHADOW_POUNCE, // Scout vanishes; next attack deals double damage

    // Cavalry special
    FLAME_CHARGE,  // straight-line charge, damages enemies, leaves fire trail
};

// ---------------------------------------------------------------------------
// SpellDef — static, shared definition for one spell.
// ---------------------------------------------------------------------------
struct SpellDef {
    const char*    name;
    const char*    description;
    int            cost;              // magic points deducted when cast
    BlessingEffect requiredBlessing;  // shrine blessing that unlocks this spell
};

// ---------------------------------------------------------------------------
// MagicSystem — manages per-unit magic pools and spell-cast validation.
//
// Each unit's magic is driven by its Affinity (aff) stat:
//   max magic  = aff × 5
//   regen/turn = aff   (called at the start of the owning player's turn)
//
// Spell effects are declared but not yet implemented — this class provides
// the validation and resource-deduction layer that future effects will slot into.
// ---------------------------------------------------------------------------
class MagicSystem {
    friend class World;
public:
    explicit MagicSystem(World& world) : world(world) {}

    /** Regenerate magic for every unit owned by playerId.  Called at turn start. */
    void advanceMagic(int playerId);

    /** True if the unit at pos exists, belongs to player, and has sufficient magic. */
    bool canCast(Position unitPos, SpellId spell, const Player& player) const;

    /** Cast a spell from casterPos onto targetPos.
     *  Validates ownership, magic cost, range, and spell-specific rules.
     *  On success, deducts magic and applies the spell effect. */
    std::optional<PlayerError> castSpell(Position      casterPos,
                                         Position      targetPos,
                                         SpellId       spell,
                                         const Player& player);

    /** Returns enemy tile positions that caster can target with the given spell. */
    std::vector<Position> getCastablePositions(Position      casterPos,
                                               const Player& player,
                                               SpellId       spell) const;

    /** Look up the static definition for a spell. */
    static const SpellDef& getSpellDef(SpellId spell);

private:
    World& world;
};
