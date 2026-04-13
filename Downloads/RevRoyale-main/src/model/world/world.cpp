#include "controller/observer.h"
#include "controller/command.h"
#include "model/world.h"
#include "controller/error.h"
#include "model/error.h"
#include "model/unit.h"
#include "model/city.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>

void printCurrentPlayer(const World& world) {
    const Player& currentPlayer = world.getCurrentPlayer();
    std::cout << "Current Player: " << currentPlayer.getId() << "\n";
}

int getDistance(const Position& from, const Position& to) {
    return std::max(std::abs(to.row() - from.row()), std::abs(to.col() - from.col()));
}

// ---------------------------------------------------------------------------
// File-local resource cost tables
// ---------------------------------------------------------------------------

static const std::map<ResourceType, int>& trainCost(UnitType type) {
    static const std::map<ResourceType, int> costs[] = {
        {{ResourceType::FOOD, 2}},                            // WARRIOR
        {{ResourceType::FOOD, 1}},                            // SCOUT
        {{ResourceType::FOOD, 2}, {ResourceType::METAL, 1}},  // RANGER
        {{ResourceType::FOOD, 3}},                            // CAVALRY
        {{ResourceType::FOOD, 2}, {ResourceType::METAL, 2}},  // MAGE
    };
    return costs[static_cast<int>(type)];
}

static const std::map<ResourceType, int>& buildCost(BuildingType type) {
    static const std::map<ResourceType, int> costs[] = {
        {{ResourceType::FOOD, 2}, {ResourceType::METAL, 3}},  // FOUNDRY
        {{ResourceType::FOOD, 3}, {ResourceType::METAL, 2}},  // BARRACK
        {{ResourceType::FOOD, 1}, {ResourceType::METAL, 1}},  // EXTRACTOR
        {{ResourceType::FOOD, 1}, {ResourceType::METAL, 1}},  // SHRINE
        {{ResourceType::FOOD, 1}, {ResourceType::METAL, 1}},  // UTILITY
    };
    return costs[static_cast<int>(type)];
}

// ---------------------------------------------------------------------------

int World::calcHitChance(const Unit& attacker, const Unit& defender) {
    int pct = 80 + (attacker.getPrecision() - defender.getAgility()) * 2;
    return std::max(25, std::min(95, pct));
}

World::World(std::vector<Player> players)
    : grid(Game::HEIGHT, std::vector<Tile>(Game::WIDTH, Tile())),
      players(players),
      currentPlayerIndex(0),
      battleSystem(*this),
      movementSystem(*this),
      trainingSystem(*this)
{
    if (players.size() < 2) {
        throw std::logic_error("Not enough players\n");
    }

    // Initialise each player's resource pool.
    for (const auto& p : players) {
        playerResources[p.getId()] = {
            {ResourceType::FOOD,  10},
            {ResourceType::METAL,  5}
        };
    }
}

