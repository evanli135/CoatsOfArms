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
// File-local capacity cost tables (resource is consumed by existence, not purchase)
// ---------------------------------------------------------------------------

// Food capacity permanently consumed by one living unit of this type.
static int unitFoodCost(UnitType type) {
    switch (type) {
        case UnitType::WARRIOR: return 2;
        case UnitType::SCOUT:   return 1;
        case UnitType::RANGER:  return 2;
        case UnitType::CAVALRY: return 3;
        case UnitType::MAGE:    return 2;
    }
    return 0;
}

// Metal capacity consumed by a placed/queued building (infrastructure only).
// Food-producing buildings and mines are "free" — they produce capacity, not consume it.
static int buildingMetalCost(BuildingType type) {
    switch (type) {
        case BuildingType::BARRACK:     return 2;
        case BuildingType::FOUNDRY:     return 1;
        default:                        return 0;
    }
}

// Food capacity produced by one completed food building.
static int buildingFoodOutput(BuildingType type) {
    switch (type) {
        case BuildingType::FARM:        return 4;
        case BuildingType::FISHERY:     return 3;
        case BuildingType::LUMBER_CAMP: return 2;
        default:                        return 0;
    }
}

// Metal capacity produced by one completed mine.
static int buildingMetalOutput(BuildingType type) {
    return (type == BuildingType::MINE) ? 3 : 0;
}

