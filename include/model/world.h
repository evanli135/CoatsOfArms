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

    void startGame();

private:
    vector<vector<Tile>> grid;
    vector<Player> players;

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
