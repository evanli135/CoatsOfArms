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
    auto& q = world.constructionQueue;
    for (auto it = q.begin(); it != q.end(); ++it) {
        if (it->pos == tilePos && it->type == buildingType
            && it->ownerPlayerId == player.getId()) {
            q.erase(it);
            return;
        }
    }
}
