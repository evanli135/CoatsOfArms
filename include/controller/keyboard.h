#pragma once

#include <map>
#include <vector>
#include <optional>

#include "util.h"
#include "world.h"
#include "error.h"
#include "observer.h"

enum class PlayerAction {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    SELECT,
    UNSELECTED,
    CONFIRM
};

class KeyState {
public:
    bool wasPressed;
    bool justPressed;
    bool isHeld;

    void update(bool isDown) {
        wasPressed = !isDown && prevDown;
        justPressed = isDown && !prevDown;
        isHeld = isDown && prevDown;
        prevDown = isDown;
    }

private:
    bool prevDown;
};

class KeyboardController : ModelObserver {
public:
    void go() {
        if (!myTurn)
        myTurn = true;
    }
    

    std::optional<PlayerError> action(PlayerAction playerAction) {
        if (!myTurn) { return PlayerError::OUTOFTURN; }

        switch (playerAction) {
            case (PlayerAction::LEFT):
                return moveHover(0, -1);

            case (PlayerAction::RIGHT):
                return moveHover(0, 1);

            case (PlayerAction::DOWN):
                return moveHover(-1, 0);

            case (PlayerAction::UP):
                return moveHover(1, 0);

            case (PlayerAction::SELECT):
                return selectCell();
            
            case (PlayerAction::UNSELECTED):
                selectedPosition = std::nullopt;
                return std::nullopt;

            case (PlayerAction::CONFIRM):
                endTurn();
                model.nextTurn();

            default:
                throw std::logic_error("Unhandled action");
        }
    }

    void onModelChanged(ModelEvent event) override {
        switch (event) {
            case ModelEvent::TURN_CHANGE:
                if (model.getCurrentPlayer().id() == player.id()) {
                    go();
                }
                break;
            default:
                break;
        }
    }
        

private:
    float repeatDelay = 0.25f;
    float repeatInterval = 0.1f;
    bool myTurn = false;


    Model::World& model;

    std::optional<Position> selectedPosition;

    Position hoverPosition;

    Player& player;

    std::map<int, KeyState> keyStates;
    std::map<int, PlayerAction> keyBinds;


    bool myPlayer(const Unit& unit) const {
        return unit.getPlayer().id() == player.id();
    }

    std::optional<PlayerError> moveHover (int dRow, int dCol) {
        try {
            hoverPosition.move(dRow, dCol);
            return std::nullopt;
        } catch (std::out_of_range) {
            return PlayerError::OUTOFBOUNDS;   
        }
    }

    void endTurn() {
        if (!myTurn) { throw std::logic_error("Not my turn"); }
        myTurn = false;
    }

    std::optional<PlayerError> selectCell() {
        if (!selectedPosition.has_value()) {
            if (model.hasUnitAt(hoverPosition) && myPlayer(model.getUnitAt(hoverPosition).value())) {
                return PlayerError::INVALIDTARGET;
            }

            selectedPosition = hoverPosition;
            return std::nullopt;
        }
        
        Position oldPos = selectedPosition.value();
        Position newPos = hoverPosition;

        if (oldPos == newPos) {
            selectedPosition = std::nullopt;
            return std::nullopt;
        }

        Position pos = selectedPosition.value();

        Tile& oldTile = model.getTileAt(oldPos);
        Tile& newTile = model.getTileAt(newPos);

        if (oldTile.hasUnit()) {
            if (newTile.hasUnit()) {
                const Unit& newUnit = newTile.getUnit().value();
                const Unit& oldUnit = oldTile.getUnit().value();

                if (myPlayer(newUnit)) {
                    return PlayerError::NOTSUPPORTED;
                }                 
                else {
                    model.battle(oldPos, newPos);
                    selectedPosition = std::nullopt;
                    return std::nullopt;
                }
            }
        } else {
            model.moveUnit(oldPos, newPos);
            selectedPosition = std::nullopt;

        }
    }

};