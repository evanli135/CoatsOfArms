#include "controller/observer.h"
#include "model/world.h"
#include "controller/error.h"
#include "model/unit.h"

#include <iostream>
#include <stdexcept>

void printCurrentPlayer(const World& world) {
    const Player& currentPlayer = world.getCurrentPlayer();
    std::cout << "Current Player: " << currentPlayer.getId() << "\n";
}

int getDistance(const Position& from, const Position& to) {
    return std::max(std::abs(to.row() - from.row()), std::abs(to.col() - from.col()));
}

World::World(std::vector<Player> players) 
    : map(Game::HEIGHT, std::vector<Tile>(Game::WIDTH, Tile())),
      players(players),
      currentPlayerIndex(0)
    //   units(std::map<int, std::vector<Unit*>>())
       {
        if (players.size() < 2) {
            throw std::logic_error("Not enough players\n");
        }
        
        // for (auto &player : players) {
        //     units[player.getId()] = std::vector<Unit*>();
        // }
      }

const Unit* World::getUnitAt(const Position& pos) const {
    const auto& opt = getTileAt(pos).getUnit();
    return opt.has_value() ? &(*opt) : nullptr;
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

    if (auto unitOpt = getTileAt(from).removeUnit(); unitOpt.has_value()) {
        getTileAt(to).placeUnit(unitOpt.value());
        getTileAt(to).getUnit().value().setMoved(true);
    } else {
        throw std::logic_error("FATAL INTERNAL: No unit at the source position");
    }
}


bool World::canMove(const Position& from, const Position& to) {
    if (!getTileAt(to).isWalkable()) {
        return false;
    }
    
    // Further movement logic can be implemented here
    return getDistance(from, to) <= getUnitAt(from)->getMovement();
}

void World::nextTurn() {
    turn++;

    currentPlayerIndex = (currentPlayerIndex + 1) % players.size();   

    int currentPlayerId = players[currentPlayerIndex].getId();
    for (int row = 0; row < Game::HEIGHT; row++) {
        for (int col = 0; col < Game::WIDTH; col++) {
            Position pos(row, col);
            if (hasUnitAt(pos)) {
                Unit& unit = getTileAt(pos).getUnit().value();
                if (unit.getOwner().getId() == currentPlayerId) {
                    unit.setMoved(false);
                }
            }
        }
    }

    printCurrentPlayer(*this);

    notifyObservers(ModelEvent::TURN_CHANGE);
}

void World::addObserver(ModelObserver* observer) {
    if (observer == nullptr) { throw std::logic_error("Nullptr observer"); }

    observers.push_back(observer);
}

void World::notifyObservers(ModelEvent event) {
    for (auto observer: observers) {
        observer->onModelChanged(event);
    }
}

void World::startGame() {
    if (phase == GamePhase::ENDGAME) { throw std::logic_error("Game has ended"); }

    phase = GamePhase::MIDGAME;

    printCurrentPlayer(*this);

    notifyObservers(ModelEvent::TURN_CHANGE);
}

std::optional<PlayerError> World::applyControllerRequest(ControllerRequest action) {
    Position origin = action.getOrigin();
    Position destination = action.getDestination();
    switch (action.getAction()) {
        case (ControllerAction::MOV):

            // Is there a friendly at origin
            if (!hasUnitAt(origin) ||
             !getCopyAt(origin).value().sameOwner(action.getPlayer()) ) {
                throw InternalError::UNITABSENCE;
            }

            if (getUnitAt(origin)->canMove() == false) {
                return PlayerError::UNITCANTMOVE;
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
            if (!hasUnitAt(origin) || !getCopyAt(origin).value().sameOwner(action.getPlayer())) {
                throw InternalError::UNITABSENCE;
            }

            if (getUnitAt(origin)->canMove() == false) {
                return PlayerError::UNITCANTMOVE;
            }

            // If there is an enemy at destination
            if (!hasUnitAt(destination) || !getCopyAt(origin).value().sameOwner(action.getPlayer())) {
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


bool World::hasUnitAt(const Position& pos) const {
    return getTileAt(pos).hasUnit();
}

const std::optional<Unit> World::getCopyAt(const Position& pos) const {
    return getTileAt(pos).getUnit();
}

const Player& World::getCurrentPlayer() const {
    if (players.empty()) {
        throw std::logic_error("No players in game");
    }
    return players.at(currentPlayerIndex);
}

//INCOMPL
bool World::canAttack(const Position& from, const Position& to) {
    // Check both positions have units
    if (!hasUnitAt(from) || !hasUnitAt(to)) {
        return false;
    }
    
    // TODO: Get actual attack range from unit stats
    return getDistance(from, to) <= getUnitAt(from)->getRange() + getUnitAt(from)->getMovement();
}

// INCOMPL
void World::battle(const Position& attackerPos, const Position& defenderPos) {
    Tile& attackerTile = getTileAt(attackerPos);
    Tile& defenderTile = getTileAt(defenderPos);
    
    if (!attackerTile.hasUnit() || !defenderTile.hasUnit()) {
        throw std::logic_error("Battle requires units at both positions");
    }
}

void World::addUnit(const Position& pos, const Unit& unit) {
    if (!getTileAt(pos).isWalkable()) {
        throw std::invalid_argument("Cannot place unit on non-walkable tile");
    }

    if (!getTileAt(pos).hasUnit()) {
        throw std::invalid_argument("Tile already has a unit");
    }

    getTileAt(pos).placeUnit(unit);
    // units[unit.getOwner().getId()].push_back(&getTileAt(pos).getUnit().value());
}


int Logic::stepCost(const Unit& unit, const Tile& tile) {
    switch (tile.getTerrain()) {
        case Terrain::GRASS:
            return 1;
        case Terrain::FOREST:
            return 2;
        case Terrain::RIVER:
            return 4;
        case Terrain::OCEAN:
            return 999; // Impassable
        case Terrain::MOUNTAIN:
            return 3;
        default:
            return 1;
    }
};


World WorldFactory::create(WorldLayout layout, std::vector<Player> players) {
    World world(players);

    switch (layout) {
        case WorldLayout::BASIC:
            world.getTileAt(Position(0, 0)).placeUnit(UnitFactory::create(UnitType::WARRIOR, players[0]));
            world.getTileAt(Position(Game::WIDTH - 1, Game::HEIGHT - 1)).placeUnit(UnitFactory::create(UnitType::WARRIOR, players[1]));
            break;
        case WorldLayout::EMPTY:
            // No units placed
            break;
        default:
            throw std::invalid_argument("Unknown WorldLayout");
    }

    return world;
}