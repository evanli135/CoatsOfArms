#pragma once

#include <optional>
#include "controller/mode_handler.h"

class World;
class Player;

/**
 * ModeHandler for TACTIC mode — handles unit movement and combat.
 *
 * Three-step interaction:
 *   Step 1 — onTileSelect() with no selection: calls selectOrigin().
 *             Requires a friendly unit that has not yet acted this turn.
 *   Step 2 — onActionButton(): choose MOV or ATT. Requires step 1 to be complete.
 *             Step 3 will be rejected until an action is chosen here.
 *   Step 3 — onTileSelect() with a selection: calls selectDestination().
 *             Dispatches the ControllerRequest to the model.
 *             Rejected with NOTSUPPORTED if no action was set in step 2.
 *
 * On a successful request, selection and pendingAction are cleared.
 * On failure, selection is preserved so the player can retry step 3.
 */
class TacticMode : public ModeHandler {
public:
    /**
     * Constructs TacticMode with clean state (no selection, no pending action).
     *
     * @param world   The game world to read unit/tile state from and send requests to.
     * @param player  The player this mode acts on behalf of.
     */
    TacticMode(World& world, const Player& player);

    /**
     * Handles a tile selection in TACTIC mode.
     *
     * If no origin is selected, calls selectOrigin().
     * If an origin is already selected, calls selectDestination().
     *
     * @param pos  The tile that was selected.
     * @return     nullopt on success.
     *             PlayerError from selectOrigin() or selectDestination().
     *
     * State: see selectOrigin() and selectDestination().
     */
    std::optional<PlayerError> onTileSelect(Position pos) override;

    /**
     * Sets the pending action (step 2). Requires a unit to be selected first (step 1).
     *
     * index 0 → MOV (move unit to destination)
     * index 1 → ATT (attack unit at destination)
     *
     * @param index  0-based action button index.
     * @return       nullopt on success.
     *               PlayerError::INVALIDTARGET if no unit is selected yet.
     *               PlayerError::NOTSUPPORTED if index > 1.
     *
     * State: pendingAction = MOV or ATT on success.
     */
    std::optional<PlayerError> onActionButton(int index) override;

    /**
     * Clears selection and pending action.
     *
     * State: selection = nullopt, pendingAction = nullopt.
     */
    void onDeselect() override;

    /**
     * Resets all mode state on mode exit.
     * Called by switchMode() before this mode is replaced.
     *
     * State: selection = nullopt, pendingAction = nullopt.
     */
    void onExit() override;

    /** Returns the currently selected origin tile, or nullopt if none. */
    std::optional<Position>         getSelection()     const override { return selection; }

    /** Returns the pending ControllerAction (MOV or ATT), or nullopt if none set. */
    std::optional<ControllerAction> getPendingAction() const override { return pendingAction; }

    /** Returns {"MOVE", "ATTACK", "CAST"} — the three actions available in TACTIC mode. */
    std::vector<std::string> getActionLabels() const override { return {"MOVE", "ATTACK", "CAST"}; }

    std::vector<bool> getEnabledActions() const override;

private:
    World& world;
    const Player& player;

    std::optional<Position>         selection;
    std::optional<ControllerAction> pendingAction;

    /**
     * Phase 1: validates and records the origin tile.
     *
     * Requires: a unit at pos owned by player that has not yet acted (canMove()).
     *
     * @param pos  Candidate origin tile.
     * @return     nullopt on success.
     *             PlayerError::INVALIDTARGET if no unit, or unit is not owned by player.
     *             PlayerError::UNITCANTMOVE if the unit has already acted this turn.
     *
     * State: selection = pos on success.
     */
    std::optional<PlayerError> selectOrigin(Position pos);

    /**
     * Phase 3: dispatches a ControllerRequest to the model using the stored origin
     * and the previously chosen pendingAction.
     *
     * Requires pendingAction to be set (via onActionButton) before calling.
     * Clears selection and pendingAction on success; preserves them on failure
     * so the player can pick a different destination.
     *
     * @param pos  The destination tile.
     * @return     nullopt on success.
     *             PlayerError::NOTSUPPORTED if no action was selected in step 2.
     *             Any PlayerError returned by World::applyControllerRequest().
     *
     * State: on success — selection = nullopt, pendingAction = nullopt.
     *        on failure — selection and pendingAction unchanged.
     */
    std::optional<PlayerError> selectDestination(Position pos);
};
