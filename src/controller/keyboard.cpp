#include "controller/keyboard.h"
#include "controller/error.h"
#include "model/world.h"
#include <stdexcept>

KeyboardController::KeyboardController(World& model, const Player& player)
    : model(model),
      player(player),
      hoverPosition(0, 0),
      myTurn(false),
      repeatDelay(0.25f),
      repeatInterval(0.1f) {}

void KeyboardController::go() {
    myTurn = true;
}

std::optional<PlayerError> KeyboardController::applyKeyboardAction(KeyboardAction keyboardAction) {
    if (!myTurn) {
        return PlayerError::OUTOFTURN;
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
    // No unit selected yet - try to select one
    if (!selectedPosition.has_value()) {
        if (!model.hasUnitAt(hoverPosition)) {
            return PlayerError::INVALIDTARGET;
        }
        
        const Unit* unit = model.getUnitAt(hoverPosition);

        if (!myPlayer(unit)) {
            return PlayerError::INVALIDTARGET;
        }

        selectedPosition = hoverPosition;
        return std::nullopt;
    }
    
    // Unit already selected - handle move/attack/deselect
    Position oldPos = selectedPosition.value();
    Position newPos = hoverPosition;

    // Clicking same position - deselect
    if (oldPos == newPos) {
        selectedPosition = std::nullopt;
        return std::nullopt;
    }

    Tile& oldTile = model.getTileAt(oldPos);
    Tile& newTile = model.getTileAt(newPos);

    // Source tile should have a unit
    if (!oldTile.hasUnit()) {
        selectedPosition = std::nullopt;
        return PlayerError::INVALIDTARGET;
    }

    const Unit& oldUnit = oldTile.getUnit().value();

    std::optional<PlayerError> result;

    // Target tile has a unit - handle combat or friendly selection
    if (newTile.hasUnit()) {
        const Unit& newUnit = newTile.getUnit().value();

        // Trying to attack friendly unit (prob for heal, buff, etc)
        if (myPlayer(newUnit)) {
            return PlayerError::NOTSUPPORTED;
        }
        
        // Attack enemy unit
        result = model.applyControllerRequest(
            ControllerRequest(
                ControllerAction::ATT,
                oldPos,
                newPos,
                player
            )
        );
        
        if (!result.has_value()) {
            selectedPosition = std::nullopt;
        }
        return result;
    }

    // Target tile is empty - move unit
    result = model.applyControllerRequest(
            ControllerRequest(
                ControllerAction::MOV,
                oldPos,
                newPos,
                player
            )
        );
    if (!result.has_value()) {
        selectedPosition = std::nullopt;
    }
    return result;
}

bool KeyboardController::myPlayer(const Unit& unit) const {
    { return unit.getOwner().getId() == player.getId(); }
}

bool KeyboardController::myPlayer(const Unit* unit) const {
    { return unit->getOwner().getId() == player.getId(); }
}
