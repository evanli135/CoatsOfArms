#include "view/tui.h"
#include "model/util.h"
#include <string>

TUI::TUI(int screenWidth, int screenHeight)
    : screenWidth(screenWidth), 
      screenHeight(screenHeight),
      fontSize(24),
      cellWidth(40),
      cellHeight(40),
      gridOffsetX(50),
      gridOffsetY(80) 
    {}

void TUI::render(
    const World& world,
    const Position& cursor,
    const Unit* selectedUnit,
    int currentPlayer
) {
    ClearBackground(Color{20, 20, 30, 255});
    
    // Title
    DrawText("POLYTOPIA CLONE", screenWidth / 2 - 150, 20, 32, RAYWHITE);

    // Current player indicator
    Color playerColor = getPlayerColor(currentPlayer);
    DrawText(TextFormat("Player %d's Turn", currentPlayer + 1), screenWidth / 2 - 80, 55, 20, playerColor);

    renderGrid(world, cursor);
    renderInfoPanel(world, selectedUnit, currentPlayer);
    renderControls();

    DrawFPS(10, 10);
}

void TUI::renderGrid(const World& world, const Position& cursor) {
    int worldWidth = Game::WIDTH;
    int worldHeight = Game::HEIGHT;
}


void TUI::renderCell(const World& world, const Position& pos, bool isCursor) {
    int x = gridOffsetX + pos.col() * cellWidth;
    int y = gridOffsetY + pos.row() * cellHeight;

    const Tile& tile = world.getTileAt(pos);

    Color bgColor = getTerrainColor(tile.getTerrain());

    if (isCursor) {
        bgColor.r = (unsigned char) std::min(255, bgColor.r + 40);
        bgColor.g = (unsigned char) std::min(255, bgColor.g + 40);
        bgColor.b = (unsigned char) std::min(255, bgColor.b + 40); 
    }

    DrawRectangle(x, y, cellWidth, cellHeight, bgColor);

    const char* terrainChar = getTerrainChar(tile.getTerrain());
     DrawRectangleLines(x, y, cellWidth, cellHeight, 
                      isCursor ? YELLOW : Color{60, 60, 70, 255});

    // Draw terrain character
    DrawText(terrainChar, x + 5, y + 5, 12, Color{200, 200, 200, 150});
    
    // Draw unit if present
    const auto& unitOpt = tile.getUnit();
}