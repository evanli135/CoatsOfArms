#include "controller/modes/building_mode.h"
#include "model/world.h"
#include "model/player.h"

BuildingMode::BuildingMode(World& world, const Player& player)
    : world(world), player(player) {}

std::optional<PlayerError> BuildingMode::onTileSelect(Position pos) {
    return selectOrigin(pos);
}

// Phase 1: choose a border tile to build on.
// Validates that the tile is in a city border owned by this player.
std::optional<PlayerError> BuildingMode::selectOrigin(Position pos) {
    // City center tiles are for training, not building placement.
    if (world.hasCityAt(pos)) return PlayerError::INVALIDTARGET;

    const City* city = world.getCityForTile(pos);
    if (!city) return PlayerError::INVALIDTARGET;
    if (!city->hasOwner() || city->getOwner().getId() != player.getId())
        return PlayerError::INVALIDTARGET;

    selection = pos;
    return std::nullopt;
}

// Phase 2 (button): choose a building type to construct at the selected tile.
std::optional<PlayerError> BuildingMode::onActionButton(int index) {
    if (!selection.has_value()) return PlayerError::INVALIDTARGET;

    static const BuildingType types[] = {
        BuildingType::FOUNDRY,
        BuildingType::BARRACK,
        BuildingType::FARM,
        BuildingType::FISHERY,
        BuildingType::LUMBER_CAMP,
        BuildingType::MINE,
    };
    if (index < 0 || index >= 6) return PlayerError::NOTSUPPORTED;

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
