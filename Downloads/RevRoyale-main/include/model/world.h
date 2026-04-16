#pragma once

#include <vector>
#include <memory>
#include <map>
#include <array>
#include <unordered_map>
#include <unordered_set>

#include "controller/observer.h"
#include "controller/command.h"
#include "controller/error.h"
#include "controller/keyboard.h"
#include "model/tile.h"
#include "model/player.h"
#include "model/resource_system.h"
#include "model/construction_system.h"
#include "model/spirit_system.h"
#include "model/magic.h"
#include "model/blessing_system.h"

using std::vector, std::unordered_map;

enum class GamePhase {
    PREGAME,
    MIDGAME,
    ENDGAME
};

// Forward-declare World so the system classes can hold a World& before
// the full World class definition.
class World;

// ---------------------------------------------------------------------------
// MovementSystem
// ---------------------------------------------------------------------------
class MovementSystem {
public:
    MovementSystem(World& world) : world(world) {}

    bool canMove(Position origin, Position destination) const;
    std::optional<PlayerError> move(Position origin, Position destination);
    vector<Position> getMovementSnapshot(Position origin) const;

    /** Returns the tile sequence of the shortest path from origin to destination,
     *  or empty if unreachable. */
    vector<Position> getPath(Position origin, Position destination) const;

private:
    World& world;

    vector<Position> getNeighbors(Position pos) const;
    bool canMoveThroughTile(Position origin, Position pos, Position destination) const;
    float stepCost(const Unit& unit, const Tile& tile) const;
    float shortestPath(Position origin, Position destination) const;
};

// ---------------------------------------------------------------------------
// CombatContext — situational modifiers applied on top of base damage.
// Computed once per strike by BattleSystem::computeCombatContext.
// ---------------------------------------------------------------------------
struct CombatContext {
    int  terrainReduction = 0;   // flat damage reduction from defender's terrain
    int  flankBonus       = 0;   // bonus damage: attacker + ally on opposite sides
    int  encircleBonus    = 0;   // bonus damage: defender surrounded by 3+ enemies

    bool isFlank          = false;
    bool isEncircled      = false;
    int  encirclingCount  = 0;   // total adjacent enemies (Chebyshev-1) around defender

    /** Net effect on outgoing damage: positive = more damage to defender. */
    int netModifier() const { return flankBonus + encircleBonus - terrainReduction; }
};

// ---------------------------------------------------------------------------
// BattleSystem
// ---------------------------------------------------------------------------
class BattleSystem {
public:
    BattleSystem(World& world) : world(world) {}

    bool canAttack(Position origin, Position destination) const;
    std::optional<PlayerError> battle(Position attackerPos, Position defenderPos);
    vector<Position> getAttackSnapshot(Position origin) const;

    /** Compute terrain / flank / encirclement modifiers for this strike. */
    CombatContext computeCombatContext(Position attackerPos,
                                       Position defenderPos) const;

private:
    World& world;

    int stepCost(Tile* tile, Unit unit) const;
    int shortestPath(Position origin, Position destination) const;
};

// ---------------------------------------------------------------------------
// TrainingSystem — manages the 2-turn unit production queue.
// ---------------------------------------------------------------------------
class TrainingSystem {
public:
    explicit TrainingSystem(World& world) : world(world) {}

    static constexpr int MAX_UNITS_PER_PLAYER = 5;

    std::optional<PlayerError> beginTraining(const Position& cityPos,
                                             UnitType        type,
                                             const Player&   player);
    void advanceTraining(int playerId);
    int  countUnitsForPlayer(int playerId) const;

private:
    World& world;
};

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------
class World {
    friend class MoveCommand;
    friend class AttackCommand;
    friend class TrainCommand;
    friend class ConstructCommand;
    friend class CastCommand;
    friend class TrainingSystem;
    friend class ResourceSystem;
    friend class ConstructionSystem;
    friend class SpiritSystem;
    friend class MagicSystem;
    friend class BlessingSystem;
    friend class BattleSystem;

public:
    World(std::vector<Player> players);
    ~World() = default;

    // BattleSystem/MovementSystem hold World& so the implicit move constructor
    // is deleted.  Define it explicitly so WorldFactory can return World by value.
    World(World&&) noexcept;
    World& operator=(World&&) = delete;
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    int getTurn() const { return turn; }

    Unit* getUnit(UnitId id) const { return units.at(id).get(); }

    Unit* getUnitAt(const Position& pos) const;
    const std::optional<Unit> getCopyAt(const Position& pos) const;

    const Tile& getTileAt(const Position& pos) const;
    Tile&       getTileAt(const Position& pos);

