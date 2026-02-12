#pragma once

#include <map>
#include <model/player.h>
#include <stdexcept>

enum class UnitType {
    Warrior,
    Archer
};

/**
 * Status Effects 
 */
enum class Status {
    BURN,
    STRENGTH
};

inline std::map<Status, bool> is_curse = {
    {Status::BURN, true},
    {Status::STRENGTH, false}
};




/**
 * TO implement:
 * Status effects
 * Stats
 */
class Unit {
public:
    Unit(UnitType type, Player player) : type(type), player(player), health(0), maxHealth(0), dmg(0), mov(0), range(0) {}
    
    Unit(UnitType type, Player player, int maxHealth, int dmg, int mov, int range) 
        : 
        type(type),
        player(player),
        health(maxHealth),
        maxHealth(maxHealth),
        dmg(dmg),
        mov(mov), 
        range(range), 
        moved(false) {}

    bool isAlive() const { return health > 0; }
    
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    int getDamage() const { return dmg; }
    int getMovement() const { return mov; }
    int getRange() const { return range; }
    const Player& getOwner() const { return player; }
    UnitType getType() const { return type; }

    bool canMove() const { return !moved; }

    void setMoved(bool flag) { moved = flag; }

    bool sameOwner(Player& player)  const {return this->player.getId() == player.getId(); }
    bool sameOwner(Player player) const {return this->player.getId() == player.getId(); }

    void raiseHP(int amount) {
        if (amount < 0) throw std::invalid_argument("Amount must be non-negative");
        health = std::min(maxHealth, health + amount);
    }

    void lowerHP(int amount) {
        if (!isAlive()) throw std::logic_error("Cannot lower HP of a dead unit");
        if (amount < 0) throw std::invalid_argument("Amount must be non-negative");
        health = std::max(0, health - amount);
    }

private:
    int health;
    int maxHealth;
    int dmg;
    int mov;
    int range;
    bool moved;
    UnitType type;
    Player player;
};

class UnitFactory {
public:
    static Unit create(UnitType type, Player player) {
        // int id = nextId++;
        switch (type) {
            case UnitType::Warrior:
                return createWarrior(player);
            case UnitType::Archer:
                return createArcher(player);
            default:
                throw std::invalid_argument("Unknown UnitType");
        }
    }

private:
    // static int nextId;
    
    static Unit createWarrior(Player player) {
        return Unit(UnitType::Warrior, player, 100, 20, 2, 1);
    }

    static Unit createArcher(Player player) {
        return Unit(UnitType::Archer, player, 75, 15, 2, 3);
    }
};
