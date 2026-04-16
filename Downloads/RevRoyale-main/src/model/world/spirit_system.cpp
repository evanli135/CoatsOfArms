#include "model/spirit_system.h"
#include "model/world.h"

// ---------------------------------------------------------------------------
// Shrine management
// ---------------------------------------------------------------------------

void SpiritSystem::addShrine(Position pos) {
    world.getTileAt(pos).setShrine(true);
    shrinePositions.push_back(pos);
}

bool SpiritSystem::hasShrineAt(Position pos) const {
    return world.getTileAt(pos).hasShrine();
}

bool SpiritSystem::isAdjacentToShrine(Position unitPos) const {
    for (const auto& sp : shrinePositions) {
        int dr = std::abs(unitPos.row() - sp.row());
        int dc = std::abs(unitPos.col() - sp.col());
        if (std::max(dr, dc) <= 1) return true;   // Chebyshev-1
    }
    return false;
}

// ---------------------------------------------------------------------------
// Prayer / blessing flow
// ---------------------------------------------------------------------------

std::array<Blessing, 3> SpiritSystem::preparePrayChoices(
    Position unitPos, const Player& player)
{
    auto choices = generateBlessingChoices(player.getId(), world.getTurn());
    pendingPrayChoices[player.getId()] = choices;
    return choices;
}

std::optional<PlayerError> SpiritSystem::completePray(
    Position unitPos, int blessingIndex, const Player& player)
{
    auto it = pendingPrayChoices.find(player.getId());
    if (it == pendingPrayChoices.end()) return PlayerError::INVALIDTARGET;
    if (blessingIndex < 0 || blessingIndex >= 3) return PlayerError::INVALIDTARGET;

    Unit* unit = world.getUnitAt(unitPos);
    if (!unit) return PlayerError::INVALIDTARGET;
    if (unit->getOwner().getId() != player.getId()) return PlayerError::INVALIDTARGET;
    if (unit->isExhausted()) return PlayerError::INVALIDTARGET;

    // Commit the chosen blessing
    const Blessing& chosen = it->second[blessingIndex];
    playerBlessings[player.getId()].push_back(chosen);
    pendingPrayChoices.erase(it);

    // Prayer exhausts the unit (uses their action for this turn)
    unit->setMoved(true);
    unit->setAttacked(true);

    return std::nullopt;
}

void SpiritSystem::clearPendingPrayChoices(int playerId) {
    pendingPrayChoices.erase(playerId);
}

// ---------------------------------------------------------------------------
// Read-only accessors
// ---------------------------------------------------------------------------

const std::vector<Blessing>& SpiritSystem::getPlayerBlessings(int playerId) const {
    static const std::vector<Blessing> empty;
    auto it = playerBlessings.find(playerId);
    return (it != playerBlessings.end()) ? it->second : empty;
}

std::optional<std::array<Blessing, 3>> SpiritSystem::getPendingPrayChoices(
    int playerId) const
{
    auto it = pendingPrayChoices.find(playerId);
    if (it == pendingPrayChoices.end()) return std::nullopt;
    return it->second;
}
