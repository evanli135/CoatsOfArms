#include "controller/modes/training_mode.h"
#include "model/world.h"
#include "model/player.h"
#include "model/city.h"
#include "model/unit.h"

TrainingMode::TrainingMode(World& world, const Player& player)
    : world(world), player(player) {}

std::optional<PlayerError> TrainingMode::onTileSelect(Position pos) {
    return selectOrigin(pos);
}

// Phase 1: choose an owned city to train from.
std::optional<PlayerError> TrainingMode::selectOrigin(Position pos) {
    if (!world.hasCityAt(pos))                                                               return PlayerError::INVALIDTARGET;
    if (!world.getCityAt(pos)->hasOwner())                                                   return PlayerError::INVALIDTARGET;
    if (world.getCityAt(pos)->getOwner().getId() != player.getId())                          return PlayerError::INVALIDTARGET;

    selection = pos;
    return std::nullopt;
}

// Phase 2 (button): choose a unit type to train.
std::optional<PlayerError> TrainingMode::onActionButton(int index) {
    if (!selection.has_value()) return PlayerError::INVALIDTARGET;

    static const UnitType types[] = {
        UnitType::WARRIOR, UnitType::SCOUT, UnitType::RANGER,
        UnitType::CAVALRY, UnitType::MAGE
    };
    if (index < 0 || index >= 5) return PlayerError::NOTSUPPORTED;

    auto result = world.issueTrainCommand(*selection, types[index], player);
    if (!result.has_value()) {
        // Unit trained — deselect city so player can do something else.
        selection.reset();
    }
    return result;
}

void TrainingMode::onDeselect() {
    selection.reset();
}

void TrainingMode::onExit() {
    selection.reset();
}
