#include "controller/modes/tactic_mode.h"
#include "model/world.h"
#include "model/player.h"
#include "model/unit.h"

TacticMode::TacticMode(World& world, const Player& player)
    : world(world), player(player) {}

std::optional<PlayerError> TacticMode::onTileSelect(Position pos) {
    if (!selection.has_value()) {
        return selectOrigin(pos);
    }
    return selectDestination(pos);
}

// Phase 1: choose a friendly unit to act with.
std::optional<PlayerError> TacticMode::selectOrigin(Position pos) {
    if (!world.hasUnitAt(pos))                       return PlayerError::INVALIDTARGET;
    const Unit* unit = world.getUnitAt(pos);
    if (!unit->sameOwner(player))                    return PlayerError::INVALIDTARGET;
    if (!unit->canMove())                            return PlayerError::UNITCANTMOVE;

    selection = pos;
    return std::nullopt;
}

// Phase 2: choose a destination tile. Requires an action to have been chosen first.
std::optional<PlayerError> TacticMode::selectDestination(Position pos) {
    if (!pendingAction.has_value()) return PlayerError::NOTSUPPORTED;

    auto result = world.applyControllerRequest(
        ControllerRequest(pendingAction.value(), selection.value(), pos, player));

    // On success clear state; on failure keep selection so the player can retry.
    if (!result.has_value()) {
        selection.reset();
        pendingAction.reset();
    }

    return result;
}

// Set the pending action via an action button. Requires a unit to be selected first.
// index 0 = MOV, index 1 = ATT
std::optional<PlayerError> TacticMode::onActionButton(int index) {
    if (!selection.has_value()) return PlayerError::INVALIDTARGET;

    switch (index) {
        case 0: pendingAction = ControllerAction::MOV; return std::nullopt;
        case 1: pendingAction = ControllerAction::ATT; return std::nullopt;
        default: return PlayerError::NOTSUPPORTED;
    }
}

void TacticMode::onDeselect() {
    selection.reset();
    pendingAction.reset();
}

void TacticMode::onExit() {
    selection.reset();
    pendingAction.reset();
}
