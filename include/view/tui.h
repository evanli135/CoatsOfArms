#pragma once

#include "raylib.h"
#include "model/world.h"
#include "model/unit.h"



class TUI {
public:
    TUI(int width, int height);

    void render(
        const World& world,
        const Position& cursor,
        const Unit* selectedUnit,
        int currentPlayer
    );

private:
    int screenWidth, screenHeight;
    int fontSize = 24;
    int cellWidth, cellHeight;
    int gridOffsetX, gridOffsetY;

    void renderGrid(const World& world, const Position& cursor);
    void renderCell(const World& world, const Position& pos, bool isCursor);
    void renderInfoPanel(const World& world, const Unit* selectedUnit, int currentPlayer);
    void renderControls();

    Color getPlayerColor(int playerId) const;
    const char* getUnitEmoji(UnitType type) const;
    const char* getTerrainChar(Terrain terraint) const;
    Color getTerrainColor(Terrain terrain) const;
};