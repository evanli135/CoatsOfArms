#pragma once

#include <vector>
#include <memory>

#include "model/spirit.h"
#include "controller/observer.h"
#include "controller/command.h"
#include "model/tile.h"
#include "controller/error.h"
#include "model/player.h"
#include "controller/keyboard.h"

using std::vector, std::unordered_map;

enum class GamePhase {
    PREGAME,
    MIDGAME,
    ENDGAME
};

// Forward-declare World so MovementSystem/BattleSystem can hold a World& before
// the full World class definition.
class World;

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

    bool canMoveThroughTile(Position origin, Position pos, Position desination) const;

    float stepCost(const Unit& unit, const Tile& tile) const;
    float shortestPath(Position origin, Position destination) const;
};

class BattleSystem {
public:
    BattleSystem(World& world) : world(world) {}

    bool canAttack(Position origin, Position destination) const;

    std::optional<PlayerError> battle(Position attackerPos, Position defenderPos);
    vector<Position> getAttackSnapshot(Position origin) const;

private:
    World& world;

    // Cost for the unit to cross this tile
    int stepCost(Tile* tile, Unit unit) const;
    int shortestPath(Position origin, Position destination) const;
};

class World {
    friend class MoveCommand;
    friend class AttackCommand;
    friend class TrainCommand;

public:
    World(std::vector<Player> players);
    ~World() = default;

    // BattleSystem/MovementSystem hold World& so the implicit move constructor
    // is deleted. Define it explicitly so WorldFactory can return World by value.
    World(World&&) noexcept;
    World& operator=(World&&) = delete;
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    int getTurn() const { return turn; }

    Unit* getUnit(UnitId id) const { return units.at(id).get(); }

    Unit* getUnitAt(const Position& pos) const;
    const std::optional<Unit> getCopyAt(const Position& pos) const;

    const Tile& getTileAt(const Position& pos) const;
    Tile& getTileAt(const Position& pos);

    bool hasUnitAt(const Position& from) const;
    bool canMove(const Position& from, const Position& to);
    bool canAttack(const Position& from, const Position& to);

    bool hasCityAt(const Position& pos) const;
    const City* getCityAt(const Position& pos) const;

    const Player& getCurrentPlayer() const;
    void nextTurn();

    /** True when every unit belonging to the current player is exhausted. */
    bool allUnitsExhausted() const;

    void moveUnit(const Position& from, const Position& to);
    void battle(const Position& attackerPos, const Position& defenderPos);

    void addToConstructionQueue(const Position& pos, BuildingType buildingType);

    std::optional<PlayerError> applyControllerRequest(ControllerRequest action);

    void notifyObservers(const ModelEvent& event);
    void addObserver(ModelObserver* observer);

    /**
     * Undoes the most recent command executed this turn.
     * No-op if the history is empty.
     */
    void undoLastCommand();

    /**
     * Clears the command history for this turn.
     * Called automatically by nextTurn().
     */
    void clearCommandHistory();

    void addUnit(const Position& pos, const Unit unit);

    /** Place a city on the given tile. If ownerIdx >= 0, sets ownership to that player. */
    void addCity(const Position& pos, City city, int ownerIdx = -1);

    /** Remove the unit at pos (no-op if empty). Used by TrainCommand::undo. */
    void removeUnit(const Position& pos);

    /** Train a unit at the city on cityPos. Called by TrainCommand::execute. */
    std::optional<PlayerError> trainUnit(const Position& cityPos, UnitType type, const Player& player);

    /** Create and execute a TrainCommand, adding it to history on success. */
    std::optional<PlayerError> issueTrainCommand(const Position& cityPos, UnitType type, const Player& player);

    void startGame();

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
        int  damage;            // damage the attacker deals to the defender
        int  defenderHpBefore;
        int  defenderHpAfter;
        bool lethal;            // attack kills the defender

        int  retaliation;       // damage the defender retaliates (0 if defender dies)
        int  attackerHpBefore;
        int  attackerHpAfter;
        bool attackerDies;      // retaliation kills the attacker

        bool attackerCanAct;    // false when attacker has already attacked
        bool inRange;           // false when defender is out of attack range
    };

    /** Returns a forecast for attacker at `from` hitting defender at `to`.
     *  Always returns a value; check attackerCanAct and inRange for validity. */
    CombatForecast getCombatForecast(Position from, Position to) const;

private:
    vector<vector<Tile>> grid;
    vector<Player> players;
    vector<Position> cityPositions;

    unordered_map<UnitId, std::unique_ptr<Unit>> units = unordered_map<UnitId, std::unique_ptr<Unit>>();

    int currentPlayerIndex = 0;

    GamePhase phase = GamePhase::PREGAME;

    int turn = 0;

    vector<ModelObserver*> observers = vector<ModelObserver*>();

    BattleSystem   battleSystem;
    MovementSystem movementSystem;

    std::vector<std::unique_ptr<GameCommand>> commandHistory;
};


/**
 * Foundry = Basic Construction Building (builder hut)
 */
// class ConstructionLogic { ... };
// class ArmyLogic { ... };
// class SpiritLogic { ... };


enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

int stepCost(const Unit& unit, const Tile& tile);
int pathCost(const Position& origin, const Position& destination);

enum class WorldLayout {
    BASIC,
    EMPTY
};

class WorldFactory {
public:
    static World create(WorldLayout layout, std::vector<Player> players);

private:
    static World createBasicWorld(std::vector<Player> players);
};
