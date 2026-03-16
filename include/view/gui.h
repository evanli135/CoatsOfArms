#pragma once

#include "raylib.h"
#include "model/world.h"
#include "model/unit.h"
#include "controller/error.h"

class ControlView;
class ErrorView;
class InformationView;
class GridView;
class ActionView;

class GUI {

private:
    ControlView controlView;
    ErrorView errorView;
    InformationView informationView;
    GridView gridView;
    ActionView actionView;

    int gridWindowWidth = 600;
    int gridWindowHeight = 600;

    int marginGridInfo = 10;

public:
    GUI(int width, int height);

    void render(
        const World& world,
        const Position& cursor,
        const Position* selectedPosition
        );
 
    void setError(const PlayerError error);
    void clearError();
};

class ControlView {

private:

public:
    ControlView();

    void render(
        const World& world,
        const Position& cursor,
        const Position* selectedPosition
    )
};

class ErrorView {
private:

public:
    ErrorView();

    void setError(const PlayerError error);
    void clearError();
};

class InformationView {

private:


public:
    InformationView();

    void render(const World& world, const Unit* selectedUnit, int currentPlayer);
};

class GridView {

private:
    void renderUnit(const World& world, const Position& pos, bool isCursor, bool isSelected);
    void renderTerrain(const World& world, const Position& pos);
    void renderCell(const World& world, const Position& pos, bool isCursor, bool isSelected);

public:
    GridView();

    void render(const World& world, const Position& cursor, const Position* selectedPosition);
};

class ActionView {

private:

public:
    ActionView();

    void render(const World& world, const Position& cursor, const Position* selectedPosition);
};