#pragma once

#include "raylib.h"
#include "model/world.h"
#include "model/unit.h"
#include "controller/error.h"



class TUI {
public:
    TUI(int width, int height);

    void render(
        const World& world,
        const Position& cursor,
        const Position* selectedPosition
        );

    void setError(const PlayerError error);
    void clearError();

private:
    int screenWidth, screenHeight;
    int fontSize = 24;
    int cellWidth, cellHeight;
    int gridOffsetX, gridOffsetY;
    int totalGridWidth, totalGridHeight;

    std::optional<PlayerError> currentError;

    void renderGrid(const World& world, const Position& cursor, const Position* selectedPosition);
    void renderCell(const World& world, const Position& pos, bool isCursor, bool isSelected);
    void renderInfoPanel(const World& world, const Unit* selectedUnit, int currentPlayer);
    void renderControls();
    void renderError();

    Color getUnitColor(const Unit* unit) const;
    Color getPlayerColor(int playerId) const;
    const char* getUnitSymbol(UnitType type) const;
    const char* getTerrainChar(Terrain terrain) const;
    Color getTerrainColor(Terrain terrain) const;
};