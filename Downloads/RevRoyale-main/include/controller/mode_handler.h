#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "controller/action.h"
#include "controller/error.h"
#include "model/spirit.h"
#include "model/util.h"

class World;
class Player;

/**
 * Interface every controller mode must implement.
 *
 * The controller owns exactly one ModeHandler at a time and forwards all
 * tile and button interactions to it. Each mode owns its own selection and
 * pending-action state — none of that leaks into the controller itself.
 *
 * Interaction follows a two-phase pattern common to all modes:
 *   Phase 1: onTileSelect() with no active selection  → picks an origin
 *   Phase 2: onTileSelect() with an active selection  → picks a destination
 *            OR onActionButton() sets intent before destination is picked
 *
 * Lifecycle: onEnter() is called when the mode becomes active,
 *            onExit()  is called just before it is replaced.
 */
class ModeHandler {
public:
    virtual ~ModeHandler() = default;

    /**
     * Called when the player selects a tile (keyboard SELECT or left mouse click on grid).
     *
     * If no origin is selected, delegates to the mode's selectOrigin() logic.
     * If an origin is already selected, delegates to selectDestination() and
     * dispatches a ControllerRequest to the model.
     *
     * @param pos  The grid tile that was selected.
     * @return     nullopt on success.
     *             PlayerError describing why the selection was rejected.
     *
     * State: may set selection, clear selection + pendingAction on a
     *        successful request, or leave selection unchanged on failure.
     */
    virtual std::optional<PlayerError> onTileSelect(Position pos) = 0;

    /**
     * Called when the player clicks an action button or presses a NUM key.
     *
     * The index is 0-based into whatever action list the active mode exposes.
     * Each mode maps indices to its own ControllerAction or unit/building type.
     *
     * @param index  0-based button index.
     * @return       nullopt on success.
     *               PlayerError::NOTSUPPORTED if the index is out of range
     *               or the action is not yet implemented.
     *               PlayerError::INVALIDTARGET if no origin has been selected yet.
     *
     * State: may set pendingAction.
     */
    virtual std::optional<PlayerError> onActionButton(int index) = 0;

    /**
     * Called when the player presses UNSELECT (keyboard) or right-clicks (mouse).
     *
     * Always clears selection and pendingAction regardless of game state.
     *
     * State: selection = nullopt, pendingAction = nullopt.
     */
    virtual void onDeselect() = 0;

    /**
     * Lifecycle hook — called after this mode is made active by switchMode().
     *
     * Default implementation does nothing. Override to perform setup that
     * must happen after the mode is installed (e.g. pre-computing highlights).
     *
     * State: no state changes in the default implementation.
     */
    virtual void onEnter() {}

    /**
     * Lifecycle hook — called just before this mode is replaced by switchMode().
     *
     * Override to reset any state that must not carry over to the next mode.
     * All three concrete modes override this to clear selection + pendingAction.
     *
     * State: typically resets selection and pendingAction.
     */
    virtual void onExit() {}

    /**
     * Returns the currently selected origin tile, if any.
     * Used by the renderer to highlight the selected cell.
     */
    virtual std::optional<Position> getSelection() const = 0;

    /**
     * Returns the pending ControllerAction, if one has been set via onActionButton().
     * Used by the renderer to indicate which action is queued.
     * Returns nullopt for modes that do not use a pending action (TRAINING, BUILDING).
     */
    virtual std::optional<ControllerAction> getPendingAction() const = 0;

    /**
     * Returns the 0-based index of the currently highlighted action button,
     * if any (used when a button was pressed before an origin was selected).
     * Default returns nullopt; TrainingMode overrides to expose its pending
     * unit-type index.
     */
    virtual std::optional<int> getPendingButtonIndex() const { return std::nullopt; }

    /**
     * Returns the three pending blessing choices when in PRAY mode phase 2,
     * otherwise nullopt.  Default returns nullopt; PrayMode overrides.
     */
    virtual std::optional<std::array<Blessing, 3>> getPendingBlessingChoices() const {
        return std::nullopt;
    }

    /**
     * Returns the display labels for this mode's action buttons, in index order.
     *
     * The view calls this to know what buttons to render in the ActionView and
     * how many button slots to hit-test. Index i in this list corresponds to
     * onActionButton(i). The list is static for a given mode — it does not
     * change after the mode is constructed.
     *
     * @return  Ordered list of button label strings (e.g. {"MOV", "ATT"}).
     */
    virtual std::vector<std::string> getActionLabels() const = 0;

    /**
     * Returns a per-button enabled mask for the current game state.
     * true = available (green), false = unavailable (red).
     * An empty vector means all buttons are enabled.
     */
    virtual std::vector<bool> getEnabledActions() const { return {}; }
};

/**
 * Factory function — constructs and returns the ModeHandler for the given mode.
 *
 * @param mode    The ControllerMode to instantiate.
 * @param world   Reference to the game world (passed into the mode handler).
 * @param player  The player this mode acts on behalf of.
 * @return        A freshly constructed ModeHandler subclass with clean state.
 *
 * Throws std::logic_error if an unknown ControllerMode value is passed.
 */
std::unique_ptr<ModeHandler> makeModeHandler(
    ControllerMode mode, World& world, const Player& player);
