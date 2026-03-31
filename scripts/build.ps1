$RAYLIB = "C:/raylib/raylib"
$COMPILER = "C:/raylib/w64devkit/bin/g++"

$SRC = @(
    "src/main.cpp",
    "src/model/world/world.cpp",
    "src/model/world/movement.cpp",
    "src/model/world/battle.cpp",
    "src/view/tui/tui.cpp",
    "src/view/gui.cpp",
    "src/view/grid_view.cpp",
    "src/view/panel_views.cpp",
    "src/view/terrain_sprites.cpp",
    "src/view/unit_sprites.cpp",
    "src/controller/command.cpp",
    "src/controller/keyboard.cpp",
    "src/controller/mouse.cpp",
    "src/controller/mode_handler.cpp",
    "src/controller/modes/tactic_mode.cpp",
    "src/controller/modes/training_mode.cpp",
    "src/controller/modes/building_mode.cpp"
)

$OUT = "bin/revroyale.exe"

# Create bin directory if it doesn't exist
if (-not (Test-Path "bin")) {
    New-Item -ItemType Directory -Path "bin"
}

& $COMPILER $SRC -o $OUT `
  -I"include" `
  -I"$RAYLIB/include" `
  -L"$RAYLIB/lib" `
  -lraylib -lopengl32 -lgdi32 -lwinmm `
  -std=c++17

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful!" -ForegroundColor Green
} else {
    Write-Host "Build failed!" -ForegroundColor Red
}