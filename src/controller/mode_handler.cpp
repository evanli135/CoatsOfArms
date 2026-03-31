#include "controller/mode_handler.h"
#include "controller/modes/tactic_mode.h"
#include "controller/modes/training_mode.h"
#include "controller/modes/building_mode.h"
#include "model/player.h"

std::unique_ptr<ModeHandler> makeModeHandler(
    ControllerMode mode, World& world, const Player& player)
{
    switch (mode) {
        case ControllerMode::TACTIC:   return std::make_unique<TacticMode>(world, player);
        case ControllerMode::TRAINING: return std::make_unique<TrainingMode>(world, player);
        case ControllerMode::BUILDING: return std::make_unique<BuildingMode>(world, player);
    }
    throw std::logic_error("Unknown ControllerMode");
}
