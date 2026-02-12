#pragma once

#include <map>
#include <optional>
#include "controller/action.h"
#include "controller/observer.h"
#include "model/util.h"
#include "model/error.h"
#include "controller/error.h"

class World;
class Player;
class Unit;

class KeyState {
public:
    KeyState() : wasPressed(false), justPressed(false), isHeld(false), prevDown(false) {}
    
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

class KeyboardController : public ModelObserver {
public:
    KeyboardController(World& model, const Player& player);
    
    void go();
    std::optional<PlayerError> applyKeyboardAction(KeyboardAction KeyboardAction);
    void onModelChanged(ModelEvent event) override;
    
    const Position& getHoverPosition() const { return hoverPosition; }
    const std::optional<Position>& getSelectedPosition() const { return selectedPosition; }
    bool isMyTurn() const { return myTurn; }

private:
    World& model;
    const Player& player;
    
    std::optional<Position> selectedPosition;
    Position hoverPosition;
    
    bool myTurn;
    float repeatDelay;
    float repeatInterval;
    
    // Helper methods
    bool myPlayer(const Unit& unit) const;
    bool myPlayer(const Unit* unit) const;
    std::optional<PlayerError> moveHover(int dRow, int dCol);
    void endTurn();
    std::optional<PlayerError> selectCell();
    // std::optional<PlayerError> applyControllerAction(ControllerAction action);
};