World::World(World&& other) noexcept
    : grid(std::move(other.grid)),
      players(std::move(other.players)),
      cityPositions(std::move(other.cityPositions)),
      units(std::move(other.units)),
      currentPlayerIndex(other.currentPlayerIndex),
      phase(other.phase),
      turn(other.turn),
      observers(std::move(other.observers)),
      commandHistory(std::move(other.commandHistory)),
      playerResources(std::move(other.playerResources)),
      constructionQueue(std::move(other.constructionQueue)),
      battleSystem(*this),
      movementSystem(*this),
      trainingSystem(*this)
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
            unit->setAttacked(false);
        }
    }

    // ── Training: tick queues and spawn completed units ──────────────────────
    trainingSystem.advanceTraining(currentPlayerId);

    // ── Economy and construction for the new current player ─────────────────

    // 1. Tick construction queue: decrement turns, complete finished buildings.
    for (auto it = constructionQueue.begin(); it != constructionQueue.end(); ) {
        if (it->ownerPlayerId != currentPlayerId) { ++it; continue; }
        it->turnsRemaining--;
        if (it->turnsRemaining <= 0) {
            if (City* city = getTileAt(it->pos).getCityMutable()) {
                city->addBuilding(it->type);
                std::cout << "Construction complete: building finished at ("
                          << it->pos.row() << "," << it->pos.col() << ")\n";
            }
            it = constructionQueue.erase(it);
        } else {
            ++it;
        }
    }

    // 2. Resource income: +3 FOOD and +1 METAL per owned city.
    auto& res = playerResources[currentPlayerId];
    for (const auto& cpos : cityPositions) {
        const City* city = getCityAt(cpos);
        if (city && city->hasOwner() && city->getOwner().getId() == currentPlayerId) {
            res[ResourceType::FOOD]  += 3;
            res[ResourceType::METAL] += 1;
        }
    }

    // 3. Unit maintenance: -1 FOOD per unit (Cavalry costs 2).
    for (const auto& [id, unit] : units) {
        if (unit->getOwner().getId() != currentPlayerId) continue;
        int maint = (unit->getType() == UnitType::CAVALRY) ? 2 : 1;
        if (res[ResourceType::FOOD] >= maint) {
            res[ResourceType::FOOD] -= maint;
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
            if (!defender) return PlayerError::INVALIDTARGET;
            const Unit* attacker = getUnitAt(origin);
            if (!attacker) return PlayerError::INVALIDTARGET;

            // Snapshot both units before the attack so undo can fully restore them.
            auto cmd = std::make_unique<AttackCommand>(
                origin, destination, action.getPlayer(),
                defender->getHealth(), *defender,
                attacker->getHealth(), *attacker
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

bool World::allUnitsExhausted() const {
    int pid = getCurrentPlayer().getId();
    for (const auto& [id, unit] : units)
        if (unit->getOwner().getId() == pid && !unit->isExhausted())
            return false;
    return true;
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

    // Attacker strikes defender.
    int dmg = attacker->computeDamageAgainst(*defender);
    defender->lowerHP(dmg);
    attacker->setAttacked(true);
    notifyObservers(DamageDealtEvent{defenderPos, dmg});

    // Defender retaliates with equal damage if still alive and attacker is in range.
    if (defender->isAlive() && battleSystem.canAttack(defenderPos, attackerPos)) {
        attacker->lowerHP(dmg);
        notifyObservers(DamageDealtEvent{attackerPos, dmg});
    }

    // Remove dead units (defender first, then check attacker).
    if (!defender->isAlive()) {
        auto defenderId = getTileAt(defenderPos).removeUnit();
        if (defenderId.has_value()) {
            notifyObservers(UnitDiedEvent{defenderId.value(), defenderPos});
            units.erase(defenderId.value());
        }
    }

    if (!attacker->isAlive()) {
        auto attackerId = getTileAt(attackerPos).removeUnit();
        if (attackerId.has_value()) {
            notifyObservers(UnitDiedEvent{attackerId.value(), attackerPos});
            units.erase(attackerId.value());
        }
    }
}

World::CombatForecast World::getCombatForecast(Position from, Position to) const {
    CombatForecast f{};

    if (!hasUnitAt(from) || !hasUnitAt(to)) return f;

    const Unit* attacker = getUnitAt(from);
    const Unit* defender = getUnitAt(to);
    if (!attacker || !defender || attacker->sameOwner(*defender)) return f;

    f.attackerCanAct   = attacker->canAttack();
    f.inRange          = battleSystem.canAttack(from, to);
    f.defenderHpBefore = defender->getHealth();
    f.attackerHpBefore = attacker->getHealth();

    f.attackHitChance       = calcHitChance(*attacker, *defender);
    f.retaliationHitChance  = calcHitChance(*defender, *attacker);

    if (f.attackerCanAct && f.inRange) {
        f.damage          = attacker->computeDamageAgainst(*defender);
        f.defenderHpAfter = std::max(0, defender->getHealth() - f.damage);
        f.lethal          = (f.defenderHpAfter == 0);

        bool defenderCanRetaliate = !f.lethal && battleSystem.canAttack(to, from);
        if (defenderCanRetaliate) {
            f.retaliation     = defender->computeDamageAgainst(*attacker, false);
            f.attackerHpAfter = std::max(0, attacker->getHealth() - f.retaliation);
            f.attackerDies    = (f.attackerHpAfter == 0);
        } else {
            f.retaliation     = 0;
            f.attackerHpAfter = attacker->getHealth();
            f.attackerDies    = false;
        }
    } else {
        f.defenderHpAfter = defender->getHealth();
        f.attackerHpAfter = attacker->getHealth();
    }

    return f;
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

void World::addCity(const Position& pos, City city, int ownerIdx) {
    if (ownerIdx >= 0 && ownerIdx < (int)players.size()) {
        city.setOwner(&players[ownerIdx]);
    }
    getTileAt(pos).setCity(std::move(city));
    cityPositions.push_back(pos);
}

void World::removeUnit(const Position& pos) {
    auto unitId = getTileAt(pos).removeUnit();
    if (unitId.has_value()) {
        units.erase(unitId.value());
    }
}

std::optional<PlayerError> World::trainUnit(const Position& cityPos, UnitType type, const Player& player) {
    // Barracks required to train units.
    const City* city = getCityAt(cityPos);
    if (!city) return PlayerError::INVALIDTARGET;
    if (!city->hasBuilding(BuildingType::BARRACK))
        return PlayerError::INVALIDTARGET;

    // Check and deduct training resources.
    const auto& cost = trainCost(type);
    auto& res = playerResources[player.getId()];
    for (const auto& [rtype, amount] : cost) {
        auto it = res.find(rtype);
        if (it == res.end() || it->second < amount)
            return PlayerError::INSUFFICIENTRESOURCES;
    }
    for (const auto& [rtype, amount] : cost) {
        res[rtype] -= amount;
    }

    return trainingSystem.beginTraining(cityPos, type, player);
}

std::optional<PlayerError> World::issueTrainCommand(const Position& cityPos, UnitType type, const Player& player) {
    auto cmd = std::make_unique<TrainCommand>(cityPos, type, player);
    auto result = cmd->execute(*this);
    if (!result.has_value()) {
        commandHistory.push_back(std::move(cmd));
    }
    return result;
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

// ---------------------------------------------------------------------------
// Construction system
// ---------------------------------------------------------------------------

std::optional<PlayerError> World::scheduleConstruction(const Position& pos, BuildingType type, const Player& player) {
    if (!hasCityAt(pos)) return PlayerError::INVALIDTARGET;
    const City* city = getCityAt(pos);
    if (!city->hasOwner() || city->getOwner().getId() != player.getId())
        return PlayerError::INVALIDTARGET;

    // Only one building may be under construction at a given city at a time.
    for (const auto& entry : constructionQueue) {
        if (entry.pos == pos) return PlayerError::INVALIDTARGET;
    }

    // Check and deduct resources.
    const auto& cost = buildCost(type);
    auto& res = playerResources[player.getId()];
    for (const auto& [rtype, amount] : cost) {
        auto it = res.find(rtype);
        if (it == res.end() || it->second < amount)
            return PlayerError::INSUFFICIENTRESOURCES;
    }
    for (const auto& [rtype, amount] : cost) {
        res[rtype] -= amount;
    }

    // Base 2 turns, -1 per foundry already built (min 1).
    int turns = 2 - city->countBuildings(BuildingType::FOUNDRY);
    turns = std::max(1, turns);

    constructionQueue.push_back({pos, type, turns, player.getId()});
    return std::nullopt;
}

void World::cancelScheduledConstruction(const Position& pos, BuildingType type, const Player& player) {
    for (auto it = constructionQueue.begin(); it != constructionQueue.end(); ++it) {
        if (it->pos == pos && it->type == type && it->ownerPlayerId == player.getId()) {
            // Refund resources.
            const auto& cost = buildCost(type);
            auto& res = playerResources[player.getId()];
            for (const auto& [rtype, amount] : cost) {
                res[rtype] += amount;
            }
            constructionQueue.erase(it);
            return;
        }
    }
}

std::optional<PlayerError> World::issueConstructCommand(const Position& pos, BuildingType type, const Player& player) {
    auto cmd = std::make_unique<ConstructCommand>(pos, type, player);
    auto result = cmd->execute(*this);
    if (!result.has_value()) {
        commandHistory.push_back(std::move(cmd));
    }
    return result;
}

// ---------------------------------------------------------------------------
// Resource helpers
// ---------------------------------------------------------------------------

const std::map<ResourceType, int>& World::getPlayerResources(int playerId) const {
    static const std::map<ResourceType, int> empty;
    auto it = playerResources.find(playerId);
    return it != playerResources.end() ? it->second : empty;
}

void World::refundTrainingCost(UnitType type, const Player& player) {
    const auto& cost = trainCost(type);
    auto& res = playerResources[player.getId()];
    for (const auto& [rtype, amount] : cost) {
        res[rtype] += amount;
    }
}

void World::refundConstructionCost(BuildingType type, const Player& player) {
    const auto& cost = buildCost(type);
    auto& res = playerResources[player.getId()];
    for (const auto& [rtype, amount] : cost) {
        res[rtype] += amount;
    }
}

std::unordered_set<Position> World::getVisiblePositions(int playerId) const {
    std::unordered_set<Position> visible;

    // Each friendly unit reveals a Chebyshev-distance diamond.
    for (int r = 0; r < Game::HEIGHT; ++r) {
        for (int c = 0; c < Game::WIDTH; ++c) {
            const Tile& tile = grid[r][c];
            if (!tile.hasUnit()) continue;
            const Unit* unit = getUnitAt(Position(r, c));
            if (!unit || unit->getOwner().getId() != playerId) continue;

            int sight = unit->getSightRange();
            for (int dr = -sight; dr <= sight; ++dr) {
                for (int dc = -sight; dc <= sight; ++dc) {
                    if (std::max(std::abs(dr), std::abs(dc)) > sight) continue;
                    int nr = r + dr, nc = c + dc;
                    if (nr >= 0 && nr < Game::HEIGHT && nc >= 0 && nc < Game::WIDTH)
                        visible.insert(Position(nr, nc));
                }
            }
        }
    }

    // Each owned city adds 2 tiles of visibility around it.
    for (const auto& cpos : cityPositions) {
        const City* city = getCityAt(cpos);
        if (!city || !city->hasOwner() || city->getOwner().getId() != playerId) continue;
        for (int dr = -2; dr <= 2; ++dr) {
            for (int dc = -2; dc <= 2; ++dc) {
                if (std::max(std::abs(dr), std::abs(dc)) > 2) continue;
                int nr = cpos.row() + dr, nc = cpos.col() + dc;
                if (nr >= 0 && nr < Game::HEIGHT && nc >= 0 && nc < Game::WIDTH)
                    visible.insert(Position(nr, nc));
            }
        }
    }

    return visible;
}

void World::addToConstructionQueue(const Position& pos, BuildingType buildingType) {
    constructionQueue.push_back({pos, buildingType, 2, -1});
}

// ---------------------------------------------------------------------------

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

            // Extra 14×14 fringe — north/east/south/west bands
            set(0, 11, Terrain::FOREST); set(1, 12, Terrain::FOREST); set(2, 13, Terrain::FOREST);
            set(11, 13, Terrain::FOREST); set(12, 12, Terrain::FOREST); set(13, 11, Terrain::FOREST);
            set(13, 3, Terrain::MOUNTAIN); set(12, 0, Terrain::MOUNTAIN); set(11, 1, Terrain::MOUNTAIN);
            set(0, 4, Terrain::RIVER); set(1, 1, Terrain::RIVER);
            set(10, 11, Terrain::RIVER); set(11, 10, Terrain::RIVER); set(12, 9, Terrain::RIVER);

            // --- Units ---
            // Blue team
            world.addUnit(Position(4, 3),  UnitFactory::create(UnitType::WARRIOR, players[0]));
            world.addUnit(Position(5, 6),  UnitFactory::create(UnitType::MAGE,    players[0]));
            // Red team
            world.addUnit(Position(8, 4),  UnitFactory::create(UnitType::SCOUT,   players[1]));
            world.addUnit(Position(7, 8),  UnitFactory::create(UnitType::RANGER,  players[1]));
            world.addUnit(Position(9, 6),  UnitFactory::create(UnitType::CAVALRY, players[1]));

            // --- Cities (each starts with a Barracks so units can be trained) ---
            {
                City p1city("Ironhaven", 4);
                p1city.addBuilding(BuildingType::BARRACK);
                world.addCity(Position(1, 11), std::move(p1city), 0);
            }
            {
                City p2city("Stonekeep", 4);
                p2city.addBuilding(BuildingType::BARRACK);
                world.addCity(Position(12, 2), std::move(p2city), 1);
            }
            break;
        }
        case WorldLayout::EMPTY:
            break;
        default:
            throw std::invalid_argument("Unknown WorldLayout");
    }

    return world;
}
