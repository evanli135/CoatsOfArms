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
    : grid(Game::HEIGHT, std::vector<Tile>(Game::WIDTH, Tile())),
      players(players),
      currentPlayerIndex(0),
      battleSystem(*this),
      movementSystem(*this)
{
    if (players.size() < 2) {
        throw std::logic_error("Not enough players\n");
    }
}

Unit* World::getUnitAt(const Position& pos) const {
    auto unitId = getTileAt(pos).getUnit();
    if (!unitId.has_value()) {
        return nullptr;
    }
    
    auto it = units.find(unitId.value());
    return (it != units.end()) ? it->second.get() : nullptr;
}

const Tile& World::getTileAt(const Position& pos) const {
    return grid.at(pos.row()).at(pos.col());
}

Tile& World::getTileAt(const Position& pos) {
    return grid.at(pos.row()).at(pos.col());
}

bool World::canMove(const Position& from, const Position& to) {
    if (!getTileAt(to).isWalkable()) {
        return false;
    }
    
    const Unit* unit = getUnitAt(from);
    if (!unit) return false;
    
    return getDistance(from, to) <= unit->getMovement();
}

void World::nextTurn() {
    turn++;
    currentPlayerIndex = (currentPlayerIndex + 1) % players.size();   

    // Reset movement for all units belonging to current player
    int currentPlayerId = players[currentPlayerIndex].getId();
    for (auto& [id, unit] : units) {
        if (unit->getOwner().getId() == currentPlayerId) {
            unit->setMoved(false);
        }
    }

    printCurrentPlayer(*this);
    notifyObservers(ModelEvent::TURN_CHANGE);
}

void World::addObserver(ModelObserver* observer) {
    if (observer == nullptr) { 
        throw std::logic_error("Nullptr observer"); 
    }
    observers.push_back(observer);
}

void World::notifyObservers(ModelEvent event) {
    for (auto observer: observers) {
        observer->onModelChanged(event);
    }
}

void World::startGame() {
    if (phase == GamePhase::ENDGAME) { 
        throw std::logic_error("Game has ended"); 
    }

    phase = GamePhase::MIDGAME;
    printCurrentPlayer(*this);
    notifyObservers(ModelEvent::TURN_CHANGE);
}

std::optional<PlayerError> World::applyControllerRequest(ControllerRequest action) {
    Position origin = action.getOrigin();
    Position destination = action.getDestination();

    switch (action.getAction()) {
        case (ControllerAction::CON):
        case (ControllerAction::TRN):
            throw std::logic_error("Not implemented yet");

        case (ControllerAction::MOV): {
            return movementSystem.move(origin, destination);
        }

        case (ControllerAction::ATT): {
            const Unit* attacker = getUnitAt(origin);
            const Unit* defender = getUnitAt(destination);
            
            if (!attacker || !attacker->sameOwner(action.getPlayer())) {
                throw InternalError::UNITABSENCE;
            }

            if (!attacker->canMove()) {
                return PlayerError::UNITCANTMOVE;
            }

            if (!defender || defender->sameOwner(action.getPlayer())) {
                throw InternalError::UNITABSENCE;
            }

            if (!canAttack(origin, destination)) {
                return PlayerError::OUTOFREACH;
            }

            battle(origin, destination);
            return std::nullopt;
        }
            
        default:
            throw InternalError::FATAL;
    }
}

bool World::hasUnitAt(const Position& pos) const {
    return getTileAt(pos).hasUnit();
}

const Player& World::getCurrentPlayer() const {
    if (players.empty()) {
        throw std::logic_error("No players in game");
    }
    return players.at(currentPlayerIndex);
}

bool World::canAttack(const Position& from, const Position& to) {
    if (!hasUnitAt(from) || !hasUnitAt(to)) {
        return false;
    }
    
    const Unit* attacker = getUnitAt(from);
    return getDistance(from, to) <= attacker->getRange() + attacker->getMovement();
}

void World::battle(const Position& attackerPos, const Position& defenderPos) {
    if (!hasUnitAt(attackerPos) || !hasUnitAt(defenderPos)) {
        throw std::logic_error("Battle requires units at both positions");
    }
    
    // TODO: Implement proper combat
    // For now, just remove defender
    auto defenderId = getTileAt(defenderPos).removeUnit();
    if (defenderId.has_value()) {
        units.erase(defenderId.value());  // unique_ptr automatically deletes
    }
}

void World::addUnit(const Position& pos, Unit unit) {
    if (!getTileAt(pos).isWalkable()) {
        throw std::invalid_argument("Cannot place unit on non-walkable tile");
    }

    if (getTileAt(pos).hasUnit()) {
        throw std::invalid_argument("Tile already has a unit");
    }

    UnitId id = unit.getId();
    units[id] = std::make_unique<Unit>(std::move(unit));  // Store in map
    getTileAt(pos).placeUnit(id);  // Store ID in tile    
}

World WorldFactory::create(WorldLayout layout, std::vector<Player> players) {
    World world(players);

    switch (layout) {
        case WorldLayout::BASIC: {
            Unit warrior1 = UnitFactory::create(UnitType::WARRIOR, players[0]);
            Unit warrior2 = UnitFactory::create(UnitType::WARRIOR, players[1]);
            
            world.addUnit(Position(0, 0), std::move(warrior1));
            world.addUnit(Position(Game::WIDTH - 1, Game::HEIGHT - 1), std::move(warrior2));
            break;
        }
        case WorldLayout::EMPTY:
            break;
        default:
            throw std::invalid_argument("Unknown WorldLayout");
    }

    return world;
}