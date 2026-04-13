#pragma once

#include <optional>
#include "controller/mode_handler.h"

class World;
class Player;

/**
 * ModeHandler for BUILDING mode — handles construction of buildings on tiles.
 *
 * Two-phase interaction:
 *   Phase 1 — onTileSelect(): calls selectOrigin().
 *              Any tile can be selected; ownership/legality is validated by the model.
 *   Phase 2 — onActionButton(): chooses the building type to construct (not yet implemented).
 *
 * NOTE: onActionButton() currently returns NOTSUPPORTED. Full implementation
 * is pending the construction and economy systems in the model.
 */
class BuildingMode : public ModeHandler {
public:
    /**
     * Constructs BuildingMode with clean state (no selection).
     *
     * @param world   The game world to send construction requests to.
     * @param player  The player this mode acts on behalf of.
     */
    BuildingMode(World& world, const Player& player);

    /**
     * Handles a tile selection in BUILDING mode.
     *
     * Always calls selectOrigin() — building has no destination tile,
     * only a target tile and a button to choose the building type.
     *
     * @param pos  The tile that was selected.
     * @return     nullopt on success (always succeeds; model validates on dispatch).
     *
     * State: see selectOrigin().
     */
    std::optional<PlayerError> onTileSelect(Position pos) override;

    /**
     * Chooses the building type to construct at the selected tile.
     *
     * Index-to-building-type mapping is TBD pending construction system implementation.
     * Currently returns NOTSUPPORTED for all indices.
     *
     * @param index  0-based building type index.
     * @return       PlayerError::INVALIDTARGET if no tile has been selected yet.
     *               PlayerError::NOTSUPPORTED always (not yet implemented).
     */
    std::optional<PlayerError> onActionButton(int index) override;

    /**
     * Clears the selected tile.
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

    /** Returns the currently selected construction tile, or nullopt if none. */
    std::optional<Position>         getSelection()     const override { return selection; }

    /** Always returns nullopt — BUILDING mode does not use a pending ControllerAction. */
    std::optional<ControllerAction> getPendingAction() const override { return std::nullopt; }

    /** Returns all constructable building types. */
    std::vector<std::string> getActionLabels() const override {
        return {"Foundry", "Barrack", "Extractor", "Shrine", "Utility"};
    }

private:
    World& world;
    const Player& player;

    std::optional<Position> selection;

    std::optional<PlayerError> selectOrigin(Position pos);
};