    bool hasUnitAt(const Position& from) const;
    bool canMove(const Position& from, const Position& to);
    bool canAttack(const Position& from, const Position& to);

    bool        hasCityAt(const Position& pos) const;
    const City* getCityAt(const Position& pos) const;

    const Player& getCurrentPlayer() const;
    void nextTurn();

    /** True when every unit belonging to the current player is exhausted. */
    bool allUnitsExhausted() const;

    void moveUnit(const Position& from, const Position& to);
    void battle(const Position& attackerPos, const Position& defenderPos);

    std::optional<PlayerError> applyControllerRequest(ControllerRequest action);

    void notifyObservers(const ModelEvent& event);
    void addObserver(ModelObserver* observer);

    /** Undoes the most recent command executed this turn. */
    void undoLastCommand();
    /** Clears the command history for this turn (called by nextTurn). */
    void clearCommandHistory();

    void addUnit(const Position& pos, const Unit unit);

    /** Place a city on the given tile. If ownerIdx >= 0, sets ownership. */
    void addCity(const Position& pos, City city, int ownerIdx = -1);

    /** Remove the unit at pos (no-op if empty). Used by TrainCommand::undo. */
    void removeUnit(const Position& pos);

    /** Train a unit at the city on cityPos. Called by TrainCommand::execute. */
    std::optional<PlayerError> trainUnit(const Position& cityPos,
                                         UnitType type,
                                         const Player& player);

    /** Create and execute a TrainCommand, adding it to history on success. */
    std::optional<PlayerError> issueTrainCommand(const Position& cityPos,
                                                  UnitType type,
                                                  const Player& player);

    /** Fielded + in-training unit count for the given player id. */
    int countUnitsForPlayer(int playerId) const {
        return trainingSystem.countUnitsForPlayer(playerId);
    }

    // ── Construction ──────────────────────────────────────────────────────

    /** Validates ownership and queues the building.  Delegates to ConstructionSystem. */
    std::optional<PlayerError> scheduleConstruction(const Position& tilePos,
                                                    BuildingType    type,
                                                    const Player&   player) {
        return constructionSystem.scheduleConstruction(tilePos, type, player);
    }

    /** Create and execute a ConstructCommand, adding it to history on success. */
    std::optional<PlayerError> issueConstructCommand(const Position& tilePos,
                                                      BuildingType    type,
                                                      const Player&   player);

    /** Remove the first queue entry for tilePos+type+owner.  Used by undo. */
    void cancelConstruction(const Position& tilePos, BuildingType type, int playerId) {
        constructionSystem.cancelEntry(tilePos, type, playerId);
    }

    const std::vector<ConstructionEntry>& getConstructionQueue() const {
        return constructionSystem.getQueue();
    }

    // ── Capacity economy ──────────────────────────────────────────────────

    int getTotalCapacity(int playerId, ResourceType rt) const {
        return resourceSystem.getTotalCapacity(playerId, rt);
    }
    int getUsedCapacity(int playerId, ResourceType rt) const {
        return resourceSystem.getUsedCapacity(playerId, rt);
    }
    int getAvailableCapacity(int playerId, ResourceType rt) const {
        return resourceSystem.getAvailableCapacity(playerId, rt);
    }

    // ── Territory queries (delegate to ConstructionSystem) ────────────────

    std::unordered_set<Position> getCityBorderTiles(Position cityPos) const {
        return constructionSystem.getCityBorderTiles(cityPos);
    }
    const City*  getCityForTile(Position pos) const {
        return constructionSystem.getCityForTile(pos);
    }
    Position getCityPosForTile(Position pos) const {
        return constructionSystem.getCityPosForTile(pos);
    }
    int  countBuildingsInCity(Position cityPos, BuildingType type) const {
        return constructionSystem.countBuildingsInCity(cityPos, type);
    }
    bool cityHasBuilding(Position cityPos, BuildingType type) const {
        return constructionSystem.cityHasBuilding(cityPos, type);
    }
    std::vector<Position> getBuildableTiles(int playerId) const {
        return constructionSystem.getBuildableTiles(playerId);
    }

    // ── Magic system (delegate to MagicSystem) ───────────────────────────

    /** True if the unit at casterPos belongs to player, is not exhausted,
     *  and has enough magic to cast the given spell. */
    bool canCast(Position casterPos, SpellId spell, const Player& player) const {
        return magicSystem.canCast(casterPos, spell, player);
    }

    /** Cast a spell from casterPos onto targetPos.
     *  Validates ownership, magic cost, and range; applies the spell effect. */
    std::optional<PlayerError> castSpell(Position      casterPos,
                                          Position      targetPos,
                                          SpellId       spell,
                                          const Player& player) {
        return magicSystem.castSpell(casterPos, targetPos, spell, player);
    }

