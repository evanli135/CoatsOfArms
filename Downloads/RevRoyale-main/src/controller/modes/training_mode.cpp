#include "controller/modes/training_mode.h"
#include "model/world.h"
#include "model/player.h"
#include "model/city.h"
#include "model/unit.h"
#include "model/resource_system.h"

static const UnitType UNIT_TYPES[] = {
    UnitType::WARRIOR, UnitType::SCOUT, UnitType::RANGER,
    UnitType::CAVALRY, UnitType::MAGE
};

TrainingMode::TrainingMode(World& world, const Player& player)
    : world(world), player(player) {}

// ---------------------------------------------------------------------------
// onTileSelect
//
// Two flows are supported:
//   City-first:   No button pressed yet   → store city, wait for button press.
//   Button-first: pendingUnitIndex is set  → execute training immediately.
//
// In both cases the tile must contain an owned city; non-city clicks while
// pendingUnitIndex is set return INVALIDTARGET so the error view fires.
// ---------------------------------------------------------------------------
std::optional<PlayerError> TrainingMode::onTileSelect(Position pos) {
    if (!world.hasCityAt(pos))                                        return PlayerError::INVALIDTARGET;
    if (!world.getCityAt(pos)->hasOwner())                            return PlayerError::INVALIDTARGET;
    if (world.getCityAt(pos)->getOwner().getId() != player.getId())   return PlayerError::INVALIDTARGET;

    if (pendingUnitIndex.has_value()) {
        // Button-first: fire the training order now.
        int idx = *pendingUnitIndex;
        auto result = world.issueTrainCommand(pos, UNIT_TYPES[idx], player);
        if (!result.has_value()) {
            pendingUnitIndex.reset();   // clear button highlight on success
        }
        return result;
    }

    // City-first: store selection and wait for a button press.
    selection = pos;
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// onActionButton
//
// Button-first: no city selected yet → store the index, wait for a tile click.
// City-first:   city already selected → execute training immediately.
// ---------------------------------------------------------------------------
std::optional<PlayerError> TrainingMode::onActionButton(int index) {
    if (index < 0 || index >= 5) return PlayerError::NOTSUPPORTED;

    if (selection.has_value()) {
        // City-first: execute right away.
        auto result = world.issueTrainCommand(*selection, UNIT_TYPES[index], player);
        if (!result.has_value()) {
            selection.reset();
        }
        return result;
    }

    // Button-first: just highlight the button; wait for city click.
    pendingUnitIndex = index;
    return std::nullopt;
}

void TrainingMode::onDeselect() {
    selection.reset();
    pendingUnitIndex.reset();
}

void TrainingMode::onExit() {
    selection.reset();
    pendingUnitIndex.reset();
}

std::vector<bool> TrainingMode::getEnabledActions() const {
    int pid        = player.getId();
    int ownedUnits = world.countUnitsForPlayer(pid);
    bool capFull   = ownedUnits >= TrainingSystem::MAX_UNITS_PER_PLAYER;

    // If a city is already selected, check whether it's already training.
    bool cityBusy = false;
    if (selection.has_value()) {
        const City* city = world.getCityAt(*selection);
        if (city && city->isTraining()) cityBusy = true;
    }

    std::vector<bool> result;
    for (auto t : UNIT_TYPES) {
        int cost       = ResourceSystem::unitFoodCost(t);
        bool canAfford = world.getAvailableCapacity(pid, ResourceType::FOOD) >= cost;
        result.push_back(!capFull && canAfford && !cityBusy);
    }
    return result;
}
