#pragma once

#include "model/util.h"
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
    ControllerAction action;
    Position origin;
    Position destination;
    Player player;

    ControllerRequest(ControllerAction action, Position origin, Position destination, Player player)
     : action(action), origin(origin), destination(destination), player(player) {}
    
    ControllerAction getAction() const { return action; }
    Position getOrigin() const { return origin; }
    Position getDestination() const { return destination; }
    const Player& getPlayer() const { return player; }
};