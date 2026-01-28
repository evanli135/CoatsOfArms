#include <iostream>
#include <raylib.h>
#include "model/util.h"
#include "model/world.h"

#include "controller/keyboard.h"

#include "view/tui.h"

int main() {
    InitWindow(800, 450, "raylib is alive");
    SetTargetFPS(60);
    
    // Probability::init(); 

    World model = World();
    KeyboardController controller = KeyboardController();


    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("If you see this, raylib works.", 190, 200, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}