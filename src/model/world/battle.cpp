#include "model/world.h"


#include <cassert>

bool BattleSystem::canAttack(Position origin, Position destination) const {
    assert(world.hasUnitAt(origin));
    assert(world.hasUnitAt(destination));

    const Unit* attacker = world.getUnitAt(origin);
    const Unit* defender = world.getUnitAt(destination);

    assert(!attacker->sameOwner(*defender));

    return getDistance(origin, destination) <= attacker->getRange();
}