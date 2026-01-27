#pragma once

#include "include/model/world.h"
#include <stdexcept>

using namespace Model;

World::World() 
    : map(Game::HEIGHT, std::vector<Tile>(Game::WIDTH, Tile())),
      currentPlayerIndex(0) {}

const std::optional<Unit>& World::getUnitAt(const Position& pos) const {
    return getTileAt(pos).getUnit();
}

std::optional<Unit>& World::getUnitAt(const Position& pos) {
    return getTileAt(pos).getUnit();
}


const Tile& World::getTileAt(const Position& pos) const {
    return map.at(pos.row).at(pos.col);
}

Tile& World::getTileAt(const Position& pos) {
    return map.at(pos.row).at(pos.col);
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

using namespace Logic;

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