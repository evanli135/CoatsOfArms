#include "controller/keyboard.h"
#include "controller/modes/tactic_mode.h"
#include "controller/modes/training_mode.h"
#include "controller/modes/building_mode.h"
#include "model/world.h"
#include "model/player.h"
#include "raylib.h"

#include <stdexcept>

KeyboardController::KeyboardController(World& model, const Player& player)
    : model(model),
      player(player),
      hoverPosition(0, 0),
      currentMode(ControllerMode::TACTIC),
      mode(std::make_unique<TacticMode>(model, player)),
      myTurn(false) {}

void KeyboardController::go() { myTurn = true; }

std::optional<PlayerError> KeyboardController::applyKeyboardAction(KeyboardAction action) {
    if (!myTurn) return PlayerError::OUTOFTURN;

    switch (action) {
        // --- Navigation (mode-agnostic) ---
        case KeyboardAction::LEFT:    return moveHover(0, -1);
        case KeyboardAction::RIGHT:   return moveHover(0,  1);
        case KeyboardAction::UP:      return moveHover(-1, 0);
        case KeyboardAction::DOWN:    return moveHover( 1, 0);

        // --- Tile interaction ---
        case KeyboardAction::SELECT:
            return mode->onTileSelect(hoverPosition);

        case KeyboardAction::UNSELECT:
            mode->onDeselect();
            return std::nullopt;

        // --- Turn management ---
        case KeyboardAction::CONFIRM:
            endTurn();
            model.nextTurn();
            return std::nullopt;

        // --- Action button shortcuts (NUM keys map to 0-based action indices) ---
        case KeyboardAction::NUM_1: return mode->onActionButton(0);
        case KeyboardAction::NUM_2: return mode->onActionButton(1);
        case KeyboardAction::NUM_3: return mode->onActionButton(2);
        case KeyboardAction::NUM_4: return mode->onActionButton(3);

        // --- Undo ---
        case KeyboardAction::UNDO: return onUndo();

        default:
            throw std::logic_error("Unhandled KeyboardAction");
    }
}

void KeyboardController::switchMode(ControllerMode next) {
    if (currentMode == next) return;
    mode->onExit();
    currentMode = next;
    mode = makeModeHandler(next, model, player);
    mode->onEnter();
}

void KeyboardController::onModelChanged(const ModelEvent& event) {
    std::visit([&](auto&& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, TurnChangeEvent>) {
            if (e.newPlayerId == player.getId()) go();
        }
        // UnitMovedEvent / UnitDiedEvent: no keyboard controller response needed.
    }, event);
}

std::optional<PlayerError> KeyboardController::onUndo() {
    if (!myTurn) return PlayerError::OUTOFTURN;
    model.undoLastCommand();
    return std::nullopt;
}

std::optional<PlayerError> KeyboardController::moveHover(int dRow, int dCol) {
    try {
        hoverPosition.move(dRow, dCol);
        return std::nullopt;
    } catch (const std::out_of_range&) {
        return PlayerError::OUTOFBOUNDS;
    }
}

void KeyboardController::endTurn() {
    if (!myTurn) throw std::logic_error("Not my turn");
    myTurn = false;
}

// ---------------------------------------------------------------------------
// Free function: poll Raylib for a keyboard action this frame.
// ---------------------------------------------------------------------------
std::optional<KeyboardAction> pollKeyboardAction() {
    static float keyRepeatTimer  = 0.0f;
    static float repeatDelay     = 0.3f;
    static float repeatInterval  = 0.05f;
    static bool  isRepeating     = false;
    static float turnCooldown    = 0.0f;

    float deltaTime = GetFrameTime();

    if (turnCooldown > 0.0f) turnCooldown -= deltaTime;

    // Non-repeating action keys
    if (IsKeyPressed(KEY_W))     return KeyboardAction::UP;
    if (IsKeyPressed(KEY_A))     return KeyboardAction::LEFT;
    if (IsKeyPressed(KEY_S))     return KeyboardAction::DOWN;
    if (IsKeyPressed(KEY_D))     return KeyboardAction::RIGHT;

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))    return KeyboardAction::SELECT;
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_TAB))  return KeyboardAction::UNSELECT;

    if (IsKeyPressed(KEY_ONE))   return KeyboardAction::NUM_1;
    if (IsKeyPressed(KEY_TWO))   return KeyboardAction::NUM_2;
    if (IsKeyPressed(KEY_THREE)) return KeyboardAction::NUM_3;
    if (IsKeyPressed(KEY_FOUR))  return KeyboardAction::NUM_4;

    if (IsKeyPressed(KEY_Z)) return KeyboardAction::UNDO;

    if (IsKeyPressed(KEY_LEFT_SHIFT) && turnCooldown <= 0.0f) {
        turnCooldown = 1.0f;
        return KeyboardAction::CONFIRM;
    }

    // Arrow keys with initial-press + auto-repeat
    if (IsKeyPressed(KEY_LEFT))  { keyRepeatTimer = 0; isRepeating = false; return KeyboardAction::LEFT;  }
    if (IsKeyPressed(KEY_RIGHT)) { keyRepeatTimer = 0; isRepeating = false; return KeyboardAction::RIGHT; }
    if (IsKeyPressed(KEY_UP))    { keyRepeatTimer = 0; isRepeating = false; return KeyboardAction::UP;    }
    if (IsKeyPressed(KEY_DOWN))  { keyRepeatTimer = 0; isRepeating = false; return KeyboardAction::DOWN;  }

    bool arrowHeld = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) ||
                     IsKeyDown(KEY_UP)   || IsKeyDown(KEY_DOWN);

    if (arrowHeld) {
        keyRepeatTimer += deltaTime;
        float threshold = isRepeating ? repeatInterval : repeatDelay;

        if (keyRepeatTimer >= threshold) {
            keyRepeatTimer = 0;
            isRepeating    = true;

            if (IsKeyDown(KEY_LEFT))  return KeyboardAction::LEFT;
            if (IsKeyDown(KEY_RIGHT)) return KeyboardAction::RIGHT;
            if (IsKeyDown(KEY_UP))    return KeyboardAction::UP;
            if (IsKeyDown(KEY_DOWN))  return KeyboardAction::DOWN;
        }
    } else {
        keyRepeatTimer = 0;
        isRepeating    = false;
    }

    return std::nullopt;
}
