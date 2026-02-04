#pragma once

#include <vector>

#include "observer.h"
#include "tile.h"
#include "error.h"
#include "player.h"

class World {
public:
    World();
    ~World() = default;

    const std::optional<const Unit>& getUnitAt(const Position& pos) const;
    std::optional<Unit>& getUnitAt(const Position& pos);

    const Tile& getTileAt(const Position& pos) const;
    Tile& getTileAt(const Position& pos);

    bool hasUnitAt(const Position& from) const;
    bool canMove(const Position& from, const Position& to);
    bool canAttack(const Position& from, const Position& to);

    const Player& getCurrentPlayer() const;
    void nextTurn();

    std::optional<PlayerError> applyControllerRequest(ControllerRequest action);

private:
    std::vector<std::vector<Tile>> map;
    std::vector<Player> players;  // Note: can't have vector of references
    int currentPlayerIndex;

    int turn = 0;

    std::map<ModelEvent, ModelObserver*> observers;
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
    
    void moveUnit(const Position& from, const Position& to);
    void battle(const Position& attackerPos, const Position& defenderPos);
}
