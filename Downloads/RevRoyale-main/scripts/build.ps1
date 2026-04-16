$RAYLIB = "C:/raylib/raylib"
$COMPILER = "C:/raylib/w64devkit/bin/g++"

# Ensure w64devkit tools (as, ld, etc.) are on PATH so g++ can find them.
$env:PATH = "C:/raylib/w64devkit/bin;" + $env:PATH

$SRC = @(
    "src/main.cpp",
    # Model
    "src/model/world/world.cpp",
    "src/model/world/movement.cpp",
    "src/model/world/battle.cpp",
    "src/model/world/training.cpp",
    "src/model/world/resource_system.cpp",
    "src/model/world/construction_system.cpp",
    "src/model/world/spirit_system.cpp",
    "src/model/world/magic_system.cpp",
    "src/model/unit/unit.cpp",
    "src/model/world/tile.cpp",
    # View — grid
    "src/view/gui.cpp",
    "src/view/grid_view.cpp",
    # View — panels (one file per class)
    "src/view/error_view.cpp",
    "src/view/action_view.cpp",
    "src/view/info_view.cpp",
    # View — sprites (one file per sprite category)
    "src/view/terrain_sprites.cpp",
    "src/view/city_sprite.cpp",
    "src/view/unit_sprites.cpp",
    # View — effects
    "src/view/damage_indicators.cpp",
    "src/view/explosion.cpp",
    "src/view/audio.cpp",
    # View — legacy TUI (kept for reference)
    "src/view/tui/tui.cpp",
    # Controller
    "src/controller/command.cpp",
    "src/controller/keyboard.cpp",
    "src/controller/mouse.cpp",
    "src/controller/mode_handler.cpp",
    "src/controller/modes/tactic_mode.cpp",
    "src/controller/modes/training_mode.cpp",
    "src/controller/modes/building_mode.cpp",
    "src/controller/modes/pray_mode.cpp",
    # View — spirit overlay
    "src/view/spirit_overlay.cpp"
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