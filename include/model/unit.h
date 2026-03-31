#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <stdexcept>
#include "model/player.h"

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
// Flyweight — UnitTemplate
//
// Holds the immutable, per-type stats that all units of a given type share.
// Unit stores a const pointer to the appropriate static template instance
// in UnitFactory, so stats are never duplicated per-instance.
// -------------------------

struct UnitTemplate {
    int maxHealth;
    int dmg;
    int mov;
    int range;
};


// -------------------------
// DamageModifier — Chain of Responsibility
//
// Each active status on a unit contributes one modifier to a chain.
// BattleSystem calls Unit::computeDamageAgainst(), which walks the chain
// left-to-right, passing the accumulated damage through each modifier.
// New status effects only require a new DamageModifier subclass.
// -------------------------

class Unit;  // forward declaration needed by DamageModifier::modify

class DamageModifier {
public:
    virtual ~DamageModifier() = default;

    /**
     * Returns a modified damage value.
     * @param dmg       Incoming accumulated damage.
     * @param attacker  The attacking unit.
     * @param defender  The defending unit.
     */
    virtual int modify(int dmg, const Unit& attacker, const Unit& defender) const = 0;
};

// BURN: deals 10 bonus flat damage on top of base.
class BurnModifier : public DamageModifier {
public:
    int modify(int dmg, const Unit& /*attacker*/, const Unit& /*defender*/) const override {
        return dmg + 10;
    }
};

// STRENGTH: increases damage by 50%.
class StrengthModifier : public DamageModifier {
public:
    int modify(int dmg, const Unit& /*attacker*/, const Unit& /*defender*/) const override {
        return dmg + dmg / 2;
    }
};


// -------------------------
// Unit
// -------------------------

class Unit {
public:
    Unit(UnitId id, UnitType type, Player player, const UnitTemplate* tmpl)
        : id(id), type(type), player(player),
          tmpl(tmpl),
          health(tmpl ? tmpl->maxHealth : 0),
          moved(false), attacked(false) {}

    bool isAlive()   const { return health > 0; }
    bool canMove()   const { return !moved; }
    bool canAttack() const { return !attacked; }

    UnitId          getId()        const { return id; }
    int             getHealth()    const { return health; }
    int             getMaxHealth() const { return tmpl->maxHealth; }
    int             getDamage()    const { return tmpl->dmg; }
    int             getMovement()  const { return tmpl->mov; }
    int             getRange()     const { return tmpl->range; }
    const Player&   getOwner()     const { return player; }
    UnitType        getType()      const { return type; }

    /**
     * Walks the modifier chain and returns the effective damage against
     * a specific defender. Starts from the base template damage.
     */
    int computeDamageAgainst(const Unit& defender) const {
        int dmg = tmpl->dmg;
        for (const auto& mod : modifiers) {
            dmg = mod->modify(dmg, *this, defender);
        }
        return dmg;
    }

    void setMoved(bool flag) { moved = flag; }

    /**
     * Sets health directly — used by command undo to restore pre-combat HP.
     * Clamped to [0, maxHealth].
     */
    void setHealth(int h) { health = std::max(0, std::min(tmpl->maxHealth, h)); }

    /** Appends a damage modifier for the given status effect. */
    void applyStatus(Status status) {
        switch (status) {
            case Status::BURN:
                modifiers.push_back(std::make_shared<BurnModifier>());
                break;
            case Status::STRENGTH:
                modifiers.push_back(std::make_shared<StrengthModifier>());
                break;
        }
    }

    void clearStatuses() { modifiers.clear(); }

    bool sameOwner(const Player& p) const { return player.getId() == p.getId(); }
    bool sameOwner(const Unit& u)   const { return sameOwner(u.getOwner()); }

    void raiseHP(int amount) {
        if (amount < 0) throw std::invalid_argument("Amount must be non-negative");
        health = std::min(tmpl->maxHealth, health + amount);
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

    const UnitTemplate* tmpl;  // shared flyweight — points to static data in UnitFactory
    int  health;
    bool moved;
    bool attacked;

    std::vector<std::shared_ptr<DamageModifier>> modifiers;
};


// -------------------------
// UnitFactory
// -------------------------

class UnitFactory {
public:
    static Unit create(UnitType type, Player player) {
        UnitId id(nextId++);
        return Unit(id, type, player, getTemplate(type));
    }

    static const UnitTemplate* getTemplate(UnitType type) {
        switch (type) {
            case UnitType::WARRIOR:  return &WARRIOR_TMPL;
            case UnitType::SCOUT:    return &SCOUT_TMPL;
            case UnitType::RANGER:   return &RANGER_TMPL;
            case UnitType::CAVALRY:  return &CAVALRY_TMPL;
            case UnitType::MAGE:     return &MAGE_TMPL;
            default: throw std::invalid_argument("Unknown UnitType");
        }
    }

private:
    inline static uint32_t nextId = 0;

    static constexpr UnitTemplate WARRIOR_TMPL = { 100, 20, 2, 1 };
    static constexpr UnitTemplate SCOUT_TMPL   = {  50, 10, 3, 1 };
    static constexpr UnitTemplate RANGER_TMPL  = {  75, 15, 2, 2 };
    static constexpr UnitTemplate CAVALRY_TMPL = {  80, 25, 4, 1 };
    static constexpr UnitTemplate MAGE_TMPL    = {  60, 30, 2, 2 };
};
