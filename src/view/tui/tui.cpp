#include "view/tui.h"
#include <string>

TUI::TUI(int screenWidth, int screenHeight)
    : screenWidth(screenWidth), 
      screenHeight(screenHeight),
      fontSize(24),
      cellWidth(40),
      cellHeight(40)
    //   gridOffsetX(50),
    //   gridOffsetY(80) 
    {}

void TUI::render(
    const World& world,
    const Position& cursor,
    const Unit* selectedUnit,
    int currentPlayer
) {
    ClearBackground(Color{20, 20, 30, 255});
    
}