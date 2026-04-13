#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <stdexcept>

#include "model/player.h"
#include "model/economy.h"
#include "model/unit.h"

using std::string, std::unordered_map;

const int MAX_LEVEL = 5;

// ---------------------------------------------------------------------------
// TrainingSlot — one unit queued for production in a city.
//
// turnsRemaining starts at 2 and decrements once at the START of each of the
// owning player's turns.  When it reaches 0 and the city tile is unoccupied,
// the unit is spawned and the slot is cleared.
// ---------------------------------------------------------------------------
struct TrainingSlot {
    UnitType unitType;
    int      ownerId;
    int      turnsRemaining;   // ≥1 = in progress; 0 = ready to spawn
};

class City {
public:
    City(const string& name)
        : name(name), borderRadius(2), upgradeLevel(0), level(0),
          owner(&Player::null()) {}

    bool          hasOwner()   const { return !owner->isNull(); }
    const Player& getOwner()   const { return *owner; }
    void setOwner(Player* player) {
        owner = (player && !player->isNull()) ? player : &Player::null();
    }

    void upgrade() {
        if (upgradeLevel >= MAX_LEVEL)
            throw std::logic_error("City has reached max upgrade level");
        upgradeLevel++;
    }

    const string& getName()      const { return name; }
    int           getBorderRadius() const { return borderRadius; }

    // ── Training slot ──────────────────────────────────────────────────────
    bool isTraining() const { return trainingSlot.has_value(); }

    const TrainingSlot* getTrainingSlot() const {
        return trainingSlot ? &*trainingSlot : nullptr;
    }
    TrainingSlot* getTrainingSlotMutable() {
        return trainingSlot ? &*trainingSlot : nullptr;
    }

    /** Queue a unit.  turns = how many of the owner's turns until it spawns. */
    void startTraining(UnitType type, int ownerId, int turns = 2) {
        trainingSlot = TrainingSlot{type, ownerId, turns};
    }

    void clearTraining() { trainingSlot.reset(); }

    /** Decrement turnsRemaining by 1 (no-op if already 0). */
    void tickTraining() {
        if (trainingSlot && trainingSlot->turnsRemaining > 0)
            trainingSlot->turnsRemaining--;
    }

private:
    string name;
    std::optional<TrainingSlot> trainingSlot;
    const Player* owner;
    int borderRadius;
    int upgradeLevel;
    int level;
};
