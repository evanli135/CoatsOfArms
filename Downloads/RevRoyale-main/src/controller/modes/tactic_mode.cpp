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
    if (!unit->canMove() && !unit->canAttack())      return PlayerError::UNITCANTMOVE;

    selection = pos;
    return std::nullopt;
}

// Phase 2: choose a destination tile.
// If an action was explicitly chosen via onActionButton(), use it.
// Otherwise auto-infer: enemy unit at destination → ATT, empty tile → MOV.
// Clicking the selected unit, a friendly unit, or empty space deselects / re-selects.
std::optional<PlayerError> TacticMode::selectDestination(Position pos) {
    bool autoInferred = !pendingAction.has_value();

    if (autoInferred) {
        if (world.hasUnitAt(pos)) {
            const Unit* target = world.getUnitAt(pos);
            if (target && !target->sameOwner(player)) {
                pendingAction = ControllerAction::ATT;
            } else {
                // Friendly unit — deselect current, then re-select if it's a different unit.
                bool sameUnit = selection.has_value() && (pos == *selection);
                selection.reset();
                pendingAction.reset();
                if (!sameUnit) {
                    return selectOrigin(pos);
                }
                return std::nullopt;
            }
        } else {
            pendingAction = ControllerAction::MOV;
        }
    }

    auto result = world.applyControllerRequest(
        ControllerRequest(pendingAction.value(), selection.value(), pos, player));

    if (!result.has_value()) {
        // After a move the unit is now at `pos`, not at *selection (origin).
        // After an attack the unit stays at *selection.
        bool wasMov = pendingAction.has_value() &&
                      (*pendingAction == ControllerAction::MOV);
        pendingAction.reset();

        Position unitPos = (wasMov && selection.has_value()) ? pos : selection.value_or(pos);
        const Unit* unit = world.getUnitAt(unitPos);
        if (!unit || unit->isExhausted()) {
            selection.reset();
        } else {
            selection = unitPos;   // track the unit to its new tile after a move
        }
    } else if (autoInferred) {
        // Failed with an auto-inferred action — reset it so the next destination
        // click can re-infer rather than replaying the failed action.
        pendingAction.reset();
    }
    // Failed with an explicitly chosen action — keep pendingAction so the player
    // can pick a different destination without re-clicking the button.

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
