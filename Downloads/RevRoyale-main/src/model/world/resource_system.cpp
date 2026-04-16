#include "model/resource_system.h"
#include "model/world.h"
#include "model/city.h"
#include <algorithm>

// ---------------------------------------------------------------------------
// Static lookup tables
// ---------------------------------------------------------------------------

// Food each fielded unit costs per turn (upkeep / supply burden).
// These are intentionally high so a food shortage is a real strategic constraint.
int ResourceSystem::unitFoodCost(UnitType type) {
    switch (type) {
        case UnitType::WARRIOR: return 4;
        case UnitType::SCOUT:   return 2;
        case UnitType::RANGER:  return 3;
        case UnitType::CAVALRY: return 6;   // horses eat a lot
        case UnitType::MAGE:    return 3;
    }
    return 0;
}

// Metal capacity consumed by the existence of an infrastructure building.
int ResourceSystem::buildingMetalCost(BuildingType type) {
    switch (type) {
        case BuildingType::FOUNDRY:     return 1;
        case BuildingType::BARRACK:     return 1;
        case BuildingType::MINE:        return 1;   // shoring iron
        case BuildingType::SHRINE:      return 2;
        default:                        return 0;   // food/wood buildings free in metal
    }
}

// Wood capacity consumed by a building (most structures need timber).
int ResourceSystem::buildingWoodCost(BuildingType type) {
    switch (type) {
        case BuildingType::FOUNDRY:     return 1;   // bellows & fuel
        case BuildingType::BARRACK:     return 2;   // large timber frame
        case BuildingType::FISHERY:     return 1;   // dock planking
        case BuildingType::MINE:        return 1;   // shaft shoring
        case BuildingType::SHRINE:      return 3;   // carved pillars + altar
        default:                        return 0;   // Farm & LumberCamp need no wood
    }
}

// Food output from a food-producing building (affected by tile richness).
int ResourceSystem::buildingFoodOutput(BuildingType type) {
    switch (type) {
        case BuildingType::FARM:    return 5;
        case BuildingType::FISHERY: return 4;
        default:                    return 0;
        // Lumber Camp now produces wood, not food.
    }
}

int ResourceSystem::buildingMetalOutput(BuildingType type) {
    return (type == BuildingType::MINE) ? 3 : 0;
}

// Wood output from a wood-producing building.
int ResourceSystem::buildingWoodOutput(BuildingType type) {
    return (type == BuildingType::LUMBER_CAMP) ? 3 : 0;
}

// Flat bonus added to (or subtracted from) a building's output based on tile richness.
static int tileResourceBonus(TileResourceValue rv) {
    switch (rv) {
        case TileResourceValue::LOW:    return -1;
        case TileResourceValue::HIGH:   return  2;
        default:                        return  0;  // MEDIUM
    }
}

// ---------------------------------------------------------------------------
// Capacity calculations
// ---------------------------------------------------------------------------

int ResourceSystem::getTotalCapacity(int playerId, ResourceType rt) const {
    int total = 0;

    if (rt == ResourceType::FOOD) {
        // Food comes entirely from food buildings (Farm, Fishery) on border tiles.
        for (const auto& cpos : world.cityPositions) {
            const City* city = world.getCityAt(cpos);
            if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
                continue;
            for (const auto& bpos : world.getCityBorderTiles(cpos)) {
                const Tile& t = world.getTileAt(bpos);
                if (t.hasTileBuilding()) {
                    int base = buildingFoodOutput(*t.getTileBuilding());
                    if (base > 0)
                        total += std::max(1, base + tileResourceBonus(t.getTileResourceValue()));
                }
            }
        }
    } else if (rt == ResourceType::METAL) {
        // Metal = city base per owned city + mine output on border tiles.
        for (const auto& cpos : world.cityPositions) {
            const City* city = world.getCityAt(cpos);
            if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
                continue;
            total += cityCapacity(ResourceType::METAL);
            for (const auto& bpos : world.getCityBorderTiles(cpos)) {
                const Tile& t = world.getTileAt(bpos);
                if (t.hasTileBuilding()) {
                    int base = buildingMetalOutput(*t.getTileBuilding());
                    if (base > 0)
                        total += std::max(1, base + tileResourceBonus(t.getTileResourceValue()));
                }
            }
        }
    } else if (rt == ResourceType::WOOD) {
        // Wood = city base per owned city + lumber camp output on border tiles.
        for (const auto& cpos : world.cityPositions) {
            const City* city = world.getCityAt(cpos);
            if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
                continue;
            total += cityCapacity(ResourceType::WOOD);
            for (const auto& bpos : world.getCityBorderTiles(cpos)) {
                const Tile& t = world.getTileAt(bpos);
                if (t.hasTileBuilding()) {
                    int base = buildingWoodOutput(*t.getTileBuilding());
                    if (base > 0)
                        total += std::max(1, base + tileResourceBonus(t.getTileResourceValue()));
                }
            }
        }
    }

    return total;
}

int ResourceSystem::getUsedCapacity(int playerId, ResourceType rt) const {
    int used = 0;

    if (rt == ResourceType::FOOD) {
        // Living units
        for (const auto& [id, unit] : world.units) {
            if (unit->getOwner().getId() == playerId)
                used += unitFoodCost(unit->getType());
        }
        // Units currently in training (capacity reserved while queued)
        for (const auto& cpos : world.cityPositions) {
            const City* city = world.getCityAt(cpos);
            if (!city) continue;
            if (city->isTraining()) {
                const TrainingSlot* slot = city->getTrainingSlot();
                if (slot && slot->ownerId == playerId)
                    used += unitFoodCost(slot->unitType);
            }
        }
    } else if (rt == ResourceType::METAL) {
        // Completed buildings on border tiles of owned cities
        for (const auto& cpos : world.cityPositions) {
            const City* city = world.getCityAt(cpos);
            if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
                continue;
            for (const auto& bpos : world.getCityBorderTiles(cpos)) {
                const Tile& t = world.getTileAt(bpos);
                if (t.hasTileBuilding())
                    used += buildingMetalCost(*t.getTileBuilding());
            }
        }
        // In-progress construction entries
        for (const auto& entry : world.constructionSystem.getQueue()) {
            if (entry.ownerPlayerId == playerId)
                used += buildingMetalCost(entry.type);
        }
    } else if (rt == ResourceType::WOOD) {
        // Completed buildings
        for (const auto& cpos : world.cityPositions) {
            const City* city = world.getCityAt(cpos);
            if (!city || !city->hasOwner() || city->getOwner().getId() != playerId)
                continue;
            for (const auto& bpos : world.getCityBorderTiles(cpos)) {
                const Tile& t = world.getTileAt(bpos);
                if (t.hasTileBuilding())
                    used += buildingWoodCost(*t.getTileBuilding());
            }
        }
        // In-progress construction entries
        for (const auto& entry : world.constructionSystem.getQueue()) {
            if (entry.ownerPlayerId == playerId)
                used += buildingWoodCost(entry.type);
        }
    }

    return used;
}

int ResourceSystem::getAvailableCapacity(int playerId, ResourceType rt) const {
    return getTotalCapacity(playerId, rt) - getUsedCapacity(playerId, rt);
}
