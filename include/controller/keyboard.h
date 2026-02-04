#pragma once

#include <map>
#include <optional>
#include "model/util.h"
#include "model/world.h"
#include "model/error.h"
#include "observer.h"
#include "model/player.h"

enum class KeyboardAction {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    SELECT,
    UNSELECT,
    CONFIRM
};

enum class ControllerAction {
    MOV,
    ATT
};


struct ControllerRequest {
private:
    ControllerAction action;
    Position origin;
    Position destination;
    Player player;

public:
    ControllerAction getAction() { return action; }
    Position getOrigin() { return origin; }
    Position getDestination() { return destination; }
    Player getPlayer() { return player; }

    ControllerRequest(ControllerAction action, Position origin, Position destination, Player player)
     : action(action), origin(origin), destination(destination), player(player) {}
};

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
    std::optional<PlayerError> moveHover(int dRow, int dCol);
    void endTurn();
    std::optional<PlayerError> selectCell();
    // std::optional<PlayerError> applyControllerAction(ControllerAction action);
};