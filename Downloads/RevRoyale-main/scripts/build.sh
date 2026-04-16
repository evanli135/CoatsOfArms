#!/usr/bin/env bash
set -e

RAYLIB_INC="/opt/homebrew/include"
RAYLIB_LIB="/opt/homebrew/lib"

SRC=(
    src/main.cpp
    # Model
    src/model/world/world.cpp
    src/model/world/movement.cpp
    src/model/world/battle.cpp
    src/model/world/training.cpp
    src/model/world/resource_system.cpp
    src/model/world/construction_system.cpp
    src/model/world/spirit_system.cpp
    src/model/world/magic_system.cpp
    src/model/world/blessing_system.cpp
    src/model/unit/unit.cpp
    src/model/world/tile.cpp
    # View
    src/view/gui.cpp
    src/view/grid_view.cpp
    src/view/error_view.cpp
    src/view/action_view.cpp
    src/view/info_view.cpp
    src/view/terrain_sprites.cpp
    src/view/city_sprite.cpp
    src/view/unit_sprites.cpp
    src/view/damage_indicators.cpp
    src/view/explosion.cpp
    src/view/audio.cpp
    src/view/tui/tui.cpp
    src/view/spirit_overlay.cpp
    # Controller
    src/controller/command.cpp
    src/controller/keyboard.cpp
    src/controller/mouse.cpp
    src/controller/mode_handler.cpp
    src/controller/modes/tactic_mode.cpp
    src/controller/modes/training_mode.cpp
    src/controller/modes/building_mode.cpp
    src/controller/modes/pray_mode.cpp
)

mkdir -p bin

SDK=/Library/Developer/CommandLineTools/SDKs/MacOSX26.4.sdk

g++ -std=c++17 "${SRC[@]}" \
    -I"include" \
    -I"$RAYLIB_INC" \
    -I"$SDK/usr/include/c++/v1" \
    -I"$SDK/usr/include" \
    -L"$RAYLIB_LIB" \
    -L"$SDK/usr/lib" \
    -lraylib \
    -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo \
    -o bin/revroyale

echo "Build successful!"
