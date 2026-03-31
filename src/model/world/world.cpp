#include "controller/observer.h"
#include "controller/command.h"
#include "model/world.h"
#include "controller/error.h"
#include "model/error.h"
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

World::World(World&& other) noexcept
    : grid(std::move(other.grid)),
      players(std::move(other.players)),
      units(std::move(other.units)),
      currentPlayerIndex(other.currentPlayerIndex),
      phase(other.phase),
      turn(other.turn),
      observers(std::move(other.observers)),
      commandHistory(std::move(other.commandHistory)),
      battleSystem(*this),       // re-bind to the new object
      movementSystem(*this)      // re-bind to the new object
{}

Unit* World::getUnitAt(const Position& pos) const {
    auto unitId = getTileAt(pos).getUnit();
    if (!unitId.has_value()) {
        return nullptr;
    }

    auto it = units.find(unitId.value());
    return (it != units.end()) ? it->second.get() : nullptr;
}

bool World::hasCityAt(const Position& pos) const {
    return getTileAt(pos).hasCity();
}

const City* World::getCityAt(const Position& pos) const {
    if (!hasCityAt(pos)) {
        return nullptr;
    }
    return getTileAt(pos).getCity();
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
    // Clear undo history — can't undo across turns.
    commandHistory.clear();

    turn++;
    currentPlayerIndex = (currentPlayerIndex + 1) % players.size();

    // Reset movement for all units belonging to new current player.
    int currentPlayerId = players[currentPlayerIndex].getId();
    for (auto& [id, unit] : units) {
        if (unit->getOwner().getId() == currentPlayerId) {
            unit->setMoved(false);
        }
    }

    printCurrentPlayer(*this);
    notifyObservers(TurnChangeEvent{turn, players[currentPlayerIndex].getId()});
}

void World::addObserver(ModelObserver* observer) {
    if (observer == nullptr) {
        throw std::logic_error("Nullptr observer");
    }
    observers.push_back(observer);
}

void World::notifyObservers(const ModelEvent& event) {
    for (auto observer : observers) {
        observer->onModelChanged(event);
    }
}

void World::startGame() {
    if (phase == GamePhase::ENDGAME) {
        throw std::logic_error("Game has ended");
    }

    phase = GamePhase::MIDGAME;
    printCurrentPlayer(*this);
    notifyObservers(TurnChangeEvent{turn, players[currentPlayerIndex].getId()});
}

std::optional<PlayerError> World::applyControllerRequest(ControllerRequest action) {
    Position origin      = action.getOrigin();
    Position destination = action.getDestination();

    switch (action.getAction()) {
        case (ControllerAction::CON):
        case (ControllerAction::TRN):
            throw std::logic_error("Not implemented yet");

        case (ControllerAction::MOV): {
            auto cmd = std::make_unique<MoveCommand>(origin, destination, action.getPlayer());
            auto result = cmd->execute(*this);
            if (!result.has_value()) {
                commandHistory.push_back(std::move(cmd));
            }
            return result;
        }

        case (ControllerAction::ATT): {
            const Unit* defender = getUnitAt(destination);
            if (!defender) throw InternalError::UNITABSENCE;

            // Snapshot the defender before the attack so undo can restore it.
            auto cmd = std::make_unique<AttackCommand>(
                origin, destination, action.getPlayer(),
                defender->getHealth(), *defender
            );
            auto result = cmd->execute(*this);
            if (!result.has_value()) {
                commandHistory.push_back(std::move(cmd));
            }
            return result;
        }

        default:
            throw InternalError::FATAL;
    }
}

void World::undoLastCommand() {
    if (commandHistory.empty()) return;
    commandHistory.back()->undo(*this);
    commandHistory.pop_back();
}

void World::clearCommandHistory() {
    commandHistory.clear();
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
    Unit* attacker = getUnitAt(attackerPos);
    Unit* defender = getUnitAt(defenderPos);

    if (!attacker || !defender) {
        throw std::logic_error("Battle requires units at both positions");
    }

    // Apply damage through the modifier chain.
    int dmg = attacker->computeDamageAgainst(*defender);
    defender->lowerHP(dmg);
    attacker->setMoved(true);

    if (!defender->isAlive()) {
        auto defenderId = getTileAt(defenderPos).removeUnit();
        if (defenderId.has_value()) {
            notifyObservers(UnitDiedEvent{defenderId.value(), defenderPos});
            units.erase(defenderId.value());
        }
    }
}

void World::moveUnit(const Position& from, const Position& to) {
    auto unitId = getTileAt(from).removeUnit();
    if (!unitId.has_value()) {
        throw std::logic_error("No unit at source position");
    }
    getTileAt(to).placeUnit(unitId.value());
    units.at(unitId.value())->setMoved(true);
    notifyObservers(UnitMovedEvent{unitId.value(), from, to});
}

void World::addUnit(const Position& pos, Unit unit) {
    if (!getTileAt(pos).isWalkable()) {
        throw std::invalid_argument("Cannot place unit on non-walkable tile");
    }

    if (getTileAt(pos).hasUnit()) {
        throw std::invalid_argument("Tile already has a unit");
    }

    UnitId id = unit.getId();
    units[id] = std::make_unique<Unit>(std::move(unit));
    getTileAt(pos).placeUnit(id);
}

World WorldFactory::create(WorldLayout layout, std::vector<Player> players) {
    World world(players);

    switch (layout) {
        case WorldLayout::BASIC: {
            // --- Terrain ---
            auto set = [&](int r, int c, Terrain t) {
                world.getTileAt(Position(r, c)).setTerrain(t);
            };

            // Forest cluster — upper-left
            set(1, 2, Terrain::FOREST); set(1, 3, Terrain::FOREST);
            set(2, 1, Terrain::FOREST); set(2, 2, Terrain::FOREST);
            set(3, 1, Terrain::FOREST);

            // Mountain range — centre
            set(3, 5, Terrain::MOUNTAIN); set(3, 6, Terrain::MOUNTAIN);
            set(4, 6, Terrain::MOUNTAIN); set(4, 7, Terrain::MOUNTAIN);
            set(5, 7, Terrain::MOUNTAIN);

            // River — diagonal crossing from upper-right to lower-left
            set(2, 8, Terrain::RIVER); set(2, 9, Terrain::RIVER);
            set(3, 7, Terrain::RIVER); set(3, 8, Terrain::RIVER);
            set(4, 4, Terrain::RIVER); set(4, 5, Terrain::RIVER);
            set(5, 3, Terrain::RIVER); set(5, 4, Terrain::RIVER);
            set(6, 3, Terrain::RIVER);

            // Ocean lake — centre-right
            set(7, 6, Terrain::OCEAN);
            set(7, 7, Terrain::OCEAN); set(8, 7, Terrain::OCEAN);

            // Forest cluster — lower-right
            set(8, 10, Terrain::FOREST);
            set(9,  9, Terrain::FOREST); set(9, 10, Terrain::FOREST);
            set(10, 9, Terrain::FOREST);

            // --- Units ---
            world.addUnit(Position(0, 0),   UnitFactory::create(UnitType::WARRIOR, players[0]));
            world.addUnit(Position(11, 11), UnitFactory::create(UnitType::WARRIOR, players[1]));
            world.addUnit(Position(9, 11),  UnitFactory::create(UnitType::RANGER,  players[1]));
            break;
        }
        case WorldLayout::EMPTY:
            break;
        default:
            throw std::invalid_argument("Unknown WorldLayout");
    }

    return world;
}
