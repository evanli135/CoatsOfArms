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

int World::calcHitChance(const Unit& attacker, const Unit& defender) {
    int pct = 65 + (attacker.getPrecision() - defender.getAgility()) * 3;
    return std::max(25, std::min(95, pct));
}

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
      units(std::move(other.units)),
      currentPlayerIndex(other.currentPlayerIndex),
      phase(other.phase),
      turn(other.turn),
      observers(std::move(other.observers)),
      commandHistory(std::move(other.commandHistory)),
      battleSystem(*this),       // re-bind to the new object
      movementSystem(*this),     // re-bind to the new object
      trainingSystem(*this)      // re-bind to the new object
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
            unit->setCharged(false);
        }
    }

    // Tick training queues and spawn any completed units for the new player.
    trainingSystem.advanceTraining(currentPlayerId);

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

        case (ControllerAction::CHG): {
            // Snap direction to the dominant cardinal axis
            int dr = destination.row() - origin.row();
            int dc = destination.col() - origin.col();
            if (dr == 0 && dc == 0) return PlayerError::INVALIDTARGET;
            if (std::abs(dr) >= std::abs(dc)) dc = 0; else dr = 0;
            dr = (dr > 0) ? 1 : (dr < 0) ? -1 : 0;
            dc = (dc > 0) ? 1 : (dc < 0) ? -1 : 0;

            const Unit* cavalry = getUnitAt(origin);
            if (!cavalry) return PlayerError::INVALIDTARGET;

            // Pre-compute path so ChargeCommand can store it for undo.
            auto path = previewChargeInDir(origin, dr, dc);

            int          enemyHpBefore = 0;
            std::optional<Unit> enemySnap;
            if (path.hitEnemy && hasUnitAt(path.enemyPos)) {
                const Unit* enemy = getUnitAt(path.enemyPos);
                if (enemy) {
                    enemyHpBefore = enemy->getHealth();
                    enemySnap     = *enemy;
                }
            }

            auto cmd = std::make_unique<ChargeCommand>(
                origin, destination, action.getPlayer(),
                path.finalPos, path.hitEnemy, path.enemyPos,
                enemyHpBefore, std::move(enemySnap));
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
    for (int r = 0; r < Game::HEIGHT; ++r) {
        for (int c = 0; c < Game::WIDTH; ++c) {
            Position pos(r, c);
            const Tile& tile = getTileAt(pos);
            if (!tile.hasUnit()) continue;
            const Unit* u = getUnit(tile.getUnit().value());
            if (!u || u->getOwner().getId() != pid) continue;
            if (u->isExhausted()) continue;      // attacked → done
            if (u->hasMoved()) {
                // Moved but not attacked — only active if there are reachable targets
                if (!battleSystem.getAttackSnapshot(pos).empty()) return false;
            } else {
                // Hasn't moved or attacked → still active
                return false;
            }
        }
    }
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
    return getDistance(from, to) <= attacker->getRange();
}

void World::battle(const Position& attackerPos, const Position& defenderPos) {
    Unit* attacker = getUnitAt(attackerPos);
    Unit* defender = getUnitAt(defenderPos);

    if (!attacker || !defender) {
        throw std::logic_error("Battle requires units at both positions");
    }

    retaliationPending_ = false;

    // ── Initial strike ──────────────────────────────────────────────────────
    bool hit = (std::rand() % 100) < calcHitChance(*attacker, *defender);
    attacker->setAttacked(true);

    if (hit) {
        int dmg = attacker->computeDamageAgainst(*defender, true);
        defender->lowerHP(dmg);
        notifyObservers(DamageDealtEvent{defenderPos, dmg, false});
    } else {
        notifyObservers(DamageDealtEvent{defenderPos, 0, true});
    }

    // ── Remove defender if killed — no retaliation possible ─────────────────
    if (!defender->isAlive()) {
        auto defenderId = getTileAt(defenderPos).removeUnit();
        if (defenderId.has_value()) {
            notifyObservers(UnitDiedEvent{defenderId.value(), defenderPos});
            units.erase(defenderId.value());
        }
        return;
    }

    // ── Queue retaliation for later (fired by executePendingRetaliation) ─────
    if (battleSystem.canAttack(defenderPos, attackerPos)) {
        retaliationPending_      = true;
        retaliatorPos_           = defenderPos;
        retaliationTargetPos_    = attackerPos;
    }
}

