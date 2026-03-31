#pragma once

#include <memory>
#include <optional>
#include "controller/action.h"
#include "controller/mode_handler.h"
#include "controller/observer.h"
#include "model/util.h"
#include "controller/error.h"

class World;
class Player;
class Unit;

/**
 * Per-key edge-detection helper.
 *
 * Wraps a raw boolean "is key down" value and derives cleaner per-frame
 * signals: justPressed (first frame down), isHeld (subsequent frames down),
 * wasPressed (first frame after release). Call update() once per frame.
 */
class KeyState {
public:
    KeyState() : wasPressed(false), justPressed(false), isHeld(false), prevDown(false) {}

    bool wasPressed;   // True on the frame the key was released.
    bool justPressed;  // True on the first frame the key is pressed.
    bool isHeld;       // True while the key is held after the first frame.

    /**
     * Advances the key state for the current frame.
     *
     * @param isDown  Whether the physical key is currently held down.
     *
     * State: updates wasPressed, justPressed, isHeld, and prevDown.
     */
    void update(bool isDown) {
        wasPressed  = !isDown && prevDown;
        justPressed =  isDown && !prevDown;
        isHeld      =  isDown &&  prevDown;
        prevDown    =  isDown;
    }

private:
    bool prevDown;
};

/**
 * Keyboard-driven controller. Implements ModelObserver to respond to turn changes.
 *
 * Shares the same ModeHandler architecture as the mouse Controller — all
 * selection and action state lives inside the active ModeHandler, not here.
 * Navigation (hover movement) is handled directly without going through the mode.
 *
 * Intended primarily as a fallback / development input path; the mouse
 * Controller is the target for production use.
 */
class KeyboardController : public ModelObserver {
public:
    /**
     * Constructs a KeyboardController for the given player, starting in TACTIC mode.
     *
     * @param model   The game world this controller reads from and writes to.
     * @param player  The player this controller acts on behalf of.
     *
     * State: myTurn = false, hoverPosition = (0,0), currentMode = TACTIC.
     */
    KeyboardController(World& model, const Player& player);

    /**
     * Grants this controller its turn.
     * Called internally from onModelChanged() when TURN_CHANGE fires
     * and it is this player's turn.
     *
     * State: myTurn = true.
     */
    void go();

    /**
     * Processes a single keyboard action for the current frame.
     *
     * Dispatches based on action type:
     *   WASD / arrows → moveHover()
     *   SELECT        → mode->onTileSelect(hoverPosition)
     *   UNSELECT      → mode->onDeselect()
     *   CONFIRM       → endTurn() + model.nextTurn()
     *   NUM 1–4       → mode->onActionButton(0–3)
     *
     * @param action  The KeyboardAction produced by pollKeyboardAction().
     * @return        nullopt on success.
     *                PlayerError::OUTOFTURN if it is not this player's turn.
     *                Any PlayerError returned by the active mode handler.
     *                Throws std::logic_error on an unhandled action value.
     */
    std::optional<PlayerError> applyKeyboardAction(KeyboardAction action);

    /**
     * Responds to model events broadcast by World::notifyObservers().
     *
     * On TURN_CHANGE: calls go() if it is now this player's turn.
     *
     * @param event  The model event that fired.
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
     * Switches to a different mode using the same lifecycle as Controller::switchMode().
     *
     * Calls onExit() on the current mode, constructs the new mode via
     * makeModeHandler(), then calls onEnter(). No-ops if next == currentMode.
     *
     * @param next  The mode to switch to.
     *
     * State: currentMode = next, mode replaced with a fresh ModeHandler.
     */
    void switchMode(ControllerMode next);

    /**
     * Returns the tile the keyboard cursor is currently hovering over.
     * Updated by moveHover() on directional key presses.
     */
    const Position& getHoverPosition() const { return hoverPosition; }

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
     * Returns true if it is currently this player's turn.
     */
    bool isMyTurn() const { return myTurn; }

private:
    World& model;
    const Player& player;

    Position       hoverPosition;
    ControllerMode currentMode;
    std::unique_ptr<ModeHandler> mode;
    bool myTurn;

    /**
     * Moves the hover position by (dRow, dCol).
     *
     * @param dRow  Row delta (-1, 0, or 1).
     * @param dCol  Column delta (-1, 0, or 1).
     * @return      nullopt on success.
     *              PlayerError::OUTOFBOUNDS if the move would leave the grid.
     *
     * State: hoverPosition updated on success.
     */
    std::optional<PlayerError> moveHover(int dRow, int dCol);

    /**
     * Revokes this controller's turn.
     * Called from applyKeyboardAction() on CONFIRM.
     *
     * State: myTurn = false.
     */
    void endTurn();
};

/**
 * Polls Raylib for a keyboard input this frame and returns the corresponding action.
 *
 * Handles WASD + arrow keys (with auto-repeat for arrows), SPACE/ENTER for SELECT,
 * BACKSPACE/TAB for UNSELECT, LEFT_SHIFT for CONFIRM (with a 1-second cooldown to
 * prevent accidental double end-turns), and NUM keys 1–4.
 *
 * @return  The KeyboardAction for the highest-priority key event this frame,
 *          or nullopt if no relevant key was pressed.
 */
std::optional<KeyboardAction> pollKeyboardAction();
