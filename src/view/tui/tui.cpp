#include "view/tui.h"
#include "model/util.h"
#include <string>

TUI::TUI(int screenWidth, int screenHeight)
    : screenWidth(screenWidth), 
      screenHeight(screenHeight),
      fontSize(24),
      cellWidth(40),
      cellHeight(40),
      currentError(std::nullopt)
{
    totalGridWidth = Game::WIDTH * cellWidth;
    totalGridHeight = Game::HEIGHT * cellHeight;
    
    // Center horizontally
    gridOffsetX = (screenWidth - totalGridWidth) / 2;
    
    // Adjust vertical centering with smaller top, bigger bottom margin
    int topMargin = 80;     // Reduced from 100
    int bottomMargin = 200; // Space for info panel
    int availableHeight = screenHeight - topMargin - bottomMargin;
    gridOffsetY = topMargin + (availableHeight - totalGridHeight) / 2;
}

void TUI::render(
    const World& world,
    const Position& cursor,
    const Position* selectedPosition
    ) {
    int currentPlayer = world.getCurrentPlayer().getId();
    ClearBackground(Color{20, 20, 30, 255});
    
    // Title
    DrawText("REV ROYALE", screenWidth / 2 - 150, 50, 32, RAYWHITE);

    // Current player indicator
    Color playerColor = getPlayerColor(currentPlayer);
    DrawText(TextFormat("Player %d's Turn", currentPlayer), screenWidth / 2 - 80, 80, 20, playerColor);

    DrawText(TextFormat("Turn: %d", world.getTurn()), screenWidth / 2 - 80, 100, 20, Color{200, 200, 200, 150});

    const Unit* selectedUnit = nullptr;
    if (selectedPosition != nullptr) {
        selectedUnit = world.getTileAt(*selectedPosition).getUnit().has_value() 
            ? &world.getTileAt(*selectedPosition).getUnit().value() 
            : nullptr;
        }

    renderGrid(world, cursor, selectedPosition);
    renderInfoPanel(world, selectedUnit, currentPlayer);
    renderControls();
    renderError();

    DrawFPS(10, 10);
}

void TUI::renderGrid(const World& world, const Position& cursor, const Position* selectedPosition) {
    int worldWidth = Game::WIDTH;
    int worldHeight = Game::HEIGHT;
    
    for (int row = 0; row < worldHeight; row++) {
        for (int col = 0; col < worldWidth; col++) {
            Position pos(row, col);
            bool isCursor = (pos == cursor);
            renderCell(world, pos, isCursor, selectedPosition != nullptr && pos == *selectedPosition);
        }
    }
}


