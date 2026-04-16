#include "controller/modes/building_mode.h"
#include "model/world.h"
#include "model/player.h"
#include "model/tile.h"
#include "model/resource_system.h"
#include "model/construction_system.h"

static const BuildingType BUILDING_TYPES[] = {
    BuildingType::FOUNDRY,
    BuildingType::BARRACK,
    BuildingType::FARM,
    BuildingType::FISHERY,
    BuildingType::LUMBER_CAMP,
    BuildingType::MINE,
    BuildingType::SHRINE,
};
static constexpr int NUM_TYPES = 7;

BuildingMode::BuildingMode(World& world, const Player& player)
    : world(world), player(player) {}

// ---------------------------------------------------------------------------
// onTileSelect
//
// Two flows:
//   City-center first: click city center → selection set, building buttons appear.
//     Then click a building button → pendingTypeIndex set, border tiles highlighted.
//     Then click a border tile → construction issued.
//
//   Border-tile first (unchanged): click border tile → selection set.
//     Then click a building button → construction issued immediately.
// ---------------------------------------------------------------------------
std::optional<PlayerError> BuildingMode::onTileSelect(Position pos) {
    if (pendingTypeIndex.has_value()) {
        // Phase 3: user has chosen a building type from the city; now picking the border tile.
        if (world.hasCityAt(pos)) return PlayerError::INVALIDTARGET;
        const City* city = world.getCityForTile(pos);
        if (!city || !city->hasOwner() || city->getOwner().getId() != player.getId())
            return PlayerError::INVALIDTARGET;

        auto result = world.issueConstructCommand(pos, BUILDING_TYPES[*pendingTypeIndex], player);
        if (!result.has_value()) {
            selection.reset();
            selectionIsCityCenter = false;
            pendingTypeIndex.reset();
        }
        return result;
    }

    return selectOrigin(pos);
}

// Phase 1a: city center clicked — select the city.
// Phase 1b: border tile clicked — select that tile directly (existing flow).
std::optional<PlayerError> BuildingMode::selectOrigin(Position pos) {
    if (world.hasCityAt(pos)) {
        // City center selected — validate ownership, wait for building type button.
        const City* city = world.getCityAt(pos);
        if (!city || !city->hasOwner() || city->getOwner().getId() != player.getId())
            return PlayerError::INVALIDTARGET;

        selection             = pos;
        selectionIsCityCenter = true;
        pendingTypeIndex.reset();
        return std::nullopt;
    }

    // Border tile selected — existing flow.
    const City* city = world.getCityForTile(pos);
    if (!city) return PlayerError::INVALIDTARGET;
    if (!city->hasOwner() || city->getOwner().getId() != player.getId())
        return PlayerError::INVALIDTARGET;

    selection             = pos;
    selectionIsCityCenter = false;
    pendingTypeIndex.reset();
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// onActionButton
//
// If a city center was selected: store the building type and wait for a
// border-tile click (phase 2 → 3). The button is highlighted via
// getPendingButtonIndex() until the tile is chosen.
//
// If a border tile was selected: issue construction immediately.
// ---------------------------------------------------------------------------
std::optional<PlayerError> BuildingMode::onActionButton(int index) {
    if (!selection.has_value()) return PlayerError::INVALIDTARGET;
    if (index < 0 || index >= NUM_TYPES) return PlayerError::NOTSUPPORTED;

    if (selectionIsCityCenter) {
        // Phase 2: building type chosen — now wait for border tile.
        pendingTypeIndex = index;
        return std::nullopt;
    }

    // Border tile already selected — build immediately.
    auto result = world.issueConstructCommand(*selection, BUILDING_TYPES[index], player);
    if (!result.has_value()) {
        selection.reset();
        selectionIsCityCenter = false;
        pendingTypeIndex.reset();
    }
    return result;
}

void BuildingMode::onDeselect() {
    selection.reset();
    selectionIsCityCenter = false;
    pendingTypeIndex.reset();
}

void BuildingMode::onExit() {
    onDeselect();
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

    // When awaiting a border-tile click, show a header hint instead of "ACTIONS".
    // (The action labels themselves stay as building types so the selected one stays visible.)

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

        // Line 2: first blocking reason, then terrain hint.
        // Tile-specific checks only apply when a border tile (not city center) is selected.
        std::string line2;
        if (availMetal < mc) {
            line2 = "Need " + std::to_string(mc) + "m metal";
        } else if (availWood < wc) {
            line2 = "Need " + std::to_string(wc) + "w wood";
        } else if (selection.has_value() && !selectionIsCityCenter) {
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
    int pid        = player.getId();
    int availMetal = world.getAvailableCapacity(pid, ResourceType::METAL);
    int availWood  = world.getAvailableCapacity(pid, ResourceType::WOOD);

    std::vector<bool> result;
    for (auto btype : BUILDING_TYPES) {
        int  mc = ResourceSystem::buildingMetalCost(btype);
        int  wc = ResourceSystem::buildingWoodCost(btype);
        bool hasResources = (availMetal >= mc && availWood >= wc);

        // When no tile is selected, or a city center is selected: only gate on resources.
        if (!selection.has_value() || selectionIsCityCenter) {
            result.push_back(hasResources);
            continue;
        }

        // Border tile selected: also check terrain, occupancy, and queue.
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
