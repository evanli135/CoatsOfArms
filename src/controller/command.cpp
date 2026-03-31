#include "controller/command.h"
#include "model/world.h"
#include "model/error.h"

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

    if (!attacker || !attacker->sameOwner(player)) throw InternalError::UNITABSENCE;
    if (!attacker->canMove())                       return PlayerError::UNITCANTMOVE;
    if (!defender || defender->sameOwner(player))   throw InternalError::UNITABSENCE;
    if (!world.canAttack(attackerPos, defenderPos)) return PlayerError::OUTOFREACH;

    world.battle(attackerPos, defenderPos);
    return std::nullopt;
}

void AttackCommand::undo(World& world) {
    // Restore attacker's action.
    if (Unit* attacker = world.getUnitAt(attackerPos)) {
        attacker->setMoved(false);
    }

    if (world.hasUnitAt(defenderPos)) {
        // Defender survived — restore HP to pre-combat value.
        world.getUnitAt(defenderPos)->setHealth(defenderHpBefore);
    } else {
        // Defender was killed — respawn from snapshot if tile is free.
        try {
            world.addUnit(defenderPos, defenderSnapshot);
        } catch (...) {
            // Tile is occupied (another unit moved here) — partial undo only.
        }
    }
}
