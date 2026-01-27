#include <map>
#include <player.h>
#include <stdexcept>

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
    Unit(Player player) : player(player), health(0), maxHealth(0), dmg(0), mov(0), range(0) {}
    
    Unit(Player player, int maxHealth, int dmg, int mov, int range) 
        : 
        player(player),
        health(maxHealth)
        , maxHealth(maxHealth)
        , dmg(dmg)
        , mov(mov)
        , range(range) {}

    bool isAlive() const { return health > 0; }
    
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    int getDamage() const { return dmg; }
    int getMovement() const { return mov; }
    int getRange() const { return range; }
    const Player& getPlayer() const { return player; }

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
    Player player;
    int health;
    int maxHealth;
    int dmg;
    int mov;
    int range;
};


enum class UnitType {
    Warrior,
    Archer
};

class UnitFactory {
public:
    static Unit create(UnitType type) {
        int id = nextId++;
        switch (type) {
            case UnitType::Warrior:
                return createWarrior(id);
            case UnitType::Archer:
                return Unit(id, 75, 15, 2, 3);
            default:
                throw std::invalid_argument("Unknown UnitType");
        }
    }

private:
    static int nextId;
    
    static Unit createWarrior(int id) {
        return Unit(id, 100, 20, 2, 1);
    }

    static Unit createArcher(int id) {
        return Unit(id, 75, 15, 2, 3);
    }
};
