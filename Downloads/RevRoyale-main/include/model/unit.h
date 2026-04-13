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
    int mov;
    int range;
    int sight; // sight range — tiles visible around the unit
    // Bonus stats
    int str;   // strength   — flat physical attack bonus
    int spc;   // special    — flat non-physical (Mage) attack bonus
    int def;   // defense    — flat physical damage reduction
    int res;   // resistance — flat non-physical damage reduction
    int prec;  // precision  — +2% hit chance per point (base 80%)
    int agi;   // agility    — −2% hit chance per attacker point
    int ini;   // initiative — bonus damage only when this unit initiates
    int grd;   // guard      — flat reduction to any incoming attack
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
    bool canMove()   const { return !moved && !attacked; }
    bool canAttack() const { return !attacked; }

    UnitId          getId()        const { return id; }
    int             getHealth()    const { return health; }
    int             getMaxHealth() const { return tmpl->maxHealth; }
    int             getMovement()   const { return tmpl->mov; }
    int             getRange()      const { return tmpl->range; }
    int             getSightRange() const { return tmpl->sight; }
    int             getStrength()   const { return tmpl->str;  }
    int             getSpecial()    const { return tmpl->spc;  }
    int             getDefense()    const { return tmpl->def;  }
    int             getResistance() const { return tmpl->res;  }
    int             getPrecision()  const { return tmpl->prec; }
    int             getAgility()    const { return tmpl->agi;  }
    int             getInitiative() const { return tmpl->ini;  }
    int             getGuard()      const { return tmpl->grd;  }
    const Player&   getOwner()      const { return player; }
    UnitType        getType()       const { return type; }

    /**
     * Computes effective damage against defender.
     * isInitiator = true  when this unit starts the attack (applies initiative).
     * isInitiator = false for retaliations.
     * Physical damage uses strength/defense; Mage uses special/resistance.
     * Guard always reduces incoming damage regardless of type.
     */
    int computeDamageAgainst(const Unit& defender, bool isInitiator = true) const {
        bool isMage = (type == UnitType::MAGE);
        int atk     = isMage ? tmpl->spc : tmpl->str;
        int mit     = isMage ? defender.tmpl->res : defender.tmpl->def;
        int raw     = atk + (isInitiator ? tmpl->ini : 0)
                      - mit - defender.tmpl->grd;
        int dmg = std::max(1, raw);
        for (const auto& mod : modifiers)
            dmg = mod->modify(dmg, *this, defender);
        return dmg;
    }

    void setMoved(bool flag)    { moved    = flag; }
    void setAttacked(bool flag) { attacked = flag; }

    // True when the unit can neither move nor attack this turn.
    bool isExhausted() const { return attacked; }

    bool hasMoved()    const { return moved; }

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

    //                                           hp   mov  rng  sgt  str  spc  def  res  prec agi  ini  grd
    static constexpr UnitTemplate WARRIOR_TMPL = { 50,  2,   1,   3,   4,   1,   3,   1,   4,   2,   2,   4 };
    static constexpr UnitTemplate SCOUT_TMPL   = { 40,  3,   1,   5,   2,   2,   2,   3,   6,   7,   2,   2 };
    static constexpr UnitTemplate RANGER_TMPL  = { 35,  3,   3,   4,   3,   1,   1,   2,   6,   5,   3,   2 };
    static constexpr UnitTemplate CAVALRY_TMPL = { 50,  4,   1,   3,   4,   1,   4,   1,   3,   1,   4,   3 };
    static constexpr UnitTemplate MAGE_TMPL    = { 25,  2,   2,   2,   1,   5,   0,   3,   5,   3,   4,   1 };
};
