$RAYLIB = "C:/raylib/raylib"
$COMPILER = "C:/raylib/w64devkit/bin/g++"

$SRC = @(
    "src/main.cpp",
    "src/model/world/world.cpp",
    "src/view/tui/tui.cpp",
    "src/controller/keyboard.cpp" 
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