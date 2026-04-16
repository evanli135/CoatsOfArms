#include "model/world.h"
#include "model/tile.h"

#include <cassert>
#include <algorithm>

bool BattleSystem::canAttack(Position origin, Position destination) const {
    assert(world.hasUnitAt(origin));
    assert(world.hasUnitAt(destination));

    assert(world.getUnitAt(origin)->isAlive() 
        && world.getUnitAt(destination)->isAlive());

    const Unit* attacker = world.getUnitAt(origin);
    const Unit* defender = world.getUnitAt(destination);

    assert(!attacker->sameOwner(*defender));

    return origin.distanceFrom(destination) <= attacker->getRange();
}

std::optional<PlayerError> BattleSystem::battle(Position attackerPos, Position defenderPos) {
    Unit* attacker = world.getUnitAt(attackerPos);
    Unit* defender = world.getUnitAt(defenderPos);

    assert(attacker && defender);

    int dmg = attacker->computeDamageAgainst(*defender);
    dmg += world.blessingSystem.extraAttackDamage(*attacker, attackerPos, *defender, defenderPos);
    dmg -= world.blessingSystem.incomingDamageReduction(*defender);
    dmg = std::max(1, dmg);

    bool wouldKill = (defender->getHealth() <= dmg);

    if (wouldKill && world.blessingSystem.tryEndure(*defender, dmg)) {
        // MARTIAL_ENDURE proc: survive at 1 HP
        defender->setHealth(1);
    } else {
        defender->lowerHP(dmg);
        if (!defender->isAlive()) {
            // FLAME_IGNITE / SHADOW_STEP: grant bonus move to killer
            world.blessingSystem.onKill(*attacker);
            world.getTileAt(defenderPos).removeUnit();
        } else {
            // FLAME_SEAR: apply burn if defender survived
            world.blessingSystem.onHit(*attacker, *defender);
        }
    }

    attacker->setAttacked(true);
    return std::nullopt;
}

CombatContext BattleSystem::computeCombatContext(
    Position attackerPos, Position defenderPos) const
{
    CombatContext ctx;

    // ── Terrain defense bonus ─────────────────────────────────────────────
    // Defender receives a flat damage reduction based on the terrain they stand on.
    Terrain terrain = world.getTileAt(defenderPos).getTerrain();
    if      (terrain == Terrain::MOUNTAIN) ctx.terrainReduction = 3;
    else if (terrain == Terrain::FOREST)   ctx.terrainReduction = 2;

    // ── Flanking ──────────────────────────────────────────────────────────
    // Triggered when the attacker and at least one allied unit are on
    // "opposite sides" of the defender: dot product of the two direction
    // vectors (each measured from the defender's tile) is negative.
    const Unit* attacker = world.getUnitAt(attackerPos);
    if (attacker) {
        int da_r = attackerPos.row() - defenderPos.row();
        int da_c = attackerPos.col() - defenderPos.col();

        for (int dr = -1; dr <= 1 && !ctx.isFlank; ++dr) {
            for (int dc = -1; dc <= 1 && !ctx.isFlank; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int nr = defenderPos.row() + dr;
                int nc = defenderPos.col() + dc;
                if (nr < 0 || nr >= Game::HEIGHT || nc < 0 || nc >= Game::WIDTH) continue;
                Position adj(nr, nc);
                if (adj == attackerPos) continue;   // skip the attacker itself
                if (!world.hasUnitAt(adj)) continue;
                const Unit* ally = world.getUnitAt(adj);
                if (!ally->sameOwner(*attacker)) continue;  // must be on attacker's team
                // Negative dot product ⟹ ally is roughly "opposite" to the attacker
                if (da_r * dr + da_c * dc < 0)
                    ctx.isFlank = true;
            }
        }
        if (ctx.isFlank) ctx.flankBonus = 3;
    }

    // ── Encirclement ──────────────────────────────────────────────────────
    // Counts all enemies adjacent (Chebyshev-1) to the defender.
    // Bonus kicks in at 3+ surrounding enemies: +2 per enemy beyond 2.
    const Unit* defender = world.getUnitAt(defenderPos);
    if (defender) {
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int nr = defenderPos.row() + dr;
                int nc = defenderPos.col() + dc;
                if (nr < 0 || nr >= Game::HEIGHT || nc < 0 || nc >= Game::WIDTH) continue;
                Position adj(nr, nc);
                if (!world.hasUnitAt(adj)) continue;
                const Unit* u = world.getUnitAt(adj);
                if (!u->sameOwner(*defender))
                    ctx.encirclingCount++;
            }
        }
        ctx.isEncircled = (ctx.encirclingCount >= 3);
        if (ctx.isEncircled)
            ctx.encircleBonus = (ctx.encirclingCount - 2) * 2;  // +2 per enemy beyond 2
    }

    return ctx;
}

vector<Position> BattleSystem::getAttackSnapshot(Position origin) const {
    vector<Position> snapshot;

    if (!world.hasUnitAt(origin)) {
        return snapshot;
    }

    const Unit* attacker = world.getUnitAt(origin);
    if (!attacker->canAttack()) return snapshot;

    for (int r = 0; r < Game::HEIGHT; r++) {
        for (int c = 0; c < Game::WIDTH; c++) {
            Position pos(r, c);
            if (world.hasUnitAt(pos) && !attacker->sameOwner(*world.getUnitAt(pos))) {
                if (canAttack(origin, pos)) {
                    snapshot.push_back(pos);
                }
            }
        }
    }
    return snapshot;
}

