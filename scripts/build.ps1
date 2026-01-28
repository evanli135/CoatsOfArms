$RAYLIB = "C:/raylib/raylib"
$COMPILER = "C:/raylib/w64devkit/bin/g++"

$SRC = @(
    "src/main.cpp"
    # "src/model/world.cpp",
    # "src/model/tile.cpp",
    # "src/model/unit.cpp",
    # "src/model/player.cpp",
    # "src/view/tui.cpp",
    # "src/controller/game.cpp",
    # "src/controller/input_handler.cpp"
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