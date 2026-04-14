#include "controller/modes/pray_mode.h"
#include "model/world.h"
#include "model/player.h"
#include "model/spirit.h"
#include <cstring>
#include <cstdio>

PrayMode::PrayMode(World& world, const Player& player)
    : world(world), player(player) {}

// ---------------------------------------------------------------------------
// Phase 1 — select the unit that will pray
// ---------------------------------------------------------------------------

std::optional<PlayerError> PrayMode::selectUnit(Position pos) {
    // Must have a friendly, non-exhausted unit
    if (!world.hasUnitAt(pos)) return PlayerError::INVALIDTARGET;
    const Unit* unit = world.getUnitAt(pos);
    if (!unit || unit->getOwner().getId() != player.getId())
        return PlayerError::INVALIDTARGET;
    if (unit->isExhausted()) return PlayerError::INVALIDTARGET;

    // Unit must be adjacent to (or on) a shrine
    if (!world.isAdjacentToShrine(pos)) return PlayerError::INVALIDTARGET;

    // Generate and cache the boon choices
    selection    = pos;
    boonChoices  = world.preparePrayChoices(pos, player);
    return std::nullopt;
}

std::optional<PlayerError> PrayMode::onTileSelect(Position pos) {
    if (!selection.has_value()) {
        return selectUnit(pos);
    }
    // Clicking a tile in phase 2 just re-selects origin
    selection.reset();
    boonChoices.reset();
    world.clearPendingPrayChoices(player.getId());
    return selectUnit(pos);
}

// ---------------------------------------------------------------------------
// Phase 2 — choose one of the 3 boons
// ---------------------------------------------------------------------------

std::optional<PlayerError> PrayMode::onActionButton(int index) {
    if (!selection.has_value() || !boonChoices.has_value())
        return PlayerError::INVALIDTARGET;
    if (index < 0 || index >= 3)
        return PlayerError::NOTSUPPORTED;

    auto result = world.completePray(*selection, index, player);
    if (!result.has_value()) {
        selection.reset();
        boonChoices.reset();
    }
    return result;
}

// ---------------------------------------------------------------------------

void PrayMode::onDeselect() {
    if (selection.has_value())
        world.clearPendingPrayChoices(player.getId());
    selection.reset();
    boonChoices.reset();
}

void PrayMode::onExit() {
    onDeselect();
}

std::vector<std::string> PrayMode::getActionLabels() const {
    if (!boonChoices.has_value()) {
        // Phase 1: hint labels (up to 3 informational slots)
        return {
            "Select unit",
            "near shrine",
            "(Chebyshev 1)"
        };
    }

    // Phase 2: show the 3 generated boon names
    const auto& c = *boonChoices;
    std::vector<std::string> labels;
    labels.reserve(3);
    for (int i = 0; i < 3; ++i) {
        // Format: "Flame: Sear (Warrior)"
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s: %s (%s)",
                      spiritName(c[i].spirit),
                      boonEffectName(c[i].effect),
                      unitTypeName(c[i].targetUnit));
        labels.push_back(buf);
    }
    return labels;
}
