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

    if (unitOpt.has_value()) {
        const Unit& unit = unitOpt.value();
        Color unitColor = getPlayerColor(unit.getOwner().getId());

        // draw unit emoji
        const char* emoji = getUnitEmoji(unit.getType());
        DrawText(emoji, x + 8, y + 8, fontSize, unitColor);

        float healthPercent = (float) (unit.getHealth() / unit.getMaxHealth());  // Assuming max 100
        int barWidth = cellWidth - 10;
        int barHeight = 4;
        int barX = x + 5;
        int barY = y + cellHeight - 8;
        
        DrawRectangle(barX, barY, barWidth, barHeight, Color{40, 40, 40, 255});
        DrawRectangle(barX, barY, (int)(barWidth * healthPercent), barHeight, 
                     healthPercent > 0.5f ? GREEN : healthPercent > 0.25f ? ORANGE : RED);
        }
    }

void TUI::renderInfoPanel(const World& world, const Unit* selectedUnit, int currentPlayer) {
    int panelX = gridOffsetX + 10 * cellWidth + 50;
    int panelY = gridOffsetY;
    int lineHeight = 25;
    int currentY = panelY;
    
    DrawText("═══ INFO ═══", panelX, currentY, 22, RAYWHITE);
    currentY += 40;
    
    if (selectedUnit) {
        Color unitColor = getPlayerColor(selectedUnit->getOwner().getId());
        
        // Unit type with emoji
        const char* emoji = getUnitEmoji(selectedUnit->getType());
        DrawText(TextFormat("%s %s", emoji, 
                selectedUnit->getType() == UnitType::Warrior ? "Warrior" : "Archer"),
                panelX, currentY, 20, unitColor);
        currentY += lineHeight;
        
        // Stats
        DrawText(TextFormat("Owner: Player %d", selectedUnit->getOwner().getId() + 1),
                panelX, currentY, 18, LIGHTGRAY);
        currentY += lineHeight;
        
        DrawText(TextFormat("Health: %d", selectedUnit->getHealth()),
                panelX, currentY, 18, LIGHTGRAY);
        currentY += lineHeight;
        
        DrawText(TextFormat("Attack: %d", selectedUnit->getDamage()),
                panelX, currentY, 18, LIGHTGRAY);
        currentY += lineHeight;
        
        // DrawText(TextFormat("Defense: %d", selectedUnit->getDef()),
        //         panelX, currentY, 18, LIGHTGRAY);
        // currentY += lineHeight;
        
        DrawText(TextFormat("Movement: %d", 
                selectedUnit->getMovement()),
                panelX, currentY, 18, selectedUnit->canMove() ? GREEN : RED);
        currentY += lineHeight;
    } else {
        DrawText("No unit selected", panelX, currentY, 18, GRAY);
        currentY += lineHeight;
    }
}

void TUI::renderControls() {
    int controlsX = gridOffsetX;
    int controlsY = gridOffsetY + 10 * cellHeight + 30;
    int lineHeight = 22;
    
    DrawText("═══ CONTROLS ═══", controlsX, controlsY, 20, RAYWHITE);
    controlsY += 30;
    
    DrawText("Arrow Keys", controlsX, controlsY, 16, SKYBLUE);
    DrawText("Move cursor", controlsX + 150, controlsY, 16, LIGHTGRAY);
    controlsY += lineHeight;
    
    DrawText("SPACE", controlsX, controlsY, 16, SKYBLUE);
    DrawText("Select unit / Move unit", controlsX + 150, controlsY, 16, LIGHTGRAY);
    controlsY += lineHeight;
    
    DrawText("ESC", controlsX, controlsY, 16, SKYBLUE);
    DrawText("Deselect unit", controlsX + 150, controlsY, 16, LIGHTGRAY);
    controlsY += lineHeight;
    
    DrawText("E", controlsX, controlsY, 16, SKYBLUE);
    DrawText("End turn", controlsX + 150, controlsY, 16, LIGHTGRAY);
    controlsY += lineHeight;
}

Color TUI::getPlayerColor(int playerId) const {
    switch (playerId) {
        case 0: return Color{100, 150, 255, 255};  // Blue
        case 1: return Color{255, 80, 80, 255};    // Red
        case 2: return Color{80, 255, 100, 255};   // Green
        case 3: return Color{255, 200, 80, 255};   // Yellow
        default: return WHITE;
    }
}

const char* TUI::getUnitEmoji(UnitType type) const {
    switch (type) {
        case UnitType::Warrior: return "⚔";   // Crossed swords
        case UnitType::Archer:  return "🏹";  // Bow and arrow
        default: return "?";
    }
}

const char* TUI::getTerrainChar(Terrain terrain) const {
    switch (terrain) {
        case Terrain::GRASS:    return "grass";
        case Terrain::FOREST:   return "forest";
        case Terrain::MOUNTAIN: return "mount";
        case Terrain::WATER:    return "water";
        default: return "?";
    }
}

Color TUI::getTerrainColor(Terrain terrain) const {
    switch (terrain) {
        case Terrain::GRASS:    return Color{60, 120, 50, 255};    // Green
        case Terrain::FOREST:   return Color{40, 90, 35, 255};     // Dark green
        case Terrain::MOUNTAIN: return Color{100, 100, 110, 255};  // Gray
        case Terrain::WATER:    return Color{50, 100, 180, 255};   // Blue
        default: return GRAY;
    }
}

