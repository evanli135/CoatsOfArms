#pragma once

#include <optional>
#include "controller/mode_handler.h"

class World;
class Player;

/**
 * ModeHandler for TRAINING mode — handles unit training at cities.
 *
 * Two-phase interaction:
 *   Phase 1 — onTileSelect(): calls selectOrigin().
 *              Requires a city at the tile that is owned by the active player.
 *   Phase 2 — onActionButton(): chooses the unit type to train (not yet implemented).
 *
 * NOTE: onActionButton() currently returns NOTSUPPORTED. Full implementation
 * is pending the economy and unit-training systems in the model.
 */
class TrainingMode : public ModeHandler {
public:
    /**
     * Constructs TrainingMode with clean state (no selection).
     *
     * @param world   The game world to read city state from.
     * @param player  The player this mode acts on behalf of.
     */
    TrainingMode(World& world, const Player& player);

    /**
     * Handles a tile selection in TRAINING mode.
     *
     * Always calls selectOrigin() — training has no destination tile,
     * only a city to train from and a button to choose the unit type.
     *
     * @param pos  The tile that was selected.
     * @return     nullopt on success.
     *             PlayerError from selectOrigin().
     *
     * State: see selectOrigin().
     */
    std::optional<PlayerError> onTileSelect(Position pos) override;

    /**
     * Chooses the unit type to train at the selected city.
     *
     * Index-to-unit-type mapping is TBD pending training system implementation.
     * Currently returns NOTSUPPORTED for all indices.
     *
     * @param index  0-based unit type index.
     * @return       PlayerError::INVALIDTARGET if no city has been selected yet.
     *               PlayerError::NOTSUPPORTED always (not yet implemented).
     */
    std::optional<PlayerError> onActionButton(int index) override;

    /**
     * Clears the selected city.
     *
     * State: selection = nullopt.
     */
    void onDeselect() override;

    /**
     * Resets all mode state on mode exit.
     * Called by switchMode() before this mode is replaced.
     *
     * State: selection = nullopt.
     */
    void onExit() override;

    /** Returns the currently selected city tile, or nullopt if none. */
    std::optional<Position>         getSelection()     const override { return selection; }

    /** Always returns nullopt — TRAINING mode does not use a pending ControllerAction. */
    std::optional<ControllerAction> getPendingAction() const override { return std::nullopt; }

    /** Returns the selected unit-type button index (button-first flow), or nullopt. */
    std::optional<int> getPendingButtonIndex() const override { return pendingUnitIndex; }

    std::vector<std::string> getActionLabels() const override {
        return {"Warrior", "Scout", "Ranger", "Cavalry", "Mage"};
    }

    std::vector<bool> getEnabledActions() const override;

private:
    World& world;
    const Player& player;

    std::optional<Position> selection;       // city chosen first
    std::optional<int>      pendingUnitIndex; // unit type chosen first

    /**
     * Phase 1: validates and records the origin city tile.
     *
     * Requires: a city at pos owned by the active player.
     *
     * @param pos  Candidate city tile.
     * @return     nullopt on success.
     *             PlayerError::INVALIDTARGET if no city at pos,
     *             or the city is not owned by this player.
     *
     * State: selection = pos on success.
     */
    std::optional<PlayerError> selectOrigin(Position pos);
};
