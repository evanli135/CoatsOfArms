#include "model/world.h"
#include "model/city.h"
#include "model/unit.h"
#include "controller/error.h"

// ---------------------------------------------------------------------------
// TrainingSystem
// ---------------------------------------------------------------------------

std::optional<PlayerError> TrainingSystem::beginTraining(
    const Position& cityPos, UnitType type, const Player& player)
{
    if (!world.hasCityAt(cityPos)) return PlayerError::INVALIDTARGET;

    City* city = world.getTileAt(cityPos).getCityMutable();
    if (!city) return PlayerError::INVALIDTARGET;

    if (!city->hasOwner() || city->getOwner().getId() != player.getId())
        return PlayerError::INVALIDTARGET;

    if (city->isTraining())
        return PlayerError::UNITCANTMOVE;   // city already has a training order

    if (countUnitsForPlayer(player.getId()) >= MAX_UNITS_PER_PLAYER)
        return PlayerError::UNITCANTMOVE;   // unit cap reached

    city->startTraining(type, player.getId(), 2);
    world.notifyObservers(TrainingStartedEvent{type, player.getId(), cityPos});
    return std::nullopt;
}

void TrainingSystem::advanceTraining(int playerId) {
    for (const auto& cpos : world.cityPositions) {
        City* city = world.getTileAt(cpos).getCityMutable();
        if (!city || !city->isTraining()) continue;

        TrainingSlot* slot = city->getTrainingSlotMutable();
        if (slot->ownerId != playerId) continue;

        // Tick down one turn for this player's turn starting.
        if (slot->turnsRemaining > 0)
            slot->turnsRemaining--;

        // Attempt to spawn when countdown reaches zero.
        if (slot->turnsRemaining == 0 && !world.hasUnitAt(cpos)) {
            // Find the owning Player object in the world's player list.
            for (const auto& p : world.players) {
                if (p.getId() == playerId) {
                    world.addUnit(cpos, UnitFactory::create(slot->unitType, p));
                    city->clearTraining();
                    break;
                }
            }
        }
        // If the tile is still occupied when turnsRemaining == 0, the slot
        // stays cleared-to-zero and we retry at the start of each subsequent
        // turn until the tile frees up.
    }
}

int TrainingSystem::countUnitsForPlayer(int playerId) const {
    int count = 0;

    // Fielded units
    for (const auto& [id, unit] : world.units) {
        if (unit->getOwner().getId() == playerId) ++count;
    }

    // Units currently in training queues
    for (const auto& cpos : world.cityPositions) {
        const City* city = world.getCityAt(cpos);
        if (city && city->isTraining() &&
            city->getTrainingSlot()->ownerId == playerId)
            ++count;
    }

    return count;
}
