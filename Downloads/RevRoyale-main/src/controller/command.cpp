#include "controller/command.h"
#include "model/world.h"
#include "model/error.h"
#include "model/city.h"

// ---------------------------------------------------------------------------
// MoveCommand
// ---------------------------------------------------------------------------

std::optional<PlayerError> MoveCommand::execute(World& world) {
    // Delegates to MovementSystem which performs full Dijkstra validation.
    return world.movementSystem.move(from, to);
}

void MoveCommand::undo(World& world) {
    // Move the unit back to its original tile.
    world.moveUnit(to, from);
    // Un-spend its action for this turn.
    if (Unit* unit = world.getUnitAt(from)) {
        unit->setMoved(false);
    }
}


// ---------------------------------------------------------------------------
// AttackCommand
// ---------------------------------------------------------------------------

std::optional<PlayerError> AttackCommand::execute(World& world) {
    const Unit* attacker = world.getUnitAt(attackerPos);
    const Unit* defender = world.getUnitAt(defenderPos);

    if (!attacker || !attacker->sameOwner(player)) return PlayerError::INVALIDTARGET;
    if (!attacker->canAttack())                     return PlayerError::UNITCANTMOVE;
    if (!defender || defender->sameOwner(player))   return PlayerError::INVALIDTARGET;
    if (!world.canAttack(attackerPos, defenderPos)) return PlayerError::OUTOFREACH;

    world.battle(attackerPos, defenderPos);
    return std::nullopt;
}

void AttackCommand::undo(World& world) {
    // Restore defender (killed or damaged).
    if (world.hasUnitAt(defenderPos)) {
        world.getUnitAt(defenderPos)->setHealth(defenderHpBefore);
    } else {
        try {
            world.addUnit(defenderPos, defenderSnapshot);
        } catch (...) {}
    }

    // Restore attacker (may have died from retaliation).
    if (world.hasUnitAt(attackerPos)) {
        Unit* attacker = world.getUnitAt(attackerPos);
        attacker->setAttacked(false);
        attacker->setHealth(attackerHpBefore);
    } else {
        try {
            world.addUnit(attackerPos, attackerSnapshot);
            world.getUnitAt(attackerPos)->setAttacked(false);
            world.getUnitAt(attackerPos)->setHealth(attackerHpBefore);
        } catch (...) {}
    }
}


// ---------------------------------------------------------------------------
// TrainCommand
// ---------------------------------------------------------------------------

std::optional<PlayerError> TrainCommand::execute(World& world) {
    return world.trainUnit(cityPos, unitType, player);
}

void TrainCommand::undo(World& world) {
    // Training is queued (no unit has spawned yet), so just cancel the order.
    // Capacity is freed automatically when the training slot is cleared.
    if (City* city = world.getTileAt(cityPos).getCityMutable()) {
        city->clearTraining();
    }
}


// ---------------------------------------------------------------------------
// ConstructCommand
// ---------------------------------------------------------------------------

std::optional<PlayerError> ConstructCommand::execute(World& world) {
    return world.scheduleConstruction(tilePos, buildingType, player);
}

void ConstructCommand::undo(World& world) {
    // Remove the construction queue entry; capacity is freed automatically.
    world.cancelConstruction(tilePos, buildingType, player.getId());
}


// ---------------------------------------------------------------------------
// CastCommand
// ---------------------------------------------------------------------------

std::optional<PlayerError> CastCommand::execute(World& world) {
    auto result = world.magicSystem.castSpell(casterPos, targetPos, spell, player);
    // For VEIL, record which positions received the buff so undo can clear them.
    if (!result.has_value() && spell == SpellId::VEIL) {
        veiledPositions.clear();
        veiledPositions.push_back(casterPos);
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int nr = casterPos.row() + dr;
                int nc = casterPos.col() + dc;
                if (nr < 0 || nr >= 16 || nc < 0 || nc >= 16) continue;
                Position adj(nr, nc);
                const Unit* u = world.getUnitAt(adj);
                if (u && u->sameOwner(player)) veiledPositions.push_back(adj);
            }
        }
    }
    return result;
}

void CastCommand::undo(World& world) {
    if (Unit* caster = world.getUnitAt(casterPos)) {
        caster->setMagic(casterMagicBefore);
        caster->setAttacked(false);
        if (spell == SpellId::SHADOW_POUNCE) {
            caster->clearShadowPounce();
        }
    }
    if (spell == SpellId::VEIL) {
        for (const Position& pos : veiledPositions) {
            if (Unit* u = world.getUnitAt(pos)) u->clearVeil();
        }
        return;
    }
    if (spell != SpellId::SHADOW_POUNCE && !targetWasBurning) {
        if (Unit* target = world.getUnitAt(targetPos)) {
            target->clearBurn();
        }
    }
}
