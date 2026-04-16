#pragma once

#include <vector>
#include <unordered_set>
#include <optional>

#include "model/economy.h"         // BuildingType
#include "model/util.h"            // Position, Game
#include "model/player.h"          // Player
#include "controller/error.h"      // PlayerError

class World;
class City;
enum class Terrain;   // defined in tile.h; forward-declare to avoid heavy chain

// ---------------------------------------------------------------------------
// ConstructionEntry — one building currently in the construction queue.
// ---------------------------------------------------------------------------
struct ConstructionEntry {
    Position     pos;            // border tile where the building will be placed
    Position     cityPos;        // city this project belongs to
    BuildingType type;
    int          turnsRemaining;
    int          ownerPlayerId;
};

// ---------------------------------------------------------------------------
// ConstructionSystem — manages the building queue, terrain validation, and
// all city-territory queries used by the construction domain.
// ---------------------------------------------------------------------------
class ConstructionSystem {
    friend class World;
public:
    explicit ConstructionSystem(World& world) : world(world) {}

    // ── Building queue ────────────────────────────────────────────────────

    /** Validates ownership, terrain, capacity, and queues the building.
     *  tilePos must be a border tile of a city owned by player. */
    std::optional<PlayerError> scheduleConstruction(const Position& tilePos,
                                                    BuildingType    type,
                                                    const Player&   player);

    /** Tick construction queue for playerId; place completed buildings on tiles. */
    void advanceConstruction(int playerId);

    /** Remove the first queue entry matching pos+type+owner (for undo). */
    void cancelEntry(const Position& tilePos, BuildingType type, int playerId);

    const std::vector<ConstructionEntry>& getQueue() const { return queue; }

    // ── Territory queries ─────────────────────────────────────────────────

    /** All tiles within Chebyshev distance borderRadius of cityPos,
     *  excluding cityPos itself. */
    std::unordered_set<Position> getCityBorderTiles(Position cityPos) const;

    /** The city whose territory contains pos (center or border), or nullptr. */
    const City*  getCityForTile(Position pos) const;

    /** Position of the city center whose territory contains pos,
     *  or {-1,-1} if pos is not in any city's territory. */
    Position     getCityPosForTile(Position pos) const;

    /** Count completed buildings of type within cityPos's border. */
    int          countBuildingsInCity(Position cityPos, BuildingType type) const;

    /** True if at least one completed building of type exists in this city. */
    bool         cityHasBuilding(Position cityPos, BuildingType type) const;

    /** True if this city has any pending construction entry in the queue. */
    bool         cityHasActiveConstruction(Position cityPos) const;

    /** All border tiles owned by playerId that don't yet have a building or
     *  pending construction entry. */
    std::vector<Position> getBuildableTiles(int playerId) const;

    // ── Terrain compatibility ─────────────────────────────────────────────
    static bool canBuildOnTerrain(BuildingType type, Terrain terrain);

private:
    World& world;
    std::vector<ConstructionEntry> queue;
};
