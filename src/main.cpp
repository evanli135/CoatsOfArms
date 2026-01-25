#include <iostream>
#include <raylib.h>

int main() {
    InitWindow(800, 450, "raylib is alive");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("If you see this, raylib works.", 190, 200, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}