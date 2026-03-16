#include "model/world.h"


#include <cassert>

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

    defender->lowerHP(attacker->getDamage());

    // EDGE CASE: GALE
    if (!defender->isAlive()) {
        world.getTileAt(defenderPos).getUnit().value();
    }

    attacker->setMoved(true);
    return std::nullopt;
}

vector<Position> BattleSystem::getAttackSnapshot(Position origin) const {
    vector<Position> snapshot;

    if (!world.hasUnitAt(origin)) {
        return snapshot;
    }

    const Unit* attacker = world.getUnitAt(origin);
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
}

