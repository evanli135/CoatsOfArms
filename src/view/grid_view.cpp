#include "view/grid_view.h"
#include "view/sprites.h"
#include "view/layout.h"
#include "model/world.h"
#include "model/tile.h"
#include "model/unit.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

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
void GridView::render(const World& world, const Position* hoverPos, const Position* selectedPos,
                      const std::vector<Position>& reachable,
                      const std::vector<Position>& attackable)
{
    std::unordered_set<Position> reachableSet(reachable.begin(), reachable.end());
    std::unordered_set<Position> attackableSet(attackable.begin(), attackable.end());

    for (int sum = 0; sum < Game::HEIGHT + Game::WIDTH - 1; ++sum) {
        int rMin = std::max(0, sum - (Game::WIDTH  - 1));
        int rMax = std::min(sum, Game::HEIGHT - 1);
        for (int row = rMin; row <= rMax; ++row) {
            int col = sum - row;
            if (col < 0 || col >= Game::WIDTH) continue;
            Position pos(row, col);
            renderCell(world, pos,
                       hoverPos    && pos == *hoverPos,
                       selectedPos && pos == *selectedPos,
                       reachableSet.count(pos) > 0,
                       attackableSet.count(pos) > 0);
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
    bool isEnemy = (u->getOwner().getId() != world.getCurrentPlayer().getId());
    if (isEnemy) {
        // Enemy units are consistently dimmed
        tint.r = (unsigned char)((tint.r + 40) / 2);
        tint.g = (unsigned char)((tint.g + 40) / 2);
        tint.b = (unsigned char)((tint.b + 40) / 2);
        tint.a = 160;
    } else if (u->isExhausted()) {
        // Current player's exhausted units are further grayed out
        tint.r = (unsigned char)((tint.r + 80) / 2);
        tint.g = (unsigned char)((tint.g + 80) / 2);
        tint.b = (unsigned char)((tint.b + 80) / 2);
        tint.a = 140;
    }

    // Shift unit origin up so the sprite is centred on the tile diamond
    // rather than sitting at the front corner.
    const int unitPy = py - ISO_HALF_H;

    // "Selectable" indicators for current player's ready units
    bool isSelectable = (u->canMove() || u->canAttack()) &&
                        (u->getOwner().getId() == world.getCurrentPlayer().getId());
    if (isSelectable) {
        float t     = (float)GetTime();
        float pulse = 0.5f + 0.5f * sinf(t * 3.0f);
        int   cx    = px;
        int   cy    = unitPy + ISO_HALF_H - 4;

        // Pulsing glow ring drawn behind the unit
        unsigned char ga = (unsigned char)(30 + 28 * pulse);
        DrawCircle(cx, cy, 22, Color{tint.r, tint.g, tint.b, ga});
        DrawCircleLines(cx, cy, 22, Color{tint.r, tint.g, tint.b, (unsigned char)(ga * 2)});

        // Bouncing downward-pointing chevron above the unit's head
        float bounce = 2.0f * sinf(t * 4.0f);
        int   iy     = unitPy - 26 + (int)bounce;
        DrawTriangle(
            {(float)(cx - 7), (float)iy},
            {(float)(cx + 7), (float)iy},
            {(float)cx,       (float)(iy + 9)},
            Color{tint.r, tint.g, tint.b, 255});
        // Bright highlight stripe on the chevron
        DrawTriangle(
            {(float)(cx - 4), (float)(iy + 1)},
            {(float)(cx + 4), (float)(iy + 1)},
            {(float)cx,       (float)(iy + 6)},
            Color{255, 255, 255, 70});
    }

    // Drop shadow, then sprite
    Sprites::unit(u->getType(), px+2, unitPy+2, Color{0,0,0,(unsigned char)(tint.a/2)});
    Sprites::unit(u->getType(), px,   unitPy,   tint);

    // HP number — bottom-left of the sprite
    float hp = (float)u->getHealth() / (float)u->getMaxHealth();
    Color hpCol = hp >= 0.5f ? Color{200, 230, 200, 220} : Color{230, 70, 70, 230};
    DrawText(TextFormat("%d", u->getHealth()), px - 26, unitPy + 42, 11, hpCol);
}

void GridView::renderHoverLayer(int px, int py) {
    Vector2 top = {(float)px,             (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,             (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};

    // Subtle yellow fill
    DrawTriangle(top, lt, bot, Color{255,240,80,40});
    DrawTriangle(top, bot, rt, Color{255,240,80,40});

    // Bright yellow outline
    DrawLineEx(top, rt,  2.0f, Color{255,220,0,230});
    DrawLineEx(rt,  bot, 2.0f, Color{255,220,0,230});
    DrawLineEx(bot, lt,  2.0f, Color{255,220,0,230});
    DrawLineEx(lt,  top, 2.0f, Color{255,220,0,230});

    // Corner tick marks — short inward segments at each diamond vertex.
    // Edge direction unit vectors: top→rt = (0.894, 0.447), top→lt = (-0.894, 0.447)
    const float T = 9.0f;
    const Color tk = Color{255,255,120,255};
    DrawLineEx(top, {top.x + T*0.894f, top.y + T*0.447f}, 2.5f, tk);
    DrawLineEx(top, {top.x - T*0.894f, top.y + T*0.447f}, 2.5f, tk);
    DrawLineEx(rt,  {rt.x  - T*0.894f, rt.y  - T*0.447f}, 2.5f, tk);
    DrawLineEx(rt,  {rt.x  - T*0.894f, rt.y  + T*0.447f}, 2.5f, tk);
    DrawLineEx(bot, {bot.x - T*0.894f, bot.y - T*0.447f}, 2.5f, tk);
    DrawLineEx(bot, {bot.x + T*0.894f, bot.y - T*0.447f}, 2.5f, tk);
    DrawLineEx(lt,  {lt.x  + T*0.894f, lt.y  - T*0.447f}, 2.5f, tk);
    DrawLineEx(lt,  {lt.x  + T*0.894f, lt.y  + T*0.447f}, 2.5f, tk);
}

void GridView::renderSelectionLayer(int px, int py) {
    Vector2 top = {(float)px,             (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,             (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};

    // Pulsing cyan fill
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f);
    unsigned char fa = (unsigned char)(20 + 35 * pulse);
    DrawTriangle(top, lt, bot, Color{80,220,255,fa});
    DrawTriangle(top, bot, rt, Color{80,220,255,fa});

    // Outer glow — two stacked borders, fading outward
    unsigned char ga = (unsigned char)(120 + 100 * pulse);
    DrawLineEx(top, rt,  4.0f, Color{0,210,255,ga});
    DrawLineEx(rt,  bot, 4.0f, Color{0,210,255,ga});
    DrawLineEx(bot, lt,  4.0f, Color{0,210,255,ga});
    DrawLineEx(lt,  top, 4.0f, Color{0,210,255,ga});
    DrawLineEx(top, rt,  2.0f, Color{200,245,255,255});
    DrawLineEx(rt,  bot, 2.0f, Color{200,245,255,255});
    DrawLineEx(bot, lt,  2.0f, Color{200,245,255,255});
    DrawLineEx(lt,  top, 2.0f, Color{200,245,255,255});

    // Corner bracket marks — long inward segments at each vertex
    const float B = 14.0f;
    const Color bk = Color{255,255,255,230};
    DrawLineEx(top, {top.x + B*0.894f, top.y + B*0.447f}, 3.0f, bk);
    DrawLineEx(top, {top.x - B*0.894f, top.y + B*0.447f}, 3.0f, bk);
    DrawLineEx(rt,  {rt.x  - B*0.894f, rt.y  - B*0.447f}, 3.0f, bk);
    DrawLineEx(rt,  {rt.x  - B*0.894f, rt.y  + B*0.447f}, 3.0f, bk);
    DrawLineEx(bot, {bot.x - B*0.894f, bot.y - B*0.447f}, 3.0f, bk);
    DrawLineEx(bot, {bot.x + B*0.894f, bot.y - B*0.447f}, 3.0f, bk);
    DrawLineEx(lt,  {lt.x  + B*0.894f, lt.y  - B*0.447f}, 3.0f, bk);
    DrawLineEx(lt,  {lt.x  + B*0.894f, lt.y  + B*0.447f}, 3.0f, bk);
}

void GridView::renderReachableLayer(int px, int py) {
    Vector2 top = {(float)px,             (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,             (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};

    // Soft blue fill
    DrawTriangle(top, lt, bot, Color{60, 140, 255, 50});
    DrawTriangle(top, bot, rt, Color{60, 140, 255, 50});
    // Blue dashed-style outline (solid, lighter)
    DrawLineEx(top, rt,  1.5f, Color{100, 180, 255, 200});
    DrawLineEx(rt,  bot, 1.5f, Color{100, 180, 255, 200});
    DrawLineEx(bot, lt,  1.5f, Color{100, 180, 255, 200});
    DrawLineEx(lt,  top, 1.5f, Color{100, 180, 255, 200});
}

void GridView::renderCell(const World& world, const Position& pos,
                          bool isHovered, bool isSelected, bool isReachable, bool isAttackable) {
    int px, py;
    isoTopVertex(pos.row(), pos.col(), scrollOffsetX, scrollOffsetY, px, py);
    const Tile& tile = world.getTileAt(pos);

    renderTerrainLayer(tile, px, py);
    renderCityLayer(tile, px, py);
    renderUnitLayer(world, tile, px, py);
    if (isReachable)  renderReachableLayer(px, py);
    if (isAttackable) renderAttackableLayer(px, py);
    if (isHovered)    renderHoverLayer(px, py);
    if (isSelected)   renderSelectionLayer(px, py);
}

void GridView::renderAttackableLayer(int px, int py) {
    Vector2 top = {(float)px,             (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,             (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};

    // Soft red fill
    DrawTriangle(top, lt, bot, Color{220, 50, 50, 55});
    DrawTriangle(top, bot, rt, Color{220, 50, 50, 55});
    // Red outline
    DrawLineEx(top, rt,  1.5f, Color{255, 80,  80,  210});
    DrawLineEx(rt,  bot, 1.5f, Color{255, 80,  80,  210});
    DrawLineEx(bot, lt,  1.5f, Color{255, 80,  80,  210});
    DrawLineEx(lt,  top, 1.5f, Color{255, 80,  80,  210});
}
