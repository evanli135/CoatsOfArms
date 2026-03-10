#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <model/player.h>
#include <stdexcept>

// -------------------------
// UnitId
// -------------------------

class UnitId {
public:
    explicit UnitId(uint32_t value) : value(value) {}

    bool operator==(const UnitId& o) const { return value == o.value; }
    bool operator!=(const UnitId& o) const { return value != o.value; }
    bool operator<(const UnitId& o)  const { return value < o.value;  }

    uint32_t raw() const { return value; }

private:
    uint32_t value;
};

// Hash support so UnitId works as an unordered_map key
namespace std {
    template<>
    struct hash<UnitId> {
        size_t operator()(const UnitId& id) const noexcept {
            return std::hash<uint32_t>{}(id.raw());
        }
    };
}


// -------------------------
// Enums
// -------------------------

enum class UnitType {
    WARRIOR,
    SCOUT,
    RANGER,
    CAVALRY,
    MAGE
};

enum class Status {
    BURN,
    STRENGTH
};

inline std::map<Status, bool> is_curse = {
    {Status::BURN, true},
    {Status::STRENGTH, false}
};


// -------------------------
// Unit
// -------------------------

class Unit {
public:
    Unit(UnitId id, UnitType type, Player player)
        : id(id), type(type), player(player),
          health(0), maxHealth(0), dmg(0), mov(0), range(0), moved(false) {}

    Unit(UnitId id, UnitType type, Player player, int maxHealth, int dmg, int mov, int range)
        : id(id), type(type), player(player),
          health(maxHealth), maxHealth(maxHealth),
          dmg(dmg), mov(mov), range(range), moved(false) {}

    bool isAlive()   const { return health > 0; }

    bool canMove()   const { return !moved; }
    bool canAttack() const { return !attacked; }

    UnitId          getId()        const { return id; }
    int             getHealth()    const { return health; }
    int             getMaxHealth() const { return maxHealth; }
    int             getDamage()    const { return dmg; }
    int             getMovement()  const { return mov; }
    int             getRange()     const { return range; }
    const Player&   getOwner()     const { return player; }
    UnitType        getType()      const { return type; }

    void setMoved(bool flag) { moved = flag; }

    bool sameOwner(const Player& p) const { return player.getId() == p.getId(); }
    bool sameOwner(const Unit& u) const { return sameOwner(u.getOwner()); }

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
    UnitId   id;
    UnitType type;
    Player   player;

    int  health;
    int  maxHealth;
    int  dmg;
    int  mov;
    int  range;
    bool moved;
    bool attacked;
};


// -------------------------
// UnitFactory
// -------------------------

class UnitFactory {
public:
    static Unit create(UnitType type, Player player) {
        UnitId id(nextId++);
        switch (type) {
            case UnitType::WARRIOR:  return createWarrior(id, player);
            case UnitType::SCOUT:    return createScout(id, player);
            case UnitType::RANGER:   return createRanger(id, player);
            case UnitType::CAVALRY:  return createCavalry(id, player);
            case UnitType::MAGE:     return createMage(id, player);
            default: throw std::invalid_argument("Unknown UnitType");
        }
    }

private:
    inline static uint32_t nextId = 0;

    static Unit createWarrior(UnitId id, Player p) { return Unit(id, UnitType::WARRIOR, p, 100, 20, 2, 1); }
    static Unit createRanger (UnitId id, Player p) { return Unit(id, UnitType::RANGER,  p,  75, 15, 2, 2); }
    static Unit createScout  (UnitId id, Player p) { return Unit(id, UnitType::SCOUT,   p,  50, 10, 3, 1); }
    static Unit createCavalry(UnitId id, Player p) { return Unit(id, UnitType::CAVALRY, p,  80, 25, 4, 1); }
    static Unit createMage   (UnitId id, Player p) { return Unit(id, UnitType::MAGE,    p,  60, 30, 2, 2); }
};