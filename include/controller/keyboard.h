#pragma once

#include <map>
#include <vector>
#include <optional>

#include "include/model/util.h"
#include "include/model/world.h"

enum class PlayerAction {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    SELECT,
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

class KeyboardController {
public:
    void go() {
        myTurn = true;
    }

    void action(PlayerAction playerAction) {
        if (myTurn = false) { throw std::logic_error("Not my turn"); }

        switch (playerAction) {
            case (PlayerAction::LEFT):
                try {
                    hoverPosition.changeRow(-1);
                } catch (std::out_of_range) {
                    //
                }

            case (PlayerAction::RIGHT):
                

            case (PlayerAction::DOWN):

            case (PlayerAction::UP):

            case (PlayerAction::SELECT):

            case (PlayerAction::CONFIRM):

        }
    }
        

private:
    float repeatDelay = 0.25f;
    float repeatInterval = 0.1f;
    bool myTurn = false;


    Model::World& model;
    std::optional<Position> selectedPosition;
    Position hoverPosition;

    std::map<int, KeyState> keyStates;
    std::map<int, PlayerAction> keyBinds;
};