$RAYLIB = "C:/raylib/raylib"
$COMPILER = "C:/raylib/w64devkit/bin/g++"

$SRC = "src/main.cpp"
$OUT = "bin/revroyale.exe"

& $COMPILER $SRC -o $OUT `
  -I"$RAYLIB/include" `
  -L"$RAYLIB/lib" `
  -lraylib -lopengl32 -lgdi32 -lwinmm