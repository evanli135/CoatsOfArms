#include <iostream>
#include <raylib.h>
#include "model/util.h"
#include "model/world.h"
#include "controller/error.h"

#include "controller/keyboard.h"

#include "view/tui.h"


std::optional<KeyboardAction> pollKeyboardAction() {
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))   return KeyboardAction::LEFT;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))  return KeyboardAction::RIGHT;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))     return KeyboardAction::UP;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))   return KeyboardAction::DOWN;
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) return KeyboardAction::SELECT;
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_TAB))   return KeyboardAction::UNSELECT;
    if (IsKeyPressed(KEY_LEFT_SHIFT))                             return KeyboardAction::CONFIRM;
    return std::nullopt;
};

int main() {
    // SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(1920, 1080, "RevRoyale");  // Standard 720p window
    MaximizeWindow();
    SetTargetFPS(60);
    
    // Probability::init(); 

    Player p1 = Player(0);
    Player p2 = Player(1);

    std::vector<Player> players = {p1, p2};

    World model = WorldFactory::create(WorldLayout::BASIC, players);
    KeyboardController controller1 = KeyboardController(model, p1);
    KeyboardController controller2 = KeyboardController(model, p2);
    TUI view = TUI(GetScreenWidth(), GetScreenHeight()); // Last here
    
    model.addObserver(&controller1);
    model.addObserver(&controller2);
    model.startGame();
 
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_Q)) {
            break;  // Exit game loop
        }

        KeyboardController& activeController = controller1.isMyTurn() ? controller1 : controller2;

        if (auto action = pollKeyboardAction()) { 

            auto error = activeController.applyKeyboardAction(*action);
            
            if (error) {
                std::cout << "Action Failed: " << playerErrorToString(error.value()) << "\n";
                view.setError(error.value());
            } else { view.clearError(); }

        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (activeController.getSelectedPosition().has_value()) {
            const Position& pos = activeController.getSelectedPosition().value();
            view.render(
                model,
                activeController.getHoverPosition(),
                &activeController.getSelectedPosition().value()
            );

        } else {
            view.render(
                model,
                activeController.getHoverPosition(),
                nullptr
            );
        }

        // const std::optional<Unit*> selectedUnit = activeController.getSelectedPosition().has_value() 
        //     ? model.getUnitAt( activeController.getSelectedPosition().value() )  
        //     : std::nullopt;     

        // view.render(model, 
        //     activeController.getHoverPosition(), 
        //     model.getUnitAt(activeController.getSelectedPosition()), 
        //     model.getCurrentPlayer().getId());


        EndDrawing();
    }

    CloseWindow();
    return 0;
}