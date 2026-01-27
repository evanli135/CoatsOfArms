#include <iostream>
#include <raylib.h>
#include "include/model/util.h"

int main() {
    InitWindow(800, 450, "raylib is alive");
    SetTargetFPS(60);
    
    Probability::init(); 

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("If you see this, raylib works.", 190, 200, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}