    /** Returns enemy tile positions the unit at casterPos can target with the spell. */
    std::vector<Position> getCastablePositions(Position      casterPos,
                                                const Player& player,
                                                SpellId       spell) const {
        return magicSystem.getCastablePositions(casterPos, player, spell);
    }

    static const SpellDef& getSpellDef(SpellId spell) {
        return MagicSystem::getSpellDef(spell);
    }

    // ── Shrine / spirit system (delegate to SpiritSystem) ────────────────

    void addShrine(Position pos)           { spiritSystem.addShrine(pos); }
    bool hasShrineAt(Position pos) const   { return spiritSystem.hasShrineAt(pos); }

    const std::vector<Position>& getShrinePositions() const {
        return spiritSystem.getShrinePositions();
    }
    bool isAdjacentToShrine(Position unitPos) const {
        return spiritSystem.isAdjacentToShrine(unitPos);
    }

    std::array<Blessing, 3> preparePrayChoices(Position unitPos, const Player& player) {
        return spiritSystem.preparePrayChoices(unitPos, player);
    }
    std::optional<PlayerError> completePray(Position unitPos,
                                            int blessingIndex,
                                            const Player& player);
    void clearPendingPrayChoices(int playerId) {
        spiritSystem.clearPendingPrayChoices(playerId);
    }
    const std::vector<Blessing>& getPlayerBlessings(int playerId) const {
        return spiritSystem.getPlayerBlessings(playerId);
    }
    std::optional<std::array<Blessing, 3>> getPendingPrayChoices(int playerId) const {
        return spiritSystem.getPendingPrayChoices(playerId);
    }

    void startGame();

    /** Returns the set of tile positions visible to the given player this turn. */
    std::unordered_set<Position> getVisiblePositions(int playerId) const;

    /** Returns all tiles the unit at `origin` can legally move to this turn. */
    std::vector<Position> getMovementSnapshot(Position origin) const {
        return movementSystem.getMovementSnapshot(origin);
    }

    /** Returns the tile sequence of the shortest path from origin to destination. */
    std::vector<Position> getPath(Position from, Position to) const {
        return movementSystem.getPath(from, to);
    }

    /** Returns all enemy tiles the unit at `origin` can legally attack this turn. */
    std::vector<Position> getAttackSnapshot(Position origin) const {
        return battleSystem.getAttackSnapshot(origin);
    }

    /** Read-only preview of a potential attack — no state is modified. */
    struct CombatForecast {
        int  damage;
        int  defenderHpBefore;
        int  defenderHpAfter;
        bool lethal;

        int  retaliation;
        int  attackerHpBefore;
        int  attackerHpAfter;
        bool attackerDies;

        bool attackerCanAct;
        bool inRange;

        int  attackHitChance;
        int  retaliationHitChance;

        // ── Situational modifiers (informational, already baked into damage) ──
        int  terrainReduction = 0;   // defender terrain reduces incoming damage
        int  flankBonus       = 0;   // attacker flanked the defender
        int  encircleBonus    = 0;   // defender is encircled by multiple enemies
        bool isFlank          = false;
        bool isEncircled      = false;
        int  encirclingCount  = 0;
    };

    CombatForecast getCombatForecast(Position from, Position to) const;

private:
    vector<vector<Tile>>                          grid;
    vector<Player>                                players;
    vector<Position>                              cityPositions;
    unordered_map<UnitId, std::unique_ptr<Unit>>  units;

    int       currentPlayerIndex = 0;
    GamePhase phase              = GamePhase::PREGAME;
    int       turn               = 0;

    vector<ModelObserver*>                        observers;

    BattleSystem       battleSystem;
    MovementSystem     movementSystem;
    TrainingSystem     trainingSystem;
    ResourceSystem     resourceSystem;
    ConstructionSystem constructionSystem;
    SpiritSystem       spiritSystem;
    MagicSystem        magicSystem;
    BlessingSystem     blessingSystem;

    std::vector<std::unique_ptr<GameCommand>> commandHistory;

    static int calcHitChance(const Unit& attacker, const Unit& defender);
};


enum class Direction { UP, DOWN, LEFT, RIGHT };

int stepCost(const Unit& unit, const Tile& tile);
int pathCost(const Position& origin, const Position& destination);

enum class WorldLayout { BASIC, EMPTY };

class WorldFactory {
public:
    static World create(WorldLayout layout, std::vector<Player> players);

private:
    static World createBasicWorld(std::vector<Player> players);
};
