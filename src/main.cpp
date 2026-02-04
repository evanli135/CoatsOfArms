#include <iostream>
#include <raylib.h>
#include "model/util.h"
#include "model/world.h"

#include "controller/keyboard.h"

#include "view/tui.h"


std::optional<KeyboardAction> pollKeyboardAction() {
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))   return KeyboardAction::LEFT;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))  return KeyboardAction::RIGHT;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))     return KeyboardAction::UP;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))   return KeyboardAction::DOWN;
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) return KeyboardAction::SELECT;
    if (IsKeyPressed(KEY_ESCAPE))                        return KeyboardAction::UNSELECT;
    if (IsKeyPressed(KEY_LEFT_SHIFT))                             return KeyboardAction::CONFIRM;
};

int main() {
    InitWindow(800, 450, "raylib is alive");
    SetTargetFPS(60);
    
    // Probability::init(); 


    Player p1 = Player(1);
    Player p2 = Player(2);

    World model = World();
    KeyboardController controller1 = KeyboardController(model, p1);
    KeyboardController controller2 = KeyboardController(model, p2);
    TUI view = TUI(0, 0); // Last here
 
    while (!WindowShouldClose()) {
        controller1.go();
        controller2.go();

        KeyboardController& activeController = controller1.isMyTurn() ? controller1 : controller2;

        if (auto action = pollKeyboardAction()) {

            auto error = activeController.applyKeyboardAction(*action);

            if (error) {
                std::cout << "Action Failed\n";
            }

        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // const std::optional<Unit*> selectedUnit = activeController.getSelectedPosition().has_value() 
        //     ? model.getUnitAt( activeController.getSelectedPosition().value() )  
        //     : std::nullopt;     

        // view.render(model, 
        //     activeController.getHoverPosition(), 
        //     model.getUnitAt(activeController.getSelectedPosition()), 
        //     model.getCurrentPlayer().getId());


        DrawText("If you see this, raylib works.", 190, 200, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}