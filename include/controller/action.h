#pragma once

#include <variant>
#include "model/util.h"
#include "model/player.h"

/**
 * Raw input intent produced by a single keypress.
 * Polled each frame by pollKeyboardAction() and passed to
 * KeyboardController::applyKeyboardAction().
 */
enum class KeyboardAction {
    LEFT,     // Move hover left one tile
    RIGHT,    // Move hover right one tile
    UP,       // Move hover up one tile
    DOWN,     // Move hover down one tile
    SELECT,   // Confirm selection on the hovered tile
    UNSELECT, // Clear current selection / cancel pending action
    CONFIRM,  // End the active player's turn
    NUM_1,    // Action button shortcut — index 0
    NUM_2,    // Action button shortcut — index 1
    NUM_3,    // Action button shortcut — index 2
    NUM_4,    // Action button shortcut — index 3
    UNDO      // Undo last action this turn (Z key)
};

/**
 * Semantic game action carried inside a ControllerRequest.
 * Tells the model what operation to perform between origin and destination.
 *
 *   MOV — move a unit from origin to destination
 *   ATT — attack the unit at destination from origin
 *   CON — begin construction of a building at destination (not yet implemented)
 *   TRN — begin training a unit at the city at origin (not yet implemented)
 */
enum class ControllerAction {
    MOV,
    ATT,
    CON,
    TRN,
    CHG   // Cavalry charge — straight-line rush up to 6 tiles
};

/**
 * The interaction mode the controller and action view are currently in.
 * Controls which ModeHandler is active and which action buttons are displayed.
 *
 *   TACTIC   — move and attack with units
 *   TRAINING — train new units at cities
 *   BUILDING — construct buildings on tiles
 */
enum class ControllerMode {
    TACTIC,
    TRAINING,
    BUILDING
};

/**
 * The result of resolving a single mouse click against the screen layout.
 * Exactly one alternative is active at a time:
 *
 *   Position       — the player clicked a grid tile
 *   int            — the player clicked an action button (0-based index;
 *                    meaning is mode-specific)
 *   ControllerMode — the player clicked a mode-switch button
 */
using ClickTarget = std::variant<Position, int, ControllerMode>;

/**
 * A fully-resolved request from the controller to the model.
 * Produced by a ModeHandler after both origin and destination have been
 * selected, then passed to World::applyControllerRequest().
 */
struct ControllerRequest {
    ControllerAction action;
    Position         origin;
    Position         destination;
    Player           player;

    /**
     * @param action       The operation to perform.
     * @param origin       Tile the acting unit currently occupies.
     * @param destination  Target tile (move destination or attack target).
     * @param player       The player issuing the request.
     */
    ControllerRequest(ControllerAction action, Position origin,
                      Position destination, Player player)
        : action(action), origin(origin), destination(destination), player(player) {}

    /** Returns the action to perform. */
    ControllerAction getAction()      const { return action; }

    /** Returns the tile the acting unit is on. */
    Position         getOrigin()      const { return origin; }

    /** Returns the target tile. */
    Position         getDestination() const { return destination; }

    /** Returns the issuing player. */
    const Player&    getPlayer()      const { return player; }
};
