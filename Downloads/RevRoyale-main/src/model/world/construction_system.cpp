#include "model/construction_system.h"
#include "model/world.h"
#include "model/tile.h"     // Tile, Terrain
#include "model/city.h"
#include "model/resource_system.h"   // buildingMetalCost
#include <algorithm>

// ---------------------------------------------------------------------------
// Terrain compatibility
// ---------------------------------------------------------------------------

bool ConstructionSystem::canBuildOnTerrain(BuildingType type, Terrain terrain) {
    switch (type) {
        case BuildingType::FARM:        return terrain == Terrain::GRASS;
        case BuildingType::FISHERY:     return terrain == Terrain::OCEAN
                                            || terrain == Terrain::RIVER;
        case BuildingType::LUMBER_CAMP: return terrain == Terrain::FOREST;
        case BuildingType::MINE:        return terrain == Terrain::MOUNTAIN;
        default:                        return terrain != Terrain::OCEAN;  // BARRACK, FOUNDRY, SHRINE
    }
}

// ---------------------------------------------------------------------------
// Building queue
// ---------------------------------------------------------------------------

std::optional<PlayerError> ConstructionSystem::scheduleConstruction(
    const Position& tilePos, BuildingType type, const Player& player)
{
    // Tile must be in a city's border (not the city center itself).
    if (world.hasCityAt(tilePos)) return PlayerError::INVALIDTARGET;
    const City* city = getCityForTile(tilePos);
    if (!city) return PlayerError::INVALIDTARGET;
    if (!city->hasOwner() || city->getOwner().getId() != player.getId())
        return PlayerError::INVALIDTARGET;

    // Can't build on an occupied tile.
    if (world.getTileAt(tilePos).hasTileBuilding()) return PlayerError::INVALIDTARGET;
    if (world.getTileAt(tilePos).hasUnit())         return PlayerError::INVALIDTARGET;

    // Building type must match this tile's terrain.
    if (!canBuildOnTerrain(type, world.getTileAt(tilePos).getTerrain()))
        return PlayerError::INVALIDTARGET;

    // Only one construction per tile at a time.
    for (const auto& entry : queue)
        if (entry.pos == tilePos) return PlayerError::INVALIDTARGET;

    // Check available metal and wood capacity.
    int metalCost = ResourceSystem::buildingMetalCost(type);
    int woodCost  = ResourceSystem::buildingWoodCost(type);
    if (world.getAvailableCapacity(player.getId(), ResourceType::METAL) < metalCost)
        return PlayerError::INSUFFICIENTRESOURCES;
    if (world.getAvailableCapacity(player.getId(), ResourceType::WOOD) < woodCost)
        return PlayerError::INSUFFICIENTRESOURCES;

    // Base 2 turns, -1 per Foundry already in this city (min 1).
    Position cpos  = getCityPosForTile(tilePos);

    // A city can only produce one thing at a time: either a unit or a building.
    if (city->isTraining()) return PlayerError::INVALIDTARGET;
    if (cityHasActiveConstruction(cpos)) return PlayerError::INVALIDTARGET;

    int      turns = 2 - countBuildingsInCity(cpos, BuildingType::FOUNDRY);
    turns = std::max(1, turns);

    queue.push_back({tilePos, cpos, type, turns, player.getId()});
    return std::nullopt;
}

void ConstructionSystem::advanceConstruction(int playerId) {
    for (auto it = queue.begin(); it != queue.end(); ) {
        if (it->ownerPlayerId != playerId) { ++it; continue; }
        it->turnsRemaining--;
        if (it->turnsRemaining <= 0) {
            world.getTileAt(it->pos).setTileBuilding(it->type);
            // A completed shrine registers as a spirit site so units can pray.
            if (it->type == BuildingType::SHRINE)
                world.addShrine(it->pos);
            it = queue.erase(it);
        } else {
            ++it;
        }
    }
}

void ConstructionSystem::cancelEntry(
    const Position& tilePos, BuildingType type, int playerId)
{
    for (auto it = queue.begin(); it != queue.end(); ++it) {
        if (it->pos == tilePos && it->type == type
                && it->ownerPlayerId == playerId) {
            queue.erase(it);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Territory queries
// ---------------------------------------------------------------------------

std::unordered_set<Position> ConstructionSystem::getCityBorderTiles(
    Position cityPos) const
{
    std::unordered_set<Position> result;
    const City* city = world.getCityAt(cityPos);
    if (!city) return result;
    int r = city->getBorderRadius();
    for (int dr = -r; dr <= r; ++dr) {
        for (int dc = -r; dc <= r; ++dc) {
            if (dr == 0 && dc == 0) continue;  // exclude city center
            if (std::max(std::abs(dr), std::abs(dc)) > r) continue;
            int nr = cityPos.row() + dr, nc = cityPos.col() + dc;
            if (nr >= 0 && nr < Game::HEIGHT && nc >= 0 && nc < Game::WIDTH)
                result.insert(Position(nr, nc));
        }
    }
    return result;
}

const City* ConstructionSystem::getCityForTile(Position pos) const {
    for (const auto& cpos : world.cityPositions) {
        if (cpos == pos) return world.getCityAt(cpos);
        const City* city = world.getCityAt(cpos);
        if (!city) continue;
        int r = city->getBorderRadius();
        if (std::max(std::abs(pos.row() - cpos.row()),
                     std::abs(pos.col() - cpos.col())) <= r)
            return city;
    }
    return nullptr;
}

Position ConstructionSystem::getCityPosForTile(Position pos) const {
    for (const auto& cpos : world.cityPositions) {
        if (cpos == pos) return cpos;
        const City* city = world.getCityAt(cpos);
        if (!city) continue;
        int r = city->getBorderRadius();
        if (std::max(std::abs(pos.row() - cpos.row()),
                     std::abs(pos.col() - cpos.col())) <= r)
            return cpos;
    }
    return Position(-1, -1);
}

int ConstructionSystem::countBuildingsInCity(
    Position cityPos, BuildingType type) const
{
    int count = 0;
    for (const auto& bpos : getCityBorderTiles(cityPos)) {
        const Tile& t = world.getTileAt(bpos);
        if (t.hasTileBuilding() && *t.getTileBuilding() == type)
            ++count;
    }
    return count;
}

bool ConstructionSystem::cityHasBuilding(
    Position cityPos, BuildingType type) const
{
    return countBuildingsInCity(cityPos, type) > 0;
}

bool ConstructionSystem::cityHasActiveConstruction(Position cityPos) const {
    for (const auto& entry : queue)
        if (entry.cityPos == cityPos) return true;
    return false;
}

std::vector<Position> ConstructionSystem::getBuildableTiles(int playerId) const {
    std::vector<Position> result;
    for (const auto& cpos : world.cityPositions) {
        const City* city = world.getCityAt(cpos);
        if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
            continue;
        // City is occupied with another production order — skip entirely.
        if (city->isTraining() || cityHasActiveConstruction(cpos))
            continue;
        for (const auto& bpos : getCityBorderTiles(cpos)) {
            const Tile& t = world.getTileAt(bpos);
            if (t.hasTileBuilding()) continue;
            if (t.getTerrain() == Terrain::OCEAN) continue;
            result.push_back(bpos);
        }
    }
    return result;
}
