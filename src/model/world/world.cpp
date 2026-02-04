#pragma once

#include "include/model/world.h"
#include "include/controller/keyboard.h"
#include "include/controller/error.h"

#include <stdexcept>


World::World() 
    : map(Game::HEIGHT, std::vector<Tile>(Game::WIDTH, Tile())),
      currentPlayerIndex(0) {}

const std::optional<Unit>& World::getUnitAt(const Position& pos) const {
    return getTileAt(pos).getUnit();
}

std::optional<Unit>& World::getUnitAt(const Position& pos) {
    return getTileAt(pos).getUnit();
}

std::optional<Unit>& World::getUnitAt(const Position& pos) {
    return getTileAt(pos).getUnit();
}


const Tile& World::getTileAt(const Position& pos) const {
    return map.at(pos.row()).at(pos.col());
}

Tile& World::getTileAt(const Position& pos) {
    return map.at(pos.row()).at(pos.col());
}

void World::moveUnit(const Position& from, const Position& to) {
    if (!canMove(from, to) || !getTileAt(from).hasUnit()) {
        throw std::invalid_argument("Invalid move");
    }

    auto unitOpt = getTileAt(from).removeUnit();
    if (unitOpt.has_value()) {
        getTileAt(to).placeUnit(unitOpt.value());
    } else {
        throw std::logic_error("FATAL INTERNAL: No unit at the source position");
    }
}


bool World::canMove(const Position& from, const Position& to) {
    if (!getTileAt(to).isWalkable()) {
        return false;
    }
    
    // Further movement logic can be implemented here
    return true;
}

void World::nextTurn() {
    turn++;
    currentPlayerIndex = (currentPlayerIndex + 1) % players.size();   
}

std::optional<PlayerError> World::applyControllerRequest(ControllerRequest action) {
    Position origin = action.getOrigin();
    Position destination = action.getDestination();
    switch (action.getAction()) {
        case (ControllerAction::MOV):

            // Is there a friendly at origin
            if (!hasUnitAt(origin) ||
             getUnitAt(origin).value().sameOwner(action.getPlayer()) ) {
                throw InternalError::UNITABSENCE;
            }

            // If destination is traversable
            if (!getTileAt(destination).isWalkable()) {
                return PlayerError::UNTRAVERSABLECELL;
            }

            // If the unit can move that far
            if (!canMove(origin, destination)) {
                return PlayerError::OUTOFREACH;
            }

            moveUnit(origin, destination);

            return std::nullopt;

        case (ControllerAction::ATT):
            // If there is a friendly at origin
            if (!hasUnitAt(origin) || getUnitAt(origin).value().sameOwner(action.getPlayer())) {
                throw InternalError::UNITABSENCE;
            }

            // If there is an enemy at destination
            if (!hasUnitAt(destination) || !getUnitAt(origin).value().sameOwner(action.getPlayer())) {
                throw InternalError::UNITABSENCE;
            }

            // If the unit can reach with their attack
            if (!canAttack(origin, destination)) {
                return PlayerError::OUTOFREACH;
            }

            battle(origin, destination);
            return std::nullopt;
            
        default:
            throw InternalError::FATAL;
    }
}

int Logic::stepCost(const Unit& unit, const Tile& tile) {
    switch (tile.getTerrain()) {
        case GRASS:
            return 1;
        case FOREST:
            return 2;
        case WATER:
            return 999; // Impassable
        case MOUNTAIN:
            return 3;
        default:
            return 1;
    }
};