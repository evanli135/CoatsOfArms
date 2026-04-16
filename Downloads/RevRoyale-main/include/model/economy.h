#pragma once

#include <string>
#include <functional>

enum class ResourceType {
    FOOD,
    WOOD,
    STONE,
    METAL
};

enum class BuildingType {
    FOUNDRY,       // speeds up construction (-1 turn), no terrain req
    BARRACK,       // enables unit training, no terrain req
    FARM,          // +4 food capacity, requires GRASS
    FISHERY,       // +3 food capacity, requires OCEAN or RIVER
    LUMBER_CAMP,   // +2 food capacity, requires FOREST
    MINE,          // +3 metal capacity, requires MOUNTAIN
    SHRINE,        // lets adjacent units pray for spirit blessings; no terrain req (except ocean)
};

class Building {
public:
    Building(BuildingType type);
    const std::string& getName() const;
    BuildingType getType() const;

    int constructionCost() const;
    int constructionCompletion() const;

    void addConstruction(int cost);

private:
    std::string name;
    BuildingType type;
};

namespace std {
    template<>
    struct hash<BuildingType> {
        size_t operator()(BuildingType b) const noexcept {
            return std::hash<int>()(static_cast<int>(b));
        }
    };
    template<>
    struct hash<ResourceType> {
        size_t operator()(ResourceType r) const noexcept {
            return std::hash<int>()(static_cast<int>(r));
        }
    };
}

// ---------------------------------------------------------------------------
// Static capacity helpers — used by World to compute available resources.
// ---------------------------------------------------------------------------

// Base resource capacity provided by a single owned city.
// Food comes only from buildings; metal and wood have a city base supply.
inline int cityCapacity(ResourceType rt) {
    if (rt == ResourceType::METAL) return 3;
    if (rt == ResourceType::WOOD)  return 2;
    return 0;   // FOOD: build Farms/Fisheries; cities provide no food
}

class BuildingFactory {
public:
    static Building createBuilding(BuildingType type) {
        return Building(type);
    }
};