#include "controller/modes/building_mode.h"
#include "model/world.h"
#include "model/player.h"
#include "model/tile.h"
#include "model/resource_system.h"
#include "model/construction_system.h"

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
        BuildingType::SHRINE,
    };
    if (index < 0 || index >= 7) return PlayerError::NOTSUPPORTED;

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

std::vector<std::string> BuildingMode::getActionLabels() const {
    struct Info {
        BuildingType type;
        const char*  name;
        const char*  terrainHint;   // nullptr = any land tile
    };
    static const Info INFOS[] = {
        { BuildingType::FOUNDRY,     "Foundry",     nullptr         },
        { BuildingType::BARRACK,     "Barracks",    nullptr         },
        { BuildingType::FARM,        "Farm",        "Grass only"    },
        { BuildingType::FISHERY,     "Fishery",     "River/Ocean"   },
        { BuildingType::LUMBER_CAMP, "Lumber Camp", "Forest only"   },
        { BuildingType::MINE,        "Mine",        "Mountain only" },
        { BuildingType::SHRINE,      "Shrine",      nullptr         },
    };

    int pid        = player.getId();
    int availMetal = world.getAvailableCapacity(pid, ResourceType::METAL);
    int availWood  = world.getAvailableCapacity(pid, ResourceType::WOOD);

    std::vector<std::string> result;
    for (const auto& info : INFOS) {
        int mc = ResourceSystem::buildingMetalCost(info.type);
        int wc = ResourceSystem::buildingWoodCost(info.type);

        // Line 1: name + cost breakdown
        std::string label = info.name;
        if (mc > 0 || wc > 0) {
            label += "  ";
            if (mc > 0) label += std::to_string(mc) + "m";
            if (mc > 0 && wc > 0) label += "+";
            if (wc > 0) label += std::to_string(wc) + "w";
        }

        // Line 2: first blocking reason, then terrain hint
        std::string line2;
        bool hasResources = (availMetal >= mc && availWood >= wc);

        if (availMetal < mc) {
            line2 = "Need " + std::to_string(mc) + "m metal";
        } else if (availWood < wc) {
            line2 = "Need " + std::to_string(wc) + "w wood";
        } else if (selection.has_value()) {
            const Tile& tile = world.getTileAt(*selection);
            bool tileEmpty = !tile.hasTileBuilding();
            bool notQueued = true;
            for (const auto& entry : world.getConstructionQueue())
                if (entry.pos == *selection) { notQueued = false; break; }
            bool terrainOk = ConstructionSystem::canBuildOnTerrain(info.type, tile.getTerrain());

            if      (!tileEmpty)  line2 = "Tile occupied";
            else if (!notQueued)  line2 = "Already queued";
            else if (!terrainOk)  line2 = info.terrainHint ? info.terrainHint : "Not on ocean";
            else if (info.terrainHint) line2 = info.terrainHint;
        } else if (info.terrainHint) {
            line2 = info.terrainHint;
        }

        if (!line2.empty())
            label += "\n" + line2;

        result.push_back(label);
    }
    return result;
}

std::vector<bool> BuildingMode::getEnabledActions() const {
    static const BuildingType TYPES[] = {
        BuildingType::FOUNDRY, BuildingType::BARRACK,
        BuildingType::FARM, BuildingType::FISHERY,
        BuildingType::LUMBER_CAMP, BuildingType::MINE,
        BuildingType::SHRINE,
    };

    int pid        = player.getId();
    int availMetal = world.getAvailableCapacity(pid, ResourceType::METAL);
    int availWood  = world.getAvailableCapacity(pid, ResourceType::WOOD);

    std::vector<bool> result;
    for (auto btype : TYPES) {
        int  mc = ResourceSystem::buildingMetalCost(btype);
        int  wc = ResourceSystem::buildingWoodCost(btype);
        bool hasResources = (availMetal >= mc && availWood >= wc);

        if (!selection.has_value()) {
            result.push_back(hasResources);
            continue;
        }

        const Tile& tile = world.getTileAt(*selection);
        bool terrainOk   = ConstructionSystem::canBuildOnTerrain(btype, tile.getTerrain());
        bool tileEmpty   = !tile.hasTileBuilding();
        bool notQueued   = true;
        for (const auto& entry : world.getConstructionQueue())
            if (entry.pos == *selection) { notQueued = false; break; }

        result.push_back(hasResources && terrainOk && tileEmpty && notQueued);
    }
    return result;
}
