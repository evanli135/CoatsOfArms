#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <optional>

#include "model/spirit.h"       // Blessing, SpiritType, generateBlessingChoices
#include "model/util.h"         // Position
#include "model/player.h"       // Player
#include "controller/error.h"   // PlayerError

class World;

// ---------------------------------------------------------------------------
// SpiritSystem — shrine placement, blessing generation, and prayer resolution.
//
// Owns all shrine/blessing state so World no longer holds any of it.
// ---------------------------------------------------------------------------
class SpiritSystem {
    friend class World;
public:
    explicit SpiritSystem(World& world) : world(world) {}

    // ── Shrine management ─────────────────────────────────────────────────

    /** Place a shrine at pos (marks tile flag + records position). */
    void addShrine(Position pos);

    bool hasShrineAt(Position pos) const;

    const std::vector<Position>& getShrinePositions() const { return shrinePositions; }

    /** True if unitPos is Chebyshev-1 adjacent to (or on) any shrine tile. */
    bool isAdjacentToShrine(Position unitPos) const;

    // ── Prayer / blessing flow ────────────────────────────────────────────

    /** Generates 3 blessing choices for player based on turn + player seed.
     *  Stores them as pending; call completePray() to finalise. */
    std::array<Blessing, 3> preparePrayChoices(Position unitPos, const Player& player);

    /** Commits the chosen blessing, exhausts the unit, clears pending choices. */
    std::optional<PlayerError> completePray(Position unitPos,
                                            int         blessingIndex,
                                            const Player& player);

    /** Discards pending choices for this player without applying a blessing. */
    void clearPendingPrayChoices(int playerId);

    // ── Read-only accessors ───────────────────────────────────────────────

    /** All blessings the given player currently holds. */
    const std::vector<Blessing>& getPlayerBlessings(int playerId) const;

    /** Pending 3-choice set for the player, or nullopt if not in prayer. */
    std::optional<std::array<Blessing, 3>> getPendingPrayChoices(int playerId) const;

private:
    World& world;

    std::vector<Position>                              shrinePositions;
    std::unordered_map<int, std::vector<Blessing>>     playerBlessings;
    std::unordered_map<int, std::array<Blessing, 3>>   pendingPrayChoices;
};