// Whether a building type may be placed on a given terrain.
static bool canBuildOnTerrain(BuildingType type, Terrain terrain) {
    switch (type) {
        case BuildingType::FARM:        return terrain == Terrain::GRASS;
        case BuildingType::FISHERY:     return terrain == Terrain::OCEAN || terrain == Terrain::RIVER;
        case BuildingType::LUMBER_CAMP: return terrain == Terrain::FOREST;
        case BuildingType::MINE:        return terrain == Terrain::MOUNTAIN;
        default:                        return terrain != Terrain::OCEAN;   // BARRACK, FOUNDRY
    }
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

    // ── Construction: tick queues, place completed buildings on their tiles ─
    for (auto it = constructionQueue.begin(); it != constructionQueue.end(); ) {
        if (it->ownerPlayerId != currentPlayerId) { ++it; continue; }
        it->turnsRemaining--;
        if (it->turnsRemaining <= 0) {
            getTileAt(it->pos).setTileBuilding(it->type);
            it = constructionQueue.erase(it);
        } else {
            ++it;
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
    if (!cityHasBuilding(cityPos, BuildingType::BARRACK))
        return PlayerError::INVALIDTARGET;

    // Check food capacity — the unit will permanently consume this capacity.
    int cost = unitFoodCost(type);
    if (getAvailableCapacity(player.getId(), ResourceType::FOOD) < cost)
        return PlayerError::INSUFFICIENTRESOURCES;

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

std::optional<PlayerError> World::scheduleConstruction(const Position& tilePos, BuildingType type, const Player& player) {
    // Tile must be in a city's border (not the city center itself).
    if (hasCityAt(tilePos)) return PlayerError::INVALIDTARGET;
    const City* city = getCityForTile(tilePos);
    if (!city) return PlayerError::INVALIDTARGET;
    if (!city->hasOwner() || city->getOwner().getId() != player.getId())
        return PlayerError::INVALIDTARGET;

    // Can't build on an occupied tile (unit present or building already there).
    if (getTileAt(tilePos).hasTileBuilding()) return PlayerError::INVALIDTARGET;
    if (getTileAt(tilePos).hasUnit())         return PlayerError::INVALIDTARGET;

    // Building type must be compatible with this tile's terrain.
    if (!canBuildOnTerrain(type, getTileAt(tilePos).getTerrain()))
        return PlayerError::INVALIDTARGET;

    // Only one construction per tile at a time.
    for (const auto& entry : constructionQueue) {
        if (entry.pos == tilePos) return PlayerError::INVALIDTARGET;
    }

    // Check available metal capacity (in-progress construction already counts).
    int cost = buildingMetalCost(type);
    if (getAvailableCapacity(player.getId(), ResourceType::METAL) < cost)
        return PlayerError::INSUFFICIENTRESOURCES;

    // Base 2 turns, -1 per foundry already in this city (min 1).
    Position cpos = getCityPosForTile(tilePos);
    int turns = 2 - countBuildingsInCity(cpos, BuildingType::FOUNDRY);
    turns = std::max(1, turns);

    constructionQueue.push_back({tilePos, cpos, type, turns, player.getId()});
    return std::nullopt;
}

std::optional<PlayerError> World::issueConstructCommand(const Position& tilePos, BuildingType type, const Player& player) {
    auto cmd = std::make_unique<ConstructCommand>(tilePos, type, player);
    auto result = cmd->execute(*this);
    if (!result.has_value()) {
        commandHistory.push_back(std::move(cmd));
    }
    return result;
}

// ---------------------------------------------------------------------------
// Capacity economy
// ---------------------------------------------------------------------------

int World::getTotalCapacity(int playerId, ResourceType rt) const {
    int total = 0;

    if (rt == ResourceType::FOOD) {
        // Food comes entirely from food buildings on border tiles.
        for (const auto& cpos : cityPositions) {
            const City* city = getCityAt(cpos);
            if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
                continue;
            for (const auto& bpos : getCityBorderTiles(cpos)) {
                const Tile& t = getTileAt(bpos);
                if (t.hasTileBuilding())
                    total += buildingFoodOutput(*t.getTileBuilding());
            }
        }
    } else if (rt == ResourceType::METAL) {
        // Metal = city base per owned city + mine output on border tiles.
        for (const auto& cpos : cityPositions) {
            const City* city = getCityAt(cpos);
            if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
                continue;
            total += cityCapacity(ResourceType::METAL);   // base per city
            for (const auto& bpos : getCityBorderTiles(cpos)) {
                const Tile& t = getTileAt(bpos);
                if (t.hasTileBuilding())
                    total += buildingMetalOutput(*t.getTileBuilding());
            }
        }
    }

    return total;
}

int World::getUsedCapacity(int playerId, ResourceType rt) const {
    int used = 0;
    if (rt == ResourceType::FOOD) {
        // Living units
        for (const auto& [id, unit] : units) {
            if (unit->getOwner().getId() == playerId)
                used += unitFoodCost(unit->getType());
        }
        // Units currently in training (capacity reserved while queued)
        for (const auto& cpos : cityPositions) {
            const City* city = getCityAt(cpos);
            if (!city) continue;
            if (city->isTraining()) {
                const TrainingSlot* slot = city->getTrainingSlot();
                if (slot && slot->ownerId == playerId)
                    used += unitFoodCost(slot->unitType);
            }
        }
    } else if (rt == ResourceType::METAL) {
        // Completed buildings on border tiles of owned cities
        for (const auto& cpos : cityPositions) {
            const City* city = getCityAt(cpos);
            if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
                continue;
            for (const auto& bpos : getCityBorderTiles(cpos)) {
                const Tile& t = getTileAt(bpos);
                if (t.hasTileBuilding())
                    used += buildingMetalCost(*t.getTileBuilding());
            }
        }
        // In-progress construction entries (capacity reserved immediately on queue)
        for (const auto& entry : constructionQueue) {
            if (entry.ownerPlayerId == playerId)
                used += buildingMetalCost(entry.type);
        }
    }
    return used;
}

int World::getAvailableCapacity(int playerId, ResourceType rt) const {
    return getTotalCapacity(playerId, rt) - getUsedCapacity(playerId, rt);
}

// ---------------------------------------------------------------------------
// Border / territory queries
// ---------------------------------------------------------------------------

std::unordered_set<Position> World::getCityBorderTiles(Position cityPos) const {
    std::unordered_set<Position> result;
    const City* city = getCityAt(cityPos);
    if (!city) return result;
    int r = city->getBorderRadius();
    for (int dr = -r; dr <= r; ++dr) {
        for (int dc = -r; dc <= r; ++dc) {
            if (dr == 0 && dc == 0) continue;  // exclude city center
            if (std::max(std::abs(dr), std::abs(dc)) > r) continue;
            int nr = cityPos.row() + dr, nc = cityPos.col() + dc;
            if (nr >= 0 && nr < Game::HEIGHT && nc >= 0 && nc < Game::WIDTH)
                result.insert(Position(nr, nc));
        }
    }
    return result;
}

const City* World::getCityForTile(Position pos) const {
    for (const auto& cpos : cityPositions) {
        if (cpos == pos) return getCityAt(cpos);  // city center itself
        const City* city = getCityAt(cpos);
        if (!city) continue;
        int r = city->getBorderRadius();
        if (std::max(std::abs(pos.row() - cpos.row()),
                     std::abs(pos.col() - cpos.col())) <= r)
            return city;
    }
    return nullptr;
}

Position World::getCityPosForTile(Position pos) const {
    for (const auto& cpos : cityPositions) {
        if (cpos == pos) return cpos;
        const City* city = getCityAt(cpos);
        if (!city) continue;
        int r = city->getBorderRadius();
        if (std::max(std::abs(pos.row() - cpos.row()),
                     std::abs(pos.col() - cpos.col())) <= r)
            return cpos;
    }
    return Position(-1, -1);
}

int World::countBuildingsInCity(Position cityPos, BuildingType type) const {
    int count = 0;
    for (const auto& bpos : getCityBorderTiles(cityPos)) {
        const Tile& t = getTileAt(bpos);
        if (t.hasTileBuilding() && *t.getTileBuilding() == type)
            ++count;
    }
    return count;
}

bool World::cityHasBuilding(Position cityPos, BuildingType type) const {
    return countBuildingsInCity(cityPos, type) > 0;
}

std::vector<Position> World::getBuildableTiles(int playerId) const {
    std::vector<Position> result;
    for (const auto& cpos : cityPositions) {
        const City* city = getCityAt(cpos);
        if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
            continue;
        for (const auto& bpos : getCityBorderTiles(cpos)) {
            const Tile& t = getTileAt(bpos);
            if (t.hasTileBuilding()) continue;
            if (t.getTerrain() == Terrain::OCEAN) continue;
            // Check no construction already queued for this tile
            bool queued = false;
            for (const auto& e : constructionQueue)
                if (e.pos == bpos) { queued = true; break; }
            if (!queued) result.push_back(bpos);
        }
    }
    return result;
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

    // Each owned city reveals tiles out to its border radius.
    for (const auto& cpos : cityPositions) {
        const City* city = getCityAt(cpos);
        if (!city || !city->hasOwner() || city->getOwner().getId() != playerId) continue;
        int rv = city->getBorderRadius();
        for (int dr = -rv; dr <= rv; ++dr) {
            for (int dc = -rv; dc <= rv; ++dc) {
                if (std::max(std::abs(dr), std::abs(dc)) > rv) continue;
                int nr = cpos.row() + dr, nc = cpos.col() + dc;
                if (nr >= 0 && nr < Game::HEIGHT && nc >= 0 && nc < Game::WIDTH)
                    visible.insert(Position(nr, nc));
            }
        }
    }

    return visible;
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

            // Mountain near Ironhaven so P1 can mine (within Chebyshev-2 border of city at 1,11)
            set(0, 12, Terrain::MOUNTAIN);

            // --- Units ---
            // Blue team
            world.addUnit(Position(4, 3),  UnitFactory::create(UnitType::WARRIOR, players[0]));
            world.addUnit(Position(5, 6),  UnitFactory::create(UnitType::MAGE,    players[0]));
            // Red team
            world.addUnit(Position(8, 4),  UnitFactory::create(UnitType::SCOUT,   players[1]));
            world.addUnit(Position(7, 8),  UnitFactory::create(UnitType::RANGER,  players[1]));
            world.addUnit(Position(9, 6),  UnitFactory::create(UnitType::CAVALRY, players[1]));

            // --- Cities (each starts with a Barracks + 2 Farms on border tiles) ---
            {
                world.addCity(Position(1, 11), City("Ironhaven"), 0);
                // Starting Barrack south of city; two farms for food production
                // Note: (0,11) is FOREST so use (0,10) and (1,10) instead (both GRASS)
                world.getTileAt(Position(2, 11)).setTileBuilding(BuildingType::BARRACK);
                world.getTileAt(Position(0, 10)).setTileBuilding(BuildingType::FARM);
                world.getTileAt(Position(1, 10)).setTileBuilding(BuildingType::FARM);
            }
            {
                world.addCity(Position(12, 2), City("Stonekeep"), 1);
                // Starting Barrack north of city; two farms for food production
                world.getTileAt(Position(11, 2)).setTileBuilding(BuildingType::BARRACK);
                world.getTileAt(Position(13, 2)).setTileBuilding(BuildingType::FARM);
                world.getTileAt(Position(12, 3)).setTileBuilding(BuildingType::FARM);
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
