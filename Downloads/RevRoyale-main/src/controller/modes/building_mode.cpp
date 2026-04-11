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

// Phase 2 (button): choose a building type to construct at the selected city.
std::optional<PlayerError> BuildingMode::onActionButton(int index) {
    if (!selection.has_value()) return PlayerError::INVALIDTARGET;

    static const BuildingType types[] = {
        BuildingType::FOUNDRY, BuildingType::BARRACK
    };
    if (index < 0 || index >= 2) return PlayerError::NOTSUPPORTED;

    auto result = world.issueConstructCommand(*selection, types[index], player);
    if (!result.has_value()) {
        selection.reset();
    }
    return result;
}

void BuildingMode::onDeselect() {
    selection.reset();
}

void BuildingMode::onExit() {
    selection.reset();
}
