#include "model/blessing_system.h"
#include "model/world.h"
#include "model/unit.h"
#include <algorithm>

// ---------------------------------------------------------------------------
// Query
// ---------------------------------------------------------------------------

bool BlessingSystem::unitHasEffect(const Unit& unit, BlessingEffect effect) const {
    int pid = unit.getOwner().getId();
    for (const auto& b : world.spiritSystem.getPlayerBlessings(pid)) {
        if (b.effect == effect && b.targetUnit == unit.getType())
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Passive refresh
// ---------------------------------------------------------------------------

void BlessingSystem::refreshUnitPassives(Unit& unit) const {
    // Movement bonus: SHADOW_STEP (Stride) gives +2, GALE_SWIFTNESS gives +1
    int movBonus = 0;
    if (unitHasEffect(unit, BlessingEffect::SHADOW_STEP))    movBonus += 2;
    if (unitHasEffect(unit, BlessingEffect::GALE_SWIFTNESS)) movBonus += 1;
    unit.setMovBonus(movBonus);

    // GALE_FAR_REACH: +1 attack range
    unit.setRangeBonus(unitHasEffect(unit, BlessingEffect::GALE_FAR_REACH) ? 1 : 0);

    // GALE_TUMBLE: may move after attacking
    unit.setTumble(unitHasEffect(unit, BlessingEffect::GALE_TUMBLE));

    // MARTIAL_ENDURE: survive one lethal blow — only grant if not yet consumed
    if (unitHasEffect(unit, BlessingEffect::MARTIAL_ENDURE) && !unit.hasEndureConsumed()) {
        unit.setEndureReady(true);
    }
}

void BlessingSystem::refreshAllUnitsForPlayer(int playerId) {
    for (auto& [id, unit] : world.units) {
        if (unit->getOwner().getId() == playerId)
            refreshUnitPassives(*unit);
    }
}

// ---------------------------------------------------------------------------
// Battle hooks
// ---------------------------------------------------------------------------

int BlessingSystem::extraAttackDamage(
    const Unit& attacker, Position attackerPos,
    const Unit& /*defender*/, Position /*defenderPos*/) const
{
    int bonus = 0;

    // SHADOW_DARK_STRIKE: bypass a portion of enemy defense (+5 flat)
    if (unitHasEffect(attacker, BlessingEffect::SHADOW_DARK_STRIKE))
        bonus += 5;

    // FLAME_IGNITE (Fury): raw offensive power — +10 flat attack damage
    if (unitHasEffect(attacker, BlessingEffect::FLAME_IGNITE))
        bonus += 10;

    // FLAME_BLAZE_AURA / MARTIAL_BATTLE_CRY:
    // attacker gains a damage bonus for each adjacent ally that holds these effects
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = attackerPos.row() + dr;
            int nc = attackerPos.col() + dc;
            if (nr < 0 || nr >= Game::HEIGHT || nc < 0 || nc >= Game::WIDTH) continue;
            Position adj(nr, nc);
            if (!world.hasUnitAt(adj)) continue;
            const Unit* ally = world.getUnitAt(adj);
            if (!ally->sameOwner(attacker)) continue;

            if (unitHasEffect(*ally, BlessingEffect::FLAME_BLAZE_AURA))
                bonus += 4;
            if (unitHasEffect(*ally, BlessingEffect::MARTIAL_BATTLE_CRY))
                bonus += 4;
        }
    }

    return bonus;
}

int BlessingSystem::incomingDamageReduction(const Unit& defender) const {
    if (unitHasEffect(defender, BlessingEffect::MARTIAL_IRON_SKIN))
        return 4;
    return 0;
}

bool BlessingSystem::tryEndure(Unit& defender, int incomingDmg) const {
    if (!defender.hasEndure()) return false;
    if (defender.getHealth() > incomingDmg) return false;  // not lethal, no proc
    defender.consumeEndure();
    return true;
}

void BlessingSystem::onHit(Unit& attacker, Unit& defender) const {
    // FLAME_SEAR: regular attacks leave a 3-turn burn (5 dmg/turn)
    if (unitHasEffect(attacker, BlessingEffect::FLAME_SEAR))
        defender.applyBurn(3, 5);
}

void BlessingSystem::onKill(Unit& /*killer*/) const {
    // FLAME_IGNITE (Fury) and SHADOW_STEP (Stride) are passive stat boosts —
    // no on-kill hook needed.
}