void World::executePendingRetaliation() {
    if (!retaliationPending_) return;
    retaliationPending_ = false;

    Unit* retaliator = getUnitAt(retaliatorPos_);
    Unit* target     = getUnitAt(retaliationTargetPos_);
    if (!retaliator || !target) return;

    // ── Retaliation strike ───────────────────────────────────────────────────
    bool retHit = (std::rand() % 100) < calcHitChance(*retaliator, *target);
    if (retHit) {
        int retDmg = retaliator->computeDamageAgainst(*target, false);
        target->lowerHP(retDmg);
        notifyObservers(DamageDealtEvent{retaliationTargetPos_, retDmg, false});
    } else {
        notifyObservers(DamageDealtEvent{retaliationTargetPos_, 0, true});
    }

    // ── Remove attacker if killed ────────────────────────────────────────────
    if (!target->isAlive()) {
        auto targetId = getTileAt(retaliationTargetPos_).removeUnit();
        if (targetId.has_value()) {
            notifyObservers(UnitDiedEvent{targetId.value(), retaliationTargetPos_});
            units.erase(targetId.value());
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
    f.attackHitChance  = calcHitChance(*attacker, *defender);

    if (f.attackerCanAct && f.inRange) {
        f.damage          = attacker->computeDamageAgainst(*defender, true);  // initiator
        f.defenderHpAfter = std::max(0, defender->getHealth() - f.damage);
        f.lethal          = (f.defenderHpAfter == 0);

        // Retaliation: defender hits back if it survives and attacker is in range.
        bool defenderCanRetaliate = !f.lethal && battleSystem.canAttack(to, from);
        if (defenderCanRetaliate) {
            f.retaliationHitChance = calcHitChance(*defender, *attacker);
            f.retaliation     = defender->computeDamageAgainst(*attacker, false); // not initiating
            f.attackerHpAfter = std::max(0, attacker->getHealth() - f.retaliation);
            f.attackerDies    = (f.attackerHpAfter == 0);
        } else {
            f.retaliationHitChance = 0;
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

            // --- Cities ---
            world.addCity(Position(1, 11),  City("Ironhaven", 4), 0);  // Player 1
            world.addCity(Position(12, 2), City("Stonekeep", 4), 1);  // Player 2
            break;
        }
        case WorldLayout::EMPTY:
            break;
        default:
            throw std::invalid_argument("Unknown WorldLayout");
    }

    return world;
}

// ---------------------------------------------------------------------------
// Cavalry Charge
// ---------------------------------------------------------------------------

World::ChargePath World::previewChargeInDir(const Position& from, int dr, int dc) const {
    ChargePath path;
    path.finalPos = from;
    if (dr == 0 && dc == 0) return path;

    const Unit* unit = getUnitAt(from);
    if (!unit) return path;

    Position current = from;
    for (int step = 0; step < 6; ++step) {
        int nr = current.row() + dr;
        int nc = current.col() + dc;
        if (nr < 0 || nr >= Game::HEIGHT || nc < 0 || nc >= Game::WIDTH) break;
        Position next(nr, nc);
        const Tile& nextTile = getTileAt(next);
        if (!nextTile.isWalkable()) break;
        if (nextTile.hasUnit()) {
            const Unit* occupant = getUnitAt(next);
            if (!occupant || occupant->sameOwner(*unit)) break;
            path.finalPos = current;
            path.hitEnemy = true;
            path.enemyPos = next;
            return path;
        }
        current = next;
    }
    path.finalPos = current;
    return path;
}

World::ChargePath World::previewCharge(const Position& from, const Position& dirTarget) const {
    int dr = (dirTarget.row() > from.row()) ? 1 : (dirTarget.row() < from.row()) ? -1 : 0;
    int dc = (dirTarget.col() > from.col()) ? 1 : (dirTarget.col() < from.col()) ? -1 : 0;
    return previewChargeInDir(from, dr, dc);
}

std::optional<PlayerError> World::executeCharge(const Position& from,
                                                  const ChargePath& path,
                                                  const Player& player) {
    Unit* unit = getUnitAt(from);
    if (!unit)                              return PlayerError::INVALIDTARGET;
    if (!unit->sameOwner(player))           return PlayerError::INVALIDTARGET;
    if (!unit->canCharge())                 return PlayerError::UNITCANTMOVE;
    if (unit->getType() != UnitType::CAVALRY) return PlayerError::NOTSUPPORTED;

    // Move cavalry to its final position (moveUnit sets moved = true)
    if (path.finalPos != from) {
        moveUnit(from, path.finalPos);
    } else {
        unit->setMoved(true);
    }

    // Re-fetch pointer after potential tile change
    Unit* cav = getUnitAt(path.finalPos);
    if (!cav) return PlayerError::INVALIDTARGET;
    cav->setCharged(true);

    // Strike the enemy if the charge collided with one
    if (path.hitEnemy) {
        Unit* enemy = getUnitAt(path.enemyPos);
        if (enemy) {
            int dmg = cav->computeChargeDamageAgainst(*enemy);
            enemy->lowerHP(dmg);
            cav->setAttacked(true);
            notifyObservers(DamageDealtEvent{path.enemyPos, dmg, false});

            if (!enemy->isAlive()) {
                auto enemyId = getTileAt(path.enemyPos).removeUnit();
                if (enemyId.has_value()) {
                    notifyObservers(UnitDiedEvent{enemyId.value(), path.enemyPos});
                    units.erase(enemyId.value());
                }
            }
        }
    }

    return std::nullopt;
}
