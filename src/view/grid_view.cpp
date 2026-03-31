#include "view/grid_view.h"
#include "view/sprites.h"
#include "view/layout.h"
#include "model/world.h"
#include "model/tile.h"
#include "model/unit.h"
#include "raylib.h"
#include <algorithm>

using namespace Layout;

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

GridView::GridView()
    : scrollOffsetX(0), scrollOffsetY(0),
      terrainSprites{},
      unitSprites{}
{
    // Scroll arrows unused in iso mode (grid fits fully on screen).
    arrowUp = arrowDown = arrowLeft = arrowRight = Rect{0, 0, 0, 0};
}

GridView::~GridView() {
    for (auto& tex : terrainSprites) if (tex) UnloadTexture(*tex);
    for (auto& tex : unitSprites)    if (tex) UnloadTexture(*tex);
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void GridView::loadTerrainSprite(Terrain t, const char* path) {
    terrainSprites[(int)t] = LoadTexture(path);
}

void GridView::loadUnitSprite(UnitType t, const char* path) {
    unitSprites[(int)t] = LoadTexture(path);
}

void GridView::scrollBy(int dpx, int dpy) {
    // 12×12 iso grid fits fully on 1920×1080 — no scroll range.
    scrollOffsetX = std::clamp(scrollOffsetX + dpx, 0, 0);
    scrollOffsetY = std::clamp(scrollOffsetY + dpy, 0, 0);
}

void GridView::resetScroll() {
    scrollOffsetX = 0;
    scrollOffsetY = 0;
}

// Painter's algorithm: render tiles in order of increasing (row + col)
// so that tiles closer to the camera (larger row+col) draw over tiles further away.
void GridView::render(const World& world, const Position* hoverPos, const Position* selectedPos) {
    for (int sum = 0; sum < Game::HEIGHT + Game::WIDTH - 1; ++sum) {
        int rMin = std::max(0, sum - (Game::WIDTH  - 1));
        int rMax = std::min(sum, Game::HEIGHT - 1);
        for (int row = rMin; row <= rMax; ++row) {
            int col = sum - row;
            if (col < 0 || col >= Game::WIDTH) continue;
            Position pos(row, col);
            renderCell(world, pos,
                       hoverPos    && pos == *hoverPos,
                       selectedPos && pos == *selectedPos);
        }
    }
}

// ---------------------------------------------------------------------------
// Rendering pipeline
// ---------------------------------------------------------------------------

// Top vertex of tile (row,col) in screen space.
static void isoTopVertex(int row, int col, int scrollX, int scrollY, int& px, int& py) {
    px = GRID_ORIG_X + (col - row) * ISO_HALF_W - scrollX;
    py = GRID_ORIG_Y + (col + row) * ISO_HALF_H - scrollY;
}

void GridView::renderTerrainLayer(const Tile& tile, int px, int py) {
    // Texture path not yet adapted for iso diamond rendering — always procedural.
    Sprites::terrain(tile.getTerrain(), px, py);
}

void GridView::renderCityLayer(const Tile& tile, int px, int py) {
    if (tile.hasCity()) Sprites::city(px, py);
}

void GridView::renderUnitLayer(const World& world, const Tile& tile, int px, int py) {
    if (!tile.hasUnit()) return;
    const Unit* u = world.getUnit(tile.getUnit().value());
    if (!u) return;

    Color tint = playerColor(u->getOwner().getId());
    if (!u->canMove()) {
        tint.r = (unsigned char)((tint.r + 80) / 2);
        tint.g = (unsigned char)((tint.g + 80) / 2);
        tint.b = (unsigned char)((tint.b + 80) / 2);
        tint.a = 140;
    }

    // Drop shadow, then sprite
    Sprites::unit(u->getType(), px+2, py+2, Color{0,0,0,(unsigned char)(tint.a/2)});
    Sprites::unit(u->getType(), px, py, tint);

    // Health bar — drawn inside the diamond face near the top.
    // At h=14px below the top vertex the diamond is 56px wide; bar fits at 50px.
    float hp  = (float)u->getHealth() / (float)u->getMaxHealth();
    int   bw  = 50;
    int   bx  = px - bw / 2;
    int   barY = py + 14;
    DrawRectangle(bx, barY, bw, 4, Color{20,20,20,200});
    Color bar = hp > 0.5f ? Color{60,200,60,255} : hp > 0.25f ? Color{230,160,30,255} : Color{220,40,40,255};
    DrawRectangle(bx, barY, (int)(bw * hp), 4, bar);
    DrawRectangle(bx, barY, (int)(bw * hp), 2, Color{255,255,255,45});
}

void GridView::renderHoverLayer(int px, int py) {
    Vector2 top = {(float)px,            (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,            (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};
    DrawTriangle(top, lt, bot, Color{255,248,180,28});
    DrawTriangle(top, bot, rt, Color{255,248,180,28});
    DrawLineEx(top, rt,  2.0f, Color{255,230,0,220});
    DrawLineEx(rt,  bot, 2.0f, Color{255,230,0,220});
    DrawLineEx(bot, lt,  2.0f, Color{255,230,0,220});
    DrawLineEx(lt,  top, 2.0f, Color{255,230,0,220});
}

void GridView::renderSelectionLayer(int px, int py) {
    Vector2 top = {(float)px,            (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,            (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};
    DrawTriangle(top, lt, bot, Color{255,255,255,18});
    DrawTriangle(top, bot, rt, Color{255,255,255,18});
    DrawLineEx(top, rt,  2.5f, WHITE);
    DrawLineEx(rt,  bot, 2.5f, WHITE);
    DrawLineEx(bot, lt,  2.5f, WHITE);
    DrawLineEx(lt,  top, 2.5f, WHITE);
    // Inner diamond
    float s = 3.0f;
    Vector2 top2 = {top.x,           top.y + s};
    Vector2 rt2  = {rt.x  - s*0.5f,  rt.y};
    Vector2 bot2 = {bot.x,           bot.y - s};
    Vector2 lt2  = {lt.x  + s*0.5f,  lt.y};
    DrawLineEx(top2, rt2,  1.5f, Color{255,255,255,130});
    DrawLineEx(rt2,  bot2, 1.5f, Color{255,255,255,130});
    DrawLineEx(bot2, lt2,  1.5f, Color{255,255,255,130});
    DrawLineEx(lt2,  top2, 1.5f, Color{255,255,255,130});
}

void GridView::renderCell(const World& world, const Position& pos, bool isHovered, bool isSelected) {
    int px, py;
    isoTopVertex(pos.row(), pos.col(), scrollOffsetX, scrollOffsetY, px, py);
    const Tile& tile = world.getTileAt(pos);

    renderTerrainLayer(tile, px, py);
    renderCityLayer(tile, px, py);
    renderUnitLayer(world, tile, px, py);
    if (isHovered)  renderHoverLayer(px, py);
    if (isSelected) renderSelectionLayer(px, py);
}
