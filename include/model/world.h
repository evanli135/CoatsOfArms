#pragma once

#include <vector>

#include "controller/observer.h"
#include "model/tile.h"
#include "controller/error.h"
#include "model/player.h"
#include "controller/keyboard.h"

enum class GamePhase {
    PREGAME,
    MIDGAME,
    ENDGAME
};

class World {
public:
    World(std::vector<Player> players);
    ~World() = default;

    const Unit* getUnitAt(const Position& pos) const;
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

    void addUnit(const Position& pos, const Unit& unit);

    void startGame();

private:
    std::vector<std::vector<Tile>> map;
    std::vector<Player> players;

    std::map<int, std::vector<Unit*>> units;

    int currentPlayerIndex = 0;

    GamePhase phase = GamePhase::PREGAME; 

    int turn = 0;

    std::vector<ModelObserver*> observers = std::vector<ModelObserver*>();
};

namespace Logic {
    enum Direction {
        UP,
        DOWN,
        LEFT,
        RIGHT
    };

    int stepCost(const Unit& unit, const Tile& tile);
    int pathCost(const Position& origin, const Position& destination);
}

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