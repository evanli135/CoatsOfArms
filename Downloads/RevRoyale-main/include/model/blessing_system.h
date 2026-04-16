#pragma once

#include "model/spirit.h"   // BlessingEffect, Blessing
#include "model/util.h"     // Position
#include "controller/error.h"

class World;
class Unit;

// ---------------------------------------------------------------------------
// BlessingSystem — implements all 12 blessing effects.
//
// Passive effects (SWIFTNESS, FAR_REACH, TUMBLE, ENDURE) are baked into Unit
// fields via refreshUnitPassives().  Active effects are applied through the
// battle hook methods called by BattleSystem::battle().
//
// Effect summary:
//   FLAME_SEAR         — regular attacks apply a 3-turn burn (5 dmg/turn)
//   FLAME_BLAZE_AURA   — adjacent allies deal +4 damage when attacking
//   FLAME_IGNITE       — kill an enemy → bonus move granted
//   SHADOW_VEIL        — (fog system, not yet mechanical)
//   SHADOW_STEP        — kill an enemy → bonus move granted
//   SHADOW_DARK_STRIKE — +5 flat bonus damage on attacks (simulates def bypass)
//   GALE_SWIFTNESS     — +1 movement range (passive, baked into unit)
//   GALE_FAR_REACH     — +1 attack range (passive, baked into unit)
//   GALE_TUMBLE        — unit may move after attacking (passive)
//   MARTIAL_IRON_SKIN  — reduce incoming damage by 4
//   MARTIAL_BATTLE_CRY — adjacent allies deal +4 damage when attacking
//   MARTIAL_ENDURE     — survive one lethal blow at 1 HP (one-time)
// ---------------------------------------------------------------------------
class BlessingSystem {
    friend class World;
public:
    explicit BlessingSystem(World& world) : world(world) {}

    // ── Passive refresh ───────────────────────────────────────────────────

    /** Bakes SWIFTNESS, FAR_REACH, TUMBLE, and ENDURE into unit's stat fields.
     *  Call after a blessing is granted or when creating a unit with blessings. */
    void refreshUnitPassives(Unit& unit) const;

    /** Calls refreshUnitPassives on every fielded unit owned by playerId.
     *  Called by World::completePray after a blessing is successfully granted. */
    void refreshAllUnitsForPlayer(int playerId);

    // ── Battle hooks ──────────────────────────────────────────────────────

    /** Extra outgoing damage contributed by DARK_STRIKE, BLAZE_AURA, BATTLE_CRY. */
    int extraAttackDamage(const Unit& attacker, Position attackerPos,
                          const Unit& defender, Position defenderPos) const;

    /** Flat incoming damage reduction from MARTIAL_IRON_SKIN. */
    int incomingDamageReduction(const Unit& defender) const;

    /** MARTIAL_ENDURE: if this blow would kill and endure is ready, survive at 1 HP.
     *  Returns true if endure proc'd; caller should clamp defender to 1 HP instead. */
    bool tryEndure(Unit& defender, int incomingDmg) const;

    /** FLAME_SEAR: apply burn to defender after a hit (called when defender survived). */
    void onHit(Unit& attacker, Unit& defender) const;

    /** FLAME_IGNITE / SHADOW_STEP: grant a bonus post-kill move to the killer. */
    void onKill(Unit& killer) const;

    // ── Query ─────────────────────────────────────────────────────────────

    /** True if the unit's player holds a blessing matching both effect and unit type. */
    bool unitHasEffect(const Unit& unit, BlessingEffect effect) const;

private:
    World& world;
};
