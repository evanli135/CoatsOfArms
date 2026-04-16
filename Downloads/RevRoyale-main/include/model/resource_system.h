#pragma once

#include "model/economy.h"   // ResourceType, BuildingType
#include "model/unit.h"      // UnitType

class World;

// ---------------------------------------------------------------------------
// ResourceSystem — capacity economy calculations.
//
// Food capacity comes from food buildings (Farm/Fishery/LumberCamp) on city
// border tiles.  Metal capacity comes from a base per owned city plus Mine
// output.  Neither resource is "spent"; capacity is consumed by existence of
// units and buildings and freed automatically when they are removed.
// ---------------------------------------------------------------------------
class ResourceSystem {
    friend class World;
public:
    explicit ResourceSystem(World& world) : world(world) {}

    /** Total food/metal provided by all entities owned by playerId. */
    int getTotalCapacity(int playerId, ResourceType rt) const;

    /** Food/metal currently consumed by living units, in-training units,
     *  completed buildings, and in-progress construction. */
    int getUsedCapacity(int playerId, ResourceType rt) const;

    /** Available = total - used.  May be negative if capacity was lost mid-game. */
    int getAvailableCapacity(int playerId, ResourceType rt) const;

    // ── Lookup tables (also used externally for cost display / validation) ──
    static int unitFoodCost(UnitType type);
    static int buildingMetalCost(BuildingType type);
    static int buildingWoodCost(BuildingType type);
    static int buildingFoodOutput(BuildingType type);
    static int buildingMetalOutput(BuildingType type);
    static int buildingWoodOutput(BuildingType type);

private:
    World& world;
};
