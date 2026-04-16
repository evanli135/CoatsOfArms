#include "controller/observer.h"
#include "controller/command.h"
#include "model/world.h"
#include "controller/error.h"
#include "model/error.h"
#include "model/unit.h"
#include "model/city.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <random>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static void printCurrentPlayer(const World& world) {
    std::cout << "Current Player: " << world.getCurrentPlayer().getId() << "\n";
}

static int chebyshev(const Position& a, const Position& b) {
    return std::max(std::abs(b.row() - a.row()), std::abs(b.col() - a.col()));
}

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

World::World(std::vector<Player> players)
    : grid(Game::HEIGHT, std::vector<Tile>(Game::WIDTH, Tile())),
      players(players),
      currentPlayerIndex(0),
      battleSystem(*this),
      movementSystem(*this),
      trainingSystem(*this),
      resourceSystem(*this),
      constructionSystem(*this),
      spiritSystem(*this),
      magicSystem(*this),
      blessingSystem(*this)
{
    if (players.size() < 2)
        throw std::logic_error("Not enough players\n");
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
      battleSystem(*this),
      movementSystem(*this),
      trainingSystem(*this),
      resourceSystem(*this),
      constructionSystem(*this),
      spiritSystem(*this),
      magicSystem(*this),
      blessingSystem(*this)
{
    // Move system-owned state (each system's data initialised empty above, then filled here)
    constructionSystem.queue            = std::move(other.constructionSystem.queue);
    spiritSystem.shrinePositions        = std::move(other.spiritSystem.shrinePositions);
    spiritSystem.playerBlessings        = std::move(other.spiritSystem.playerBlessings);
    spiritSystem.pendingPrayChoices     = std::move(other.spiritSystem.pendingPrayChoices);
}

// ---------------------------------------------------------------------------
// Core tile / unit / city accessors
// ---------------------------------------------------------------------------

Unit* World::getUnitAt(const Position& pos) const {
    auto unitId = getTileAt(pos).getUnit();
    if (!unitId.has_value()) return nullptr;
    auto it = units.find(unitId.value());
    return (it != units.end()) ? it->second.get() : nullptr;
}

bool World::hasCityAt(const Position& pos) const {
    return getTileAt(pos).hasCity();
}

const City* World::getCityAt(const Position& pos) const {
    if (!hasCityAt(pos)) return nullptr;
    return getTileAt(pos).getCity();
}

const Tile& World::getTileAt(const Position& pos) const {
    return grid.at(pos.row()).at(pos.col());
}

Tile& World::getTileAt(const Position& pos) {
    return grid.at(pos.row()).at(pos.col());
}

bool World::hasUnitAt(const Position& pos) const {
    return getTileAt(pos).hasUnit();
}

bool World::canMove(const Position& from, const Position& to) {
    if (!getTileAt(to).isWalkable()) return false;
    const Unit* unit = getUnitAt(from);
    if (!unit) return false;
    return chebyshev(from, to) <= unit->getMovement();
}

bool World::canAttack(const Position& from, const Position& to) {
    if (!hasUnitAt(from) || !hasUnitAt(to)) return false;
    const Unit* attacker = getUnitAt(from);
    return chebyshev(from, to) <= attacker->getRange() + attacker->getMovement();
}

const Player& World::getCurrentPlayer() const {
    if (players.empty()) throw std::logic_error("No players in game");
    return players.at(currentPlayerIndex);
}

bool World::allUnitsExhausted() const {
    int pid = getCurrentPlayer().getId();
    for (const auto& [id, unit] : units)
        if (unit->getOwner().getId() == pid && !unit->isExhausted())
            return false;
    return true;
}

// ---------------------------------------------------------------------------
// Turn management
// ---------------------------------------------------------------------------

void World::nextTurn() {
    commandHistory.clear();

    turn++;
    currentPlayerIndex = (currentPlayerIndex + 1) % players.size();
    int pid = players[currentPlayerIndex].getId();

    // Reset movement flags for the new current player's units.
    for (auto& [id, unit] : units) {
        if (unit->getOwner().getId() == pid) {
            unit->setMoved(false);
            unit->setAttacked(false);
        }
    }

    trainingSystem.advanceTraining(pid);
    constructionSystem.advanceConstruction(pid);
    magicSystem.advanceMagic(pid);

    printCurrentPlayer(*this);
    notifyObservers(TurnChangeEvent{turn, players[currentPlayerIndex].getId()});
}

void World::startGame() {
    if (phase == GamePhase::ENDGAME)
        throw std::logic_error("Game has ended");
    phase = GamePhase::MIDGAME;
    printCurrentPlayer(*this);
    notifyObservers(TurnChangeEvent{turn, players[currentPlayerIndex].getId()});
}

// ---------------------------------------------------------------------------
// Observer management
// ---------------------------------------------------------------------------

void World::addObserver(ModelObserver* observer) {
    if (!observer) throw std::logic_error("Nullptr observer");
    observers.push_back(observer);
}

void World::notifyObservers(const ModelEvent& event) {
    for (auto obs : observers) obs->onModelChanged(event);
}

// ---------------------------------------------------------------------------
// Command / undo
// ---------------------------------------------------------------------------

std::optional<PlayerError> World::applyControllerRequest(ControllerRequest action) {
    Position origin      = action.getOrigin();
    Position destination = action.getDestination();

    switch (action.getAction()) {
        case ControllerAction::CON:
        case ControllerAction::TRN:
            throw std::logic_error("Not implemented yet");

        case ControllerAction::MOV: {
            auto cmd = std::make_unique<MoveCommand>(origin, destination, action.getPlayer());
            auto result = cmd->execute(*this);
            if (!result.has_value()) commandHistory.push_back(std::move(cmd));
            return result;
        }

        case ControllerAction::ATT: {
            const Unit* defender = getUnitAt(destination);
            if (!defender) return PlayerError::INVALIDTARGET;
            const Unit* attacker = getUnitAt(origin);
            if (!attacker) return PlayerError::INVALIDTARGET;

            auto cmd = std::make_unique<AttackCommand>(
                origin, destination, action.getPlayer(),
                defender->getHealth(), *defender,
                attacker->getHealth(), *attacker);
            auto result = cmd->execute(*this);
            if (!result.has_value()) commandHistory.push_back(std::move(cmd));
            return result;
        }

        case ControllerAction::CAST: {
            const Unit* caster = getUnitAt(origin);
            if (!caster) return PlayerError::INVALIDTARGET;
            const Unit* target = getUnitAt(destination);
            if (!target) return PlayerError::INVALIDTARGET;

            int  casterMagicBefore = caster->getCurrentMagic();
            bool targetWasBurning  = target->hasBurn();

            auto cmd = std::make_unique<CastCommand>(
                origin, destination, SpellId::SEAR, action.getPlayer(),
                casterMagicBefore, targetWasBurning);
            auto result = cmd->execute(*this);
            if (!result.has_value()) commandHistory.push_back(std::move(cmd));
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

// ---------------------------------------------------------------------------
// Unit / city mutation
// ---------------------------------------------------------------------------

void World::moveUnit(const Position& from, const Position& to) {
    auto unitId = getTileAt(from).removeUnit();
    if (!unitId.has_value()) throw std::logic_error("No unit at source position");
    getTileAt(to).placeUnit(unitId.value());
    units.at(unitId.value())->setMoved(true);
    notifyObservers(UnitMovedEvent{unitId.value(), from, to});
}

void World::addCity(const Position& pos, City city, int ownerIdx) {
    if (ownerIdx >= 0 && ownerIdx < (int)players.size())
        city.setOwner(&players[ownerIdx]);
    getTileAt(pos).setCity(std::move(city));
    cityPositions.push_back(pos);
}

void World::removeUnit(const Position& pos) {
    auto unitId = getTileAt(pos).removeUnit();
    if (unitId.has_value()) units.erase(unitId.value());
}

void World::addUnit(const Position& pos, const Unit unit) {
    if (!getTileAt(pos).isWalkable())
        throw std::invalid_argument("Cannot place unit on non-walkable tile");
    if (getTileAt(pos).hasUnit())
        throw std::invalid_argument("Tile already has a unit");
    UnitId id = unit.getId();
    units[id] = std::make_unique<Unit>(std::move(unit));
    getTileAt(pos).placeUnit(id);
}

// ---------------------------------------------------------------------------
// Training
// ---------------------------------------------------------------------------

std::optional<PlayerError> World::trainUnit(
    const Position& cityPos, UnitType type, const Player& player)
{
    const City* city = getCityAt(cityPos);
    if (!city) return PlayerError::INVALIDTARGET;
    if (!constructionSystem.cityHasBuilding(cityPos, BuildingType::BARRACK))
        return PlayerError::INVALIDTARGET;

    int cost = ResourceSystem::unitFoodCost(type);
    if (getAvailableCapacity(player.getId(), ResourceType::FOOD) < cost)
        return PlayerError::INSUFFICIENTRESOURCES;

    return trainingSystem.beginTraining(cityPos, type, player);
}

std::optional<PlayerError> World::issueTrainCommand(
    const Position& cityPos, UnitType type, const Player& player)
{
    auto cmd = std::make_unique<TrainCommand>(cityPos, type, player);
    auto result = cmd->execute(*this);
    if (!result.has_value()) commandHistory.push_back(std::move(cmd));
    return result;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

std::optional<PlayerError> World::issueConstructCommand(
    const Position& tilePos, BuildingType type, const Player& player)
{
    auto cmd = std::make_unique<ConstructCommand>(tilePos, type, player);
    auto result = cmd->execute(*this);
    if (!result.has_value()) commandHistory.push_back(std::move(cmd));
    return result;
}

// ---------------------------------------------------------------------------
// Spirit / blessing
// ---------------------------------------------------------------------------

std::optional<PlayerError> World::completePray(
    Position unitPos, int blessingIndex, const Player& player)
{
    auto result = spiritSystem.completePray(unitPos, blessingIndex, player);
    if (!result.has_value())
        blessingSystem.refreshAllUnitsForPlayer(player.getId());
    return result;
}

// ---------------------------------------------------------------------------
// Combat
// ---------------------------------------------------------------------------

int World::calcHitChance(const Unit& attacker, const Unit& defender) {
    int pct = 80 + (attacker.getPrecision() - defender.getAgility()) * 2;
    return std::max(25, std::min(95, pct));
}

void World::battle(const Position& attackerPos, const Position& defenderPos) {
    Unit* attacker = getUnitAt(attackerPos);
    Unit* defender = getUnitAt(defenderPos);
    if (!attacker || !defender)
        throw std::logic_error("Battle requires units at both positions");

    // Apply terrain / flank / encirclement modifiers to attack damage.
    CombatContext ctx = battleSystem.computeCombatContext(attackerPos, defenderPos);
    int dmg = std::max(1, attacker->computeDamageAgainst(*defender) + ctx.netModifier());
    defender->lowerHP(dmg);
    attacker->setAttacked(true);
    notifyObservers(DamageDealtEvent{defenderPos, dmg});

    // Retaliation: defender strikes back with its own contextual modifiers.
    if (defender->isAlive() && battleSystem.canAttack(defenderPos, attackerPos)) {
        CombatContext retCtx = battleSystem.computeCombatContext(defenderPos, attackerPos);
        int retDmg = std::max(1, defender->computeDamageAgainst(*attacker, false)
                                 + retCtx.netModifier());
        attacker->lowerHP(retDmg);
        notifyObservers(DamageDealtEvent{attackerPos, retDmg});
    }

    if (!defender->isAlive()) {
        auto did = getTileAt(defenderPos).removeUnit();
        if (did.has_value()) {
            notifyObservers(UnitDiedEvent{did.value(), defenderPos});
            units.erase(did.value());
        }
    }
    if (!attacker->isAlive()) {
        auto aid = getTileAt(attackerPos).removeUnit();
        if (aid.has_value()) {
            notifyObservers(UnitDiedEvent{aid.value(), attackerPos});
            units.erase(aid.value());
        }
    }
}

World::CombatForecast World::getCombatForecast(Position from, Position to) const {
    CombatForecast f{};
    if (!hasUnitAt(from) || !hasUnitAt(to)) return f;
    const Unit* attacker = getUnitAt(from);
    const Unit* defender = getUnitAt(to);
    if (!attacker || !defender || attacker->sameOwner(*defender)) return f;

    f.attackerCanAct  = attacker->canAttack();
    f.inRange         = battleSystem.canAttack(from, to);
    f.defenderHpBefore = defender->getHealth();
    f.attackerHpBefore = attacker->getHealth();
    f.attackHitChance      = calcHitChance(*attacker, *defender);
    f.retaliationHitChance = calcHitChance(*defender, *attacker);

    // Situational context (baked into damage, also stored for UI display).
    CombatContext ctx = battleSystem.computeCombatContext(from, to);
    f.terrainReduction = ctx.terrainReduction;
    f.flankBonus       = ctx.flankBonus;
    f.encircleBonus    = ctx.encircleBonus;
    f.isFlank          = ctx.isFlank;
    f.isEncircled      = ctx.isEncircled;
    f.encirclingCount  = ctx.encirclingCount;

    if (f.attackerCanAct && f.inRange) {
        int rawDmg = attacker->computeDamageAgainst(*defender) + ctx.netModifier();
        rawDmg += blessingSystem.extraAttackDamage(*attacker, from, *defender, to);
        rawDmg -= blessingSystem.incomingDamageReduction(*defender);
        f.damage = std::max(1, rawDmg);

        // MARTIAL_ENDURE: lethal blow is blocked, defender survives at 1 HP
        bool endureWouldProc = (defender->getHealth() <= f.damage) && defender->hasEndure();
        f.defenderHpAfter = endureWouldProc ? 1 : std::max(0, defender->getHealth() - f.damage);
        f.lethal          = (!endureWouldProc && f.defenderHpAfter == 0);

        bool canRetaliate = !f.lethal && battleSystem.canAttack(to, from);
        if (canRetaliate) {
            CombatContext retCtx = battleSystem.computeCombatContext(to, from);
            int retDmg = defender->computeDamageAgainst(*attacker, false) + retCtx.netModifier();
            retDmg += blessingSystem.extraAttackDamage(*defender, to, *attacker, from);
            retDmg -= blessingSystem.incomingDamageReduction(*attacker);
            f.retaliation     = std::max(1, retDmg);
            bool atkEndure    = (attacker->getHealth() <= f.retaliation) && attacker->hasEndure();
            f.attackerHpAfter = atkEndure ? 1 : std::max(0, attacker->getHealth() - f.retaliation);
            f.attackerDies    = (!atkEndure && f.attackerHpAfter == 0);
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

// ---------------------------------------------------------------------------
// Visibility
// ---------------------------------------------------------------------------

std::unordered_set<Position> World::getVisiblePositions(int playerId) const {
    std::unordered_set<Position> visible;

    for (int r = 0; r < Game::HEIGHT; ++r) {
        for (int c = 0; c < Game::WIDTH; ++c) {
            const Tile& tile = grid[r][c];
            if (!tile.hasUnit()) continue;
            const Unit* unit = getUnitAt(Position(r, c));
            if (!unit || unit->getOwner().getId() != playerId) continue;
            int sight = unit->getSightRange();
            for (int dr = -sight; dr <= sight; ++dr)
                for (int dc = -sight; dc <= sight; ++dc) {
                    if (std::max(std::abs(dr), std::abs(dc)) > sight) continue;
                    int nr = r + dr, nc = c + dc;
                    if (nr >= 0 && nr < Game::HEIGHT && nc >= 0 && nc < Game::WIDTH)
                        visible.insert(Position(nr, nc));
                }
        }
    }

    for (const auto& cpos : cityPositions) {
        const City* city = getCityAt(cpos);
        if (!city || !city->hasOwner() || city->getOwner().getId() != playerId) continue;
        int rv = city->getBorderRadius();
        for (int dr = -rv; dr <= rv; ++dr)
            for (int dc = -rv; dc <= rv; ++dc) {
                if (std::max(std::abs(dr), std::abs(dc)) > rv) continue;
                int nr = cpos.row() + dr, nc = cpos.col() + dc;
                if (nr >= 0 && nr < Game::HEIGHT && nc >= 0 && nc < Game::WIDTH)
                    visible.insert(Position(nr, nc));
            }
    }

    return visible;
}

// ---------------------------------------------------------------------------
// WorldFactory
// ---------------------------------------------------------------------------

World WorldFactory::create(WorldLayout layout, std::vector<Player> players) {
    World world(players);

    switch (layout) {
        case WorldLayout::BASIC: {
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

            // River — diagonal crossing
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

            // Fringe bands
            set(0, 11, Terrain::FOREST); set(1, 12, Terrain::FOREST); set(2, 13, Terrain::FOREST);
            set(11, 13, Terrain::FOREST); set(12, 12, Terrain::FOREST); set(13, 11, Terrain::FOREST);
            set(13, 3, Terrain::MOUNTAIN); set(12, 0, Terrain::MOUNTAIN); set(11, 1, Terrain::MOUNTAIN);
            set(0, 4, Terrain::RIVER); set(1, 1, Terrain::RIVER);
            set(10, 11, Terrain::RIVER); set(11, 10, Terrain::RIVER); set(12, 9, Terrain::RIVER);

            // Mountain near Ironhaven (within Chebyshev-2 border of city at 1,11)
            set(0, 12, Terrain::MOUNTAIN);

            // Units
            world.addUnit(Position(4, 3), UnitFactory::create(UnitType::WARRIOR, players[0]));
            world.addUnit(Position(5, 6), UnitFactory::create(UnitType::MAGE,    players[0]));
            world.addUnit(Position(8, 4), UnitFactory::create(UnitType::SCOUT,   players[1]));
            world.addUnit(Position(7, 8), UnitFactory::create(UnitType::RANGER,  players[1]));
            world.addUnit(Position(9, 6), UnitFactory::create(UnitType::CAVALRY, players[1]));

            // Cities
            {
                world.addCity(Position(1, 11), City("Ironhaven"), 0);
                world.getTileAt(Position(2, 11)).setTileBuilding(BuildingType::BARRACK);
                world.getTileAt(Position(0, 10)).setTileBuilding(BuildingType::FARM);
                world.getTileAt(Position(1, 10)).setTileBuilding(BuildingType::FARM);
            }
            {
                world.addCity(Position(12, 2), City("Stonekeep"), 1);
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

    // Assign each tile a randomized resource value — this shapes building placement strategy.
    // Distribution: 25% LOW, 50% MEDIUM, 25% HIGH.
    {
        std::mt19937 rng(std::random_device{}());
        std::discrete_distribution<int> dist({25, 50, 25});
        for (int r = 0; r < Game::HEIGHT; ++r)
            for (int c = 0; c < Game::WIDTH; ++c)
                world.getTileAt(Position(r, c)).setTileResourceValue(
                    static_cast<TileResourceValue>(dist(rng)));
    }

    return world;
}
