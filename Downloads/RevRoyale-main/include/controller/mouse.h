#pragma once

#include <array>
#include <memory>
#include <optional>
#include "controller/mode_handler.h"
#include "controller/observer.h"
#include "model/spirit.h"
#include "model/util.h"
#include "controller/error.h"

class World;
class Player;

/**
 * Returns true if the right mouse button was pressed this frame.
 *
 * Only fires on the frame the button is first pressed (not while held).
 * Intended to trigger Controller::onRightClick() in the game loop.
 *
 * @return  true on the first frame of a right-button press, false otherwise.
 */
bool pollMouseRightClick();


class Controller : public ModelObserver {
public:
    /**
     * Constructs a Controller for the given player, starting in TACTIC mode.
     *
     * @param model   The game world this controller reads from and writes to.
     * @param player  The player this controller acts on behalf of.
     *
     * State: myTurn = false, currentMode = TACTIC, mode = TacticMode.
     */
    Controller(World& model, const Player& player);

    /**
     * Handles a resolved left mouse click.
     *
     * Dispatches to the active mode handler based on the variant type:
     *   Position       → mode->onTileSelect()
     *   int            → mode->onActionButton()
     *   ControllerMode → switchMode()
     *
     * The ClickTarget is produced by GUI::pollClick() each frame.
     *
     * @param click  The resolved click target from GUI::pollClick().
     * @return       nullopt on success.
     *               PlayerError::OUTOFTURN if it is not this player's turn.
     *               Any PlayerError returned by the active mode handler.
     *
     * State: delegates all state changes to the active ModeHandler, or
     *        replaces the active mode on a ControllerMode click.
     */
    std::optional<PlayerError> onClick(ClickTarget click);

    /**
     * Handles a right mouse button press.
     *
     * Always clears the active mode's selection and pending action,
     * regardless of game state or whose turn it is.
     *
     * State: calls mode->onDeselect(), clearing selection and pendingAction
     *        inside the active ModeHandler.
     */
    void onRightClick();

    /**
     * Handles a press of the end-turn button.
     *
     * Ends this player's turn and advances the model to the next player.
     * Observers (including this controller) will receive TURN_CHANGE.
     *
     * @return  nullopt on success.
     *          PlayerError::OUTOFTURN if it is not this player's turn.
     *
     * State: myTurn = false. model.nextTurn() is called, which resets the
     *        moved flags for the next player's units and fires TURN_CHANGE.
     */
    std::optional<PlayerError> onEndTurn();

    /**
     * Switches the controller to a different mode.
     *
     * Calls onExit() on the current mode (clearing its state), constructs
     * the new mode via makeModeHandler(), then calls onEnter() on it.
     * No-ops if next == currentMode.
     *
     * @param next  The mode to switch to.
     *
     * State: currentMode = next, mode is replaced with a fresh ModeHandler.
     */
    void switchMode(ControllerMode next);

    /**
     * Responds to model events broadcast by World::notifyObservers().
     *
     * On TURN_CHANGE: sets myTurn = true if it is now this player's turn.
     *
     * @param event  The model event that was fired.
     *
     * State: may set myTurn = true.
     */
    void onModelChanged(const ModelEvent& event) override;

    /**
     * Undoes the last action taken this turn (if any).
     * @return  nullopt on success.
     *          PlayerError::OUTOFTURN if it is not this player's turn.
     */
    std::optional<PlayerError> onUndo();

    /**
     * Returns the current hover position (the tile the cursor is over).
     * Updated each frame by the game loop via setHoverPosition().
     * nullopt if the cursor has not yet entered the grid.
     */
    const std::optional<Position>& getHoverPosition() const { return hoverPosition; }

    /**
     * Returns the tile currently selected by the active mode, if any.
     * Delegates to mode->getSelection().
     */
    std::optional<Position> getSelectedPosition() const { return mode->getSelection(); }

    /**
     * Returns the pending ControllerAction set by the active mode, if any.
     * Delegates to mode->getPendingAction().
     */
    std::optional<ControllerAction> getPendingAction() const { return mode->getPendingAction(); }

    /**
     * Returns the highlighted button index set by the active mode (e.g. in
     * training mode when a unit type is selected before a city).
     * Delegates to mode->getPendingButtonIndex().
     */
    std::optional<int> getPendingButtonIndex() const { return mode->getPendingButtonIndex(); }

    /**
     * Returns the action button labels for the active mode.
     * Delegates to mode->getActionLabels(). Pass to GUI::pollClick() and
     * GUI::render() each frame so the view knows what buttons to display.
     */
    std::vector<std::string> getActionLabels()  const { return mode->getActionLabels(); }
    std::vector<bool>        getEnabledActions() const { return mode->getEnabledActions(); }

    /** Returns true if it is currently this player's turn. */
    bool isMyTurn() const { return myTurn; }

    /** Returns the currently active ControllerMode. */
    ControllerMode getCurrentMode() const { return currentMode; }

    /** Returns the three pending blessing choices when in PRAY mode phase 2, else nullopt. */
    std::optional<std::array<Blessing, 3>> getPendingBlessingChoices() const {
        return mode->getPendingBlessingChoices();
    }

    /** Returns the spell selected in SPELL_TARGET sub-state, otherwise nullopt. */
    std::optional<SpellId> getSelectedSpell() const { return mode->getSelectedSpell(); }

    /**
     * Updates the stored hover position.
     *
     * Call each frame with the result of GUI::pollHover().
     *
     * @param pos  The grid tile currently under the cursor.
     *
     * State: hoverPosition = pos.
     */
    void setHoverPosition(Position pos) { hoverPosition = pos; }

private:
    World& model;
    const Player& player;

    std::optional<Position>      hoverPosition;
    ControllerMode               currentMode;
    std::unique_ptr<ModeHandler> mode;
    bool myTurn;

    /**
     * Grants this controller its turn.
     * Called internally from onModelChanged on TURN_CHANGE.
     * State: myTurn = true.
     */
    void go();

    /**
     * Revokes this controller's turn.
     * Called internally from onEndTurn().
     * State: myTurn = false.
     */
    void endTurn();
};
