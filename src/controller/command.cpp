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
