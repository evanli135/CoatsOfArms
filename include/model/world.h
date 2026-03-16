#pragma once

#include <vector>
#include <memory>

#include "model/spirit.h"
#include "controller/observer.h"
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

class MovementSystem;
class BattleSystem;

class World {
public:
    World(std::vector<Player> players);
    ~World() = default;

    int getTurn() const { return turn; }

    Unit* getUnit(const UnitId id) const { return units.at(id).get(); }
    Unit* getUnit(UnitId id) const { return units.at(id).get(); };

    Unit* getUnitAt(const Position& pos) const;
    const std::optional<Unit> getCopyAt(const Position& pos) const;

    const Tile& getTileAt(const Position& pos) const;
    Tile& getTileAt(const Position& pos);

    bool hasUnitAt(const Position& from) const;
    bool canMove(const Position& from, const Position& to);
    bool canAttack(const Position& from, const Position& to);

    const Player& getCurrentPlayer() const;
    void nextTurn();

    void moveUnit(const Position& from, const Position& to);
    void battle(const Position& attackerPos, const Position& defenderPos);

    std::optional<PlayerError> applyControllerRequest(ControllerRequest action);

    void notifyObservers(ModelEvent event);
    void addObserver(ModelObserver* observer);

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

    BattleSystem battleSystem{*this};
    MovementSystem movementSystem{*this};
};

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
    float shortestPath(Position origin, Position destination);
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
    int stepCost(Tile* tile, Unit unit);
    int shortestPath(Position origin, Position destination);
};


/**
 * Foundry = Basic Construction Building (builder hut)
 */
// class ConstructionLogic {
// public:
//     void tick();
//     void assignFoundry(Position position, BuildingType buildingType);
//     void reassignFoundry(Position newPosition, BuildingType newBuildingType, Position oldPosition, BuildingType oldBuildingType);

//     int numFreeFoundries() const { return freeFoundries; }
//     bool hasFreeFoundries() const { return freeFoundries > 0; }


// private:
//     unordered_map<Position, unordered_map<BuildingType, int>> foundryJobs;

//     int totalFoundries;
//     int freeFoundries;

//     void construct(Grid& grid, Position position, BuildingType buildingType);
// };

// class ArmyLogic {
// public:
//     void tick();

//     void beginTraining(UnitType unitType, Position position, int barracks);
//     void addBarrack(Position position);
//     void haltTraining(Position position);

// private:
//     unordered_map<Position, unordered_map<UnitType, int>> barrackJobs;

//     void deploy(Grid& grid, Position position, UnitType buildingType);
// };

// class SpiritLogic {
// public:
//     void tick();

//     void beginDiscovering(Blessing blessing);
//     void haltDiscovering(Blessing blessing);

// private:
//     unordered_map<Blessing, int> shrineJobs;

//     void discover();
// };


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

