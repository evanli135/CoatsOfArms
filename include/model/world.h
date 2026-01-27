#pragma once

#include <vector>
#include "tile.h"
#include "player.h"

namespace Model {
class World {
public:
    World();
    ~World() = default;

    const std::optional<Unit>& getUnitAt(const Position& pos) const;
    std::optional<Unit>& getUnitAt(const Position& pos);

    const Tile& getTileAt(const Position& pos) const;
    Tile& getTileAt(const Position& pos);

    void moveUnit(const Position& from, const Position& to);
    bool canMove(const Position& from, const Position& to);
    void battle(const Position& attackerPos, const Position& defenderPos);

    const Player& getCurrentPlayer() const;
    void nextTurn();

private:
    std::vector<std::vector<Tile>> map;
    std::vector<Player> players;  // Note: can't have vector of references
    int currentPlayerIndex;
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
}