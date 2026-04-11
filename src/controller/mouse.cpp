#include "controller/mouse.h"
#include "controller/modes/tactic_mode.h"
#include "controller/modes/training_mode.h"
#include "controller/modes/building_mode.h"
#include "model/world.h"
#include "model/player.h"
#include "model/unit.h"
#include "raylib.h"

Controller::Controller(World& model, const Player& player)
    : model(model),
      player(player),
      currentMode(ControllerMode::TACTIC),
      mode(std::make_unique<TacticMode>(model, player)),
      myTurn(false) {}

std::optional<PlayerError> Controller::onClick(ClickTarget click) {
    if (!myTurn) return PlayerError::OUTOFTURN;

    return std::visit([&](auto&& target) -> std::optional<PlayerError> {
        using T = std::decay_t<decltype(target)>;
        if constexpr (std::is_same_v<T, Position>) {
            // Unit always takes precedence over city.  If the player clicks a tile
            // that has a friendly unit while not in TACTIC mode, auto-switch first.
            if (model.hasUnitAt(target)) {
                const Unit* u = model.getUnitAt(target);
                if (u && u->sameOwner(player) && currentMode != ControllerMode::TACTIC)
                    switchMode(ControllerMode::TACTIC);
            }
            return mode->onTileSelect(target);
        } else if constexpr (std::is_same_v<T, int>) {
            return mode->onActionButton(target);
        } else {
            switchMode(target);
            return std::nullopt;
        }
    }, click);
}

void Controller::switchMode(ControllerMode next) {
    if (currentMode == next) return;
    mode->onExit();
    currentMode = next;
    mode = makeModeHandler(next, model, player);
    mode->onEnter();
}

void Controller::onModelChanged(const ModelEvent& event) {
    std::visit([&](auto&& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, TurnChangeEvent>) {
            if (e.newPlayerId == player.getId()) go();
        }
        // UnitMovedEvent / UnitDiedEvent: no mouse controller response needed.
    }, event);
}

std::optional<PlayerError> Controller::onUndo() {
    if (!myTurn) return PlayerError::OUTOFTURN;
    model.undoLastCommand();
    return std::nullopt;
}

void Controller::go()      { myTurn = true;  }
void Controller::endTurn() { myTurn = false; }

void Controller::onRightClick() {
    mode->onDeselect();
}

std::optional<PlayerError> Controller::onEndTurn() {
    if (!myTurn) return PlayerError::OUTOFTURN;
    mode->onDeselect();   // clear any pending selection / action before handing off
    endTurn();
    model.nextTurn();
    return std::nullopt;
}

bool pollMouseRightClick() {
    return IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
}