void TUI::renderCell(const World& world, const Position& pos, bool isCursor, bool isSelected) {
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

    // Draw borders - selected gets priority, then cursor, then normal
    if (isSelected) {
        // Thick white border for selected cell
        DrawRectangleLines(x, y, cellWidth, cellHeight, WHITE);
        DrawRectangleLines(x + 1, y + 1, cellWidth - 2, cellHeight - 2, WHITE);
        DrawRectangleLines(x + 2, y + 2, cellWidth - 4, cellHeight - 4, WHITE);
    } else if (isCursor) {
        DrawRectangleLines(x, y, cellWidth, cellHeight, YELLOW);
    } else {
        DrawRectangleLines(x, y, cellWidth, cellHeight, Color{60, 60, 70, 255});
    }

    // Draw terrain character
    const char* terrainChar = getTerrainChar(tile.getTerrain());
    DrawText(terrainChar, x + 5, y + 5, 12, Color{200, 200, 200, 150});
    
    // Draw unit if present
    const auto& unitOpt = tile.getUnit();

    if (unitOpt.has_value()) {
        const Unit& unit = unitOpt.value();
        Color unitColor = getUnitColor(&unit);

        // draw unit symbol
        const char* emoji = getUnitSymbol(unit.getType());
        DrawText(emoji, x + 8, y + 8, fontSize, unitColor);

        float healthPercent = (float) (unit.getHealth() / unit.getMaxHealth());
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
    int panelX = screenWidth / 2 - 150; // Right Margin
    int panelY = screenHeight - 180; // Align with top
    int lineHeight = 25;
    int currentY = panelY;
    
    DrawText("=== INFO ===", panelX, currentY, 22, RAYWHITE);
    currentY += 40;
    
    if (selectedUnit != nullptr) {
        Color unitColor = getPlayerColor(selectedUnit->getOwner().getId());
        
        // Unit type with emoji
        const char* emoji = getUnitSymbol(selectedUnit->getType());
        DrawText(TextFormat("%s %s", emoji, 
                selectedUnit->getType() == UnitType::WARRIOR ? "Warrior" : "Archer"),
                panelX, currentY, 20, unitColor);
        currentY += lineHeight;
        
        // Stats in a horizontal layout to save vertical space
        DrawText(TextFormat("Owner: P%d | HP: %d | ATK: %d | MOV: %d | Can Move: %c", 
                selectedUnit->getOwner().getId() + 1,
                selectedUnit->getHealth(),
                selectedUnit->getDamage(),
                selectedUnit->getMovement(),
                selectedUnit->canMove() ? 'Y' : 'N'),
                panelX, currentY, 18, LIGHTGRAY);
    } else {
        DrawText("No unit selected", panelX, currentY, 18, GRAY);
    }
}


void TUI::renderControls() {
    int controlsX = 20;
    int controlsY = gridOffsetY;
    int lineHeight = 22;
    
    DrawText("=== CONTROLS ===", controlsX, controlsY, 20, RAYWHITE);
    controlsY += 30;
    
    DrawText("Arrow Keys", controlsX, controlsY, 16, SKYBLUE);
    DrawText("Move cursor", controlsX + 100, controlsY, 16, LIGHTGRAY);
    controlsY += lineHeight;
    
    DrawText("SPACE", controlsX, controlsY, 16, SKYBLUE);
    DrawText("Select / Move", controlsX + 100, controlsY, 16, LIGHTGRAY);
    controlsY += lineHeight;
    
    DrawText("ESC", controlsX, controlsY, 16, SKYBLUE);
    DrawText("Deselect", controlsX + 100, controlsY, 16, LIGHTGRAY);
    controlsY += lineHeight;
    
    DrawText("SHIFT", controlsX, controlsY, 16, SKYBLUE);
    DrawText("End turn", controlsX + 100, controlsY, 16, LIGHTGRAY);
    controlsY += lineHeight;
    
    DrawText("Q", controlsX, controlsY, 16, SKYBLUE);
    DrawText("Quit", controlsX + 100, controlsY, 16, LIGHTGRAY);
}

void TUI::renderError() {
    int errorX = screenWidth / 2 + (totalGridWidth / 2) + 50;
    int errorY = (screenHeight - totalGridHeight) / 2;
    
    DrawText("=== ERROR ===", errorX, errorY, 20, RED);
    errorY += 30;
    
    if (currentError.has_value()) {  // If error EXISTS
        DrawText(playerErrorToString(currentError.value()).c_str(), 
            errorX, errorY, 16, RED);   
    } else {  // If NO error
        DrawText("No errors", errorX, errorY, 16, LIGHTGRAY);
    }
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

Color TUI::getUnitColor(const Unit* unit) const {
    if (unit == nullptr) throw std::logic_error("Unit pointer cannot be null");

    Color baseColor;
    switch (unit->getOwner().getId()) {
        case 0: baseColor = Color{100, 150, 255, 255};  // Blue
                break;
        case 1: baseColor = Color{255, 80, 80, 255};    // Red
                break;
        case 2: baseColor = Color{80, 255, 100, 255};   // Green
                break;
        case 3: baseColor = Color{255, 200, 80, 255};   // Yellow
                break;
        default: baseColor = WHITE;
    }

    // If unit can't move, desaturate and darken it
    if (!unit->canMove()) {
        // Mix with gray to desaturate (50% mix)
        baseColor.r = (baseColor.r + 100) / 2;
        baseColor.g = (baseColor.g + 100) / 2;
        baseColor.b = (baseColor.b + 100) / 2;
        // Reduce opacity
        baseColor.a = 150;
    }

    return baseColor;
}

/**
 * "♦" // diamond
"●" // circle  
"■" // square
"★" // star
"→" // arrow
 */

const char* TUI::getUnitSymbol(UnitType type) const {
    switch (type) {
        case UnitType::WARRIOR: return "x";   // Crossed swords
        case UnitType::RANGER:  return ">";
        case UnitType::SCOUT:   return "^";
        case UnitType::CAVALRY: return "!";
        case UnitType::MAGE:    return "*";
        default: return "?";
    }
}

const char* TUI::getTerrainChar(Terrain terrain) const {
    switch (terrain) {
        case Terrain::GRASS:    return "grs";
        case Terrain::FOREST:   return "for";
        case Terrain::MOUNTAIN: return "mnt";
        case Terrain::OCEAN:    return "wtr";
        default: return "?";
    }
}

Color TUI::getTerrainColor(Terrain terrain) const {
    switch (terrain) {
        case Terrain::GRASS:    return Color{60, 120, 50, 255};    // Green
        case Terrain::FOREST:   return Color{40, 90, 35, 255};     // Dark green
        case Terrain::MOUNTAIN: return Color{100, 100, 110, 255};  // Gray
        case Terrain::OCEAN:    return Color{50, 100, 180, 255};   // Blue
        default: return GRAY;
    }
}

void TUI::setError(const PlayerError error) {
    currentError = error;
}

void TUI::clearError() {
    currentError = std::nullopt;
}