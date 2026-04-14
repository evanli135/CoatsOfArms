#pragma once

#include <optional>
#include <array>
#include <string>
#include <vector>
#include "controller/mode_handler.h"
#include "model/spirit.h"

class World;
class Player;

/**
 * ModeHandler for PRAY mode — shrine interaction to receive spirit boons.
 *
 * Two-phase interaction:
 *   Phase 1 — onTileSelect(): select a unit that is adjacent to a shrine
 *              and has not yet acted this turn.
 *   Phase 2 — onActionButton(0–2): choose one of 3 generated boon offers.
 *
 * On success the unit is exhausted and the chosen boon is recorded on the
 * player.  The boon has no mechanical effect yet (framework only).
 */
class PrayMode : public ModeHandler {
public:
    PrayMode(World& world, const Player& player);

    std::optional<PlayerError> onTileSelect(Position pos) override;
    std::optional<PlayerError> onActionButton(int index) override;
    void onDeselect() override;
    void onExit()     override;

    std::optional<Position>         getSelection()     const override { return selection; }
    std::optional<ControllerAction> getPendingAction() const override { return std::nullopt; }

    /** Phase 1: descriptive labels. Phase 2: the 3 boon names. */
    std::vector<std::string> getActionLabels() const override;

private:
    World& world;
    const Player& player;

    std::optional<Position>         selection;
    std::optional<std::array<Boon, 3>> boonChoices;

    std::optional<PlayerError> selectUnit(Position pos);
};
