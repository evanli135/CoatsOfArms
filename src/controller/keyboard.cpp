#include "controller/keyboard.h"
#include "controller/error.h"
#include "model/world.h"

#include <iostream>
#include "raylib.h"

#include <stdexcept>

KeyboardController::KeyboardController(World& model, const Player& player)
    : model(model),
      player(player),
      hoverPosition(0, 0),
      myTurn(false),
      repeatDelay(0.25f),
      repeatInterval(0.1f),
      waitingForActionInput(false) {}

void KeyboardController::go() {
    myTurn = true;
}

std::optional<PlayerError> KeyboardController::applyKeyboardAction(KeyboardAction keyboardAction) {
    if (!myTurn) {
        return PlayerError::OUTOFTURN;
    }

    if (waitingForActionInput) {
        if (!selectedPosition.has_value()) { throw std::logic_error("No cell selected for action"); }

        Tile tile = model.getTileAt(selectedPosition.value());

        if (tile.hasUnit() && tile.getUnit()->sameOwner(player)) {
            if (keyboardAction == KeyboardAction::NUM_1) {
                pendingAction = ControllerAction::MOV;
            } else if (keyboardAction == KeyboardAction::NUM_2) {
                pendingAction = ControllerAction::ATT;
            } else {
                return PlayerError::NOTSUPPORTED;
            }

            pendingAction = ControllerAction::MOV;
        } else if (tile.hasUnit() && !tile.getUnit()->sameOwner(player)) {
            return PlayerError::INVALIDTARGET;
        } else if (tile.hasCity()) {
            if (keyboardAction == KeyboardAction::NUM_1) {
                pendingAction = ControllerAction::CON;
            } else if (keyboardAction == KeyboardAction::NUM_2) {
                pendingAction = ControllerAction::TRN;
            }
        } else {
            if (keyboardAction == KeyboardAction::NUM_1) {
                pendingAction = ControllerAction::CON;
            }
        }
    }

    switch (keyboardAction) {
        case KeyboardAction::LEFT:
            return moveHover(0, -1);

        case KeyboardAction::RIGHT:
            return moveHover(0, 1);

        case KeyboardAction::DOWN:
            return moveHover(1, 0);

        case KeyboardAction::UP:
            return moveHover(-1, 0);

        case KeyboardAction::SELECT:
            return selectCell();
        
        case KeyboardAction::UNSELECT:
            selectedPosition = std::nullopt;
            return std::nullopt;

        case KeyboardAction::CONFIRM:
            endTurn();
            model.nextTurn();
            return std::nullopt;

        default:
            throw std::logic_error("Unhandled action");
    }
}

void KeyboardController::onModelChanged(ModelEvent event) {
    switch (event) {
        case ModelEvent::TURN_CHANGE:
            if (model.getCurrentPlayer().getId() == player.getId()) {
                go();
            }
            break;
        default:
            break;
    }
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
    if (!myTurn) {
        throw std::logic_error("Not my turn");
    }
    myTurn = false;
}

std::optional<PlayerError> KeyboardController::selectCell() {
    if (!pendingAction.has_value()) {
        Tile tile = model.getTileAt(hoverPosition);

        if (tile.hasUnit()) {
            if (!tile.getUnit()->isAlive()) { throw std::logic_error("Unit is dead"); }
            if (tile.getUnit()->sameOwner(player)) { return PlayerError::INVALIDTARGET; } 

            waitingForActionInput = true;
            selectedPosition = hoverPosition;
            return std::nullopt;
        }

        else if (tile.hasCity()) {
            waitingForActionInput = true;
            selectedPosition = hoverPosition;
            return std::nullopt;

        } else {
            waitingForActionInput = true;
            selectedPosition = hoverPosition;
            return std::nullopt;

        }


        return std::nullopt;
    }

    Tile origin = model.getTileAt(selectedPosition.value());
    Tile destination = model.getTileAt(hoverPosition);
    ControllerAction action = pendingAction.value();

    return model.applyControllerRequest(ControllerRequest(action, selectedPosition.value(), hoverPosition, player));
}

bool KeyboardController::myPlayer(const Unit& unit) const {
    { return unit.getOwner().getId() == player.getId(); }
}

bool KeyboardController::myPlayer(const Unit* unit) const {
    { return unit->getOwner().getId() == player.getId(); }
}

std::optional<KeyboardAction> pollKeyboardAction() {
    static float keyRepeatTimer = 0.0f;
    static float repeatDelay = 0.3f;      // Initial delay before repeat starts
    static float repeatInterval = 0.05f;  // Time between repeats
    static bool isRepeating = false;
    
    static float turnCooldown = 0.0f;     // Cooldown for turn changes
    
    float deltaTime = GetFrameTime();
    
    // Update cooldown
    if (turnCooldown > 0.0f) {
        turnCooldown -= deltaTime;
    }
    
    // Non-repeating keys (WASD and action keys)
    if (IsKeyPressed(KEY_W)) return KeyboardAction::UP;
    if (IsKeyPressed(KEY_A)) return KeyboardAction::LEFT;
    if (IsKeyPressed(KEY_S)) return KeyboardAction::DOWN;
    if (IsKeyPressed(KEY_D)) return KeyboardAction::RIGHT;
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) return KeyboardAction::SELECT;
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_TAB)) return KeyboardAction::UNSELECT;
    
    // Turn change with cooldown
    if (IsKeyPressed(KEY_LEFT_SHIFT) && turnCooldown <= 0.0f) {
        turnCooldown = 1.0f;  // 1 second cooldown
        return KeyboardAction::CONFIRM;
    }
    
    // Arrow keys with repeat - check for initial press first
    if (IsKeyPressed(KEY_LEFT)) {
        keyRepeatTimer = 0.0f;
        isRepeating = false;
        return KeyboardAction::LEFT;
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        keyRepeatTimer = 0.0f;
        isRepeating = false;
        return KeyboardAction::RIGHT;
    }
    if (IsKeyPressed(KEY_UP)) {
        keyRepeatTimer = 0.0f;
        isRepeating = false;
        return KeyboardAction::UP;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        keyRepeatTimer = 0.0f;
        isRepeating = false;
        return KeyboardAction::DOWN;
    }
    
    // Check for held arrow keys (with repeat)
    bool arrowKeyHeld = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) ||
                        IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN);
    
    if (arrowKeyHeld) {
        keyRepeatTimer += deltaTime;
        
        float threshold = isRepeating ? repeatInterval : repeatDelay;
        
        if (keyRepeatTimer >= threshold) {
            keyRepeatTimer = 0.0f;
            isRepeating = true;
            
            // Return the held arrow direction
            if (IsKeyDown(KEY_LEFT)) return KeyboardAction::LEFT;
            if (IsKeyDown(KEY_RIGHT)) return KeyboardAction::RIGHT;
            if (IsKeyDown(KEY_UP)) return KeyboardAction::UP;
            if (IsKeyDown(KEY_DOWN)) return KeyboardAction::DOWN;
        }
    } else {
        keyRepeatTimer = 0.0f;
        isRepeating = false;
    }
    
    return std::nullopt;
}