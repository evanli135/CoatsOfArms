#include "controller/modes/building_mode.h"
#include "model/world.h"
#include "model/player.h"

BuildingMode::BuildingMode(World& world, const Player& player)
    : world(world), player(player) {}

std::optional<PlayerError> BuildingMode::onTileSelect(Position pos) {
    return selectOrigin(pos);
}

// Phase 1: choose a tile to build on (world validates ownership/legality).
std::optional<PlayerError> BuildingMode::selectOrigin(Position pos) {
    selection = pos;
    return std::nullopt;
}

// Phase 2 (button): choose a building type — not yet implemented.
std::optional<PlayerError> BuildingMode::onActionButton(int index) {
    if (!selection.has_value()) return PlayerError::INVALIDTARGET;
    return PlayerError::NOTSUPPORTED;
}

void BuildingMode::onDeselect() {
    selection.reset();
}

void BuildingMode::onExit() {
    selection.reset();
}
