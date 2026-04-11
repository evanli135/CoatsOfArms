#include "view/grid_view.h"
#include "view/sprites.h"
#include "view/layout.h"
#include "model/world.h"
#include "model/tile.h"
#include "model/unit.h"
#include "model/city.h"
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
    scrollOffsetX = std::clamp(scrollOffsetX + dpx, scrollMinX_, scrollMaxX_);
    scrollOffsetY = std::clamp(scrollOffsetY + dpy, scrollMinY_, scrollMaxY_);
}

void GridView::applyScrollBounds(int scrollMinX, int scrollMaxX, int scrollMinY, int scrollMaxY) {
    scrollMinX_ = scrollMinX;
    scrollMaxX_ = scrollMaxX;
    scrollMinY_ = scrollMinY;
    scrollMaxY_ = scrollMaxY;
    scrollOffsetX = std::clamp(scrollOffsetX, scrollMinX_, scrollMaxX_);
    scrollOffsetY = std::clamp(scrollOffsetY, scrollMinY_, scrollMaxY_);
}

void GridView::resetScroll() {
    scrollOffsetX = 0;
    scrollOffsetY = 0;
}

// Painter's algorithm: render tiles in order of increasing (row + col)
// so that tiles closer to the camera (larger row+col) draw over tiles further away.
void GridView::render(const Layout::ViewLayout& layout,
                      const World& world, const Position* hoverPos, const Position* selectedPos,
                      const std::vector<Position>& reachable,
                      const std::vector<Position>& attackable,
                      const std::vector<Position>& lethal,
                      const std::vector<Position>& path)
{
    gridOrigX_ = layout.gridOrigX;
    gridOrigY_ = layout.gridOrigY;
    std::unordered_set<Position> reachableSet(reachable.begin(), reachable.end());
    std::unordered_set<Position> attackableSet(attackable.begin(), attackable.end());
    std::unordered_set<Position> lethalSet(lethal.begin(), lethal.end());

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
                       attackableSet.count(pos) > 0,
                       lethalSet.count(pos) > 0);
        }
    }

    // Path arrows drawn in a separate post-pass so they sit on top of all tiles.
    renderPathArrows(path);
}

// ---------------------------------------------------------------------------
// Rendering pipeline
// ---------------------------------------------------------------------------

void GridView::isoTopVertex(int row, int col, int& px, int& py) const {
    px = gridOrigX_ + (col - row) * ISO_HALF_W - scrollOffsetX;
    py = gridOrigY_ + (col + row) * ISO_HALF_H - scrollOffsetY;
}

void GridView::renderTerrainLayer(const Tile& tile, int px, int py) {
    // Texture path not yet adapted for iso diamond rendering — always procedural.
    Sprites::terrain(tile.getTerrain(), px, py);
}

void GridView::renderCityLayer(const Tile& tile, int px, int py) {
    if (!tile.hasCity()) return;
    const City* city = tile.getCity();
    Color factionColor = city->hasOwner()
        ? playerColor(city->getOwner().getId())
        : Color{160, 160, 160, 255};   // neutral grey for unclaimed
    Sprites::city(px, py, factionColor);

    if (!city->isTraining()) return;
    const TrainingSlot* slot = city->getTrainingSlot();

    // Ghost unit — only visible when no real unit is blocking the tile.
    const int ghostPy = py - ISO_HALF_H;
    if (!tile.hasUnit()) {
        // Semi-transparent grayed silhouette of the queued unit type.
        Sprites::unit(slot->unitType, px + 1, ghostPy + 1, Color{0,   0,   0,   35});
        Sprites::unit(slot->unitType, px,     ghostPy,     Color{140, 140, 155, 85});
    }

    // Circular countdown arc above the ghost / castle flag.
    // 0 turns remaining → full circle (ready), 2 turns remaining → empty.
    const float progress = 1.0f - slot->turnsRemaining / 2.0f;  // 0.0 → 1.0
    const float t        = (float)GetTime();
    const float pulse    = 0.5f + 0.5f * sinf(t * 3.0f);
    const Vector2 arcCenter = { (float)px, (float)(ghostPy - 16) };
    const float innerR = 7.0f, outerR = 11.0f;

    // Background ring (full circle, dark)
    DrawRing(arcCenter, innerR, outerR, 0.0f, 360.0f, 24,
             Color{30, 30, 40, 180});

    // Progress arc: sweeps from top (-90°) clockwise
    if (progress > 0.01f) {
        float endAngle = -90.0f + progress * 360.0f;
        Color arcCol   = slot->turnsRemaining == 0
            ? Color{80,  220, 100, (unsigned char)(200 + 55 * pulse)}  // ready: bright green
            : Color{100, 190, 255, 200};                               // in progress: blue
        DrawRing(arcCenter, innerR, outerR, -90.0f, endAngle,
                 std::max(4, (int)(progress * 24)), arcCol);
    }

    // Small tick at 12 o'clock
    DrawCircleV({arcCenter.x, arcCenter.y - outerR - 1.5f}, 1.5f,
                Color{200, 200, 220, 160});
}

void GridView::renderUnitLayer(const World& world, const Tile& tile, const Position& pos, int px, int py) {
    if (!tile.hasUnit()) return;
    const Unit* u = world.getUnit(tile.getUnit().value());
    if (!u) return;

    const bool isCurrentPlayer = (u->getOwner().getId() == world.getCurrentPlayer().getId());

    // A unit is "effectively done" when it truly has nothing left to do this turn:
    // either it has attacked, or it has moved and no enemy is within attack range.
    bool effectivelyDone = false;
    if (isCurrentPlayer) {
        if (u->isExhausted()) {
            effectivelyDone = true;
        } else if (u->hasMoved()) {
            effectivelyDone = world.getAttackSnapshot(pos).empty();
        }
    }

    Color tint = playerColor(u->getOwner().getId());
    if (!isCurrentPlayer) {
        // Enemy units are consistently dimmed
        tint.r = (unsigned char)((tint.r + 40) / 2);
        tint.g = (unsigned char)((tint.g + 40) / 2);
        tint.b = (unsigned char)((tint.b + 40) / 2);
        tint.a = 160;
    } else if (effectivelyDone) {
        // Current player's done units are grayed out
        tint.r = (unsigned char)((tint.r + 80) / 2);
        tint.g = (unsigned char)((tint.g + 80) / 2);
        tint.b = (unsigned char)((tint.b + 80) / 2);
        tint.a = 140;
    }

    // Shift unit origin up so the sprite is centred on the tile diamond
    // rather than sitting at the front corner.
    const int unitPy = py - ISO_HALF_H;

    // "Selectable" indicators for current player's active units
    bool isSelectable = isCurrentPlayer && !effectivelyDone;
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

void GridView::renderReachableLayer(int px, int py, bool isHovered) {
    Vector2 top = {(float)px,             (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,             (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};

    if (isHovered) {
        const float t  = (float)GetTime();
        const float cx = (float)px;
        const float cy = (float)(py + ISO_HALF_H);

        // Bright fill
        DrawTriangle(top, lt, bot, Color{70, 155, 255, 100});
        DrawTriangle(top, bot, rt, Color{70, 155, 255, 100});

        // ── Border: 2 diamond rings falling inward ───────────────────────────
        float fallPhase = fmodf(t * 1.4f, 1.0f);
        for (int ring = 0; ring < 2; ++ring) {
            float p     = fmodf(fallPhase + ring * 0.5f, 1.0f);
            float inset = p * 0.55f;
            float a     = 1.0f - p;
            unsigned char ba = (unsigned char)(a * 255.0f);
            float lw    = 3.0f - p * 1.5f;
            if (lw < 1.0f) lw = 1.0f;

            auto lv = [&](Vector2 v) -> Vector2 {
                return { cx + (v.x - cx)*(1.0f - inset),
                         cy + (v.y - cy)*(1.0f - inset) };
            };
            Vector2 t2=lv(top), r2=lv(rt), b2=lv(bot), l2=lv(lt);
            Color c = Color{60, 200, 255, ba};
            DrawLineEx(t2, r2, lw, c);
            DrawLineEx(r2, b2, lw, c);
            DrawLineEx(b2, l2, lw, c);
            DrawLineEx(l2, t2, lw, c);
        }

        // ── 2 blue V-arrows cascading down into the tile ─────────────────────
        const float period = 0.75f;
        const float aw     = 12.0f;
        const float ah     = 10.0f;

        for (int i = 0; i < 2; ++i) {
            float p   = fmodf(fmodf(t, period) / period + i * 0.5f, 1.0f);
            float ay  = (cy - 20.0f) + p * 26.0f;
            float a   = 1.0f - p;
            unsigned char aa  = (unsigned char)(a * 255.0f);
            unsigned char sha = (unsigned char)(a * 80.0f);

            Color arrowCol = Color{40, 190, 255, aa};
            Color arrowShd = Color{0, 0, 0, sha};

            DrawLineEx({cx - aw + 1, ay - ah + 1}, {cx + 1, ay + 1}, 3.5f, arrowShd);
            DrawLineEx({cx + aw + 1, ay - ah + 1}, {cx + 1, ay + 1}, 3.5f, arrowShd);
            DrawLineEx({cx - aw, ay - ah}, {cx, ay}, 3.5f, arrowCol);
            DrawLineEx({cx + aw, ay - ah}, {cx, ay}, 3.5f, arrowCol);
        }
    } else {
        // Non-hovered reachable tile — static blue outline
        DrawTriangle(top, lt, bot, Color{60, 140, 255, 50});
        DrawTriangle(top, bot, rt, Color{60, 140, 255, 50});
        DrawLineEx(top, rt,  1.5f, Color{100, 180, 255, 200});
        DrawLineEx(rt,  bot, 1.5f, Color{100, 180, 255, 200});
        DrawLineEx(bot, lt,  1.5f, Color{100, 180, 255, 200});
        DrawLineEx(lt,  top, 1.5f, Color{100, 180, 255, 200});
    }
}

void GridView::renderLethalLayer(int px, int py) {
    const float t     = (float)GetTime();
    const float pulse = 0.5f + 0.5f * sinf(t * 3.5f);
    const unsigned char alpha = (unsigned char)(185 + 70 * pulse);

    // Anchor above the unit sprite
    const float ix = (float)px;
    const float iy = (float)(py - ISO_HALF_H - 28);

    // Red danger glow underneath
    unsigned char ga = (unsigned char)(40 + 60 * pulse);
    DrawCircle((int)ix, (int)iy, 15, Color{200, 20, 20, ga});

    // Drop shadow
    DrawCircle((int)ix + 1, (int)iy + 2, 11, Color{0, 0, 0, (unsigned char)(alpha * 0.4f)});

    // Cranium
    DrawCircle((int)ix, (int)iy, 10, Color{235, 230, 215, alpha});

    // Cheekbone bumps (keep the skull shape wider at mid-face)
    DrawCircle((int)(ix - 7), (int)(iy + 5), 6, Color{235, 230, 215, alpha});
    DrawCircle((int)(ix + 7), (int)(iy + 5), 6, Color{235, 230, 215, alpha});

    // Jaw bar
    DrawRectangle((int)(ix - 8), (int)(iy + 8), 17, 6, Color{235, 230, 215, alpha});

    // Teeth gaps: 3 vertical lines dividing the jaw into 4 teeth
    Color bgCol = {16, 16, 26, alpha};
    for (int i = 1; i <= 3; ++i)
        DrawLine((int)(ix - 8) + i * 4, (int)(iy + 8),
                 (int)(ix - 8) + i * 4, (int)(iy + 14), bgCol);

    // Eye sockets
    DrawCircle((int)(ix - 4), (int)(iy - 2), 3, Color{15, 15, 20, alpha});
    DrawCircle((int)(ix + 4), (int)(iy - 2), 3, Color{15, 15, 20, alpha});

    // Nose cavity (two small holes side by side)
    DrawCircle((int)(ix - 2), (int)(iy + 3), 2, Color{15, 15, 20, alpha});
    DrawCircle((int)(ix + 2), (int)(iy + 3), 2, Color{15, 15, 20, alpha});
}

void GridView::renderAttackableRingsLayer(int px, int py) {
    const float t         = (float)GetTime();
    const float fallPhase = fmodf(t * 1.4f, 1.0f);
    for (int ring = 0; ring < 2; ++ring) {
        float p    = fmodf(fallPhase + ring * 0.5f, 1.0f);
        float yOff = (1.0f - p) * -44.0f;
        float a    = p < 0.65f ? 1.0f : 1.0f - (p - 0.65f) / 0.35f;
        unsigned char ba = (unsigned char)(a * 130.0f);   // reduced alpha
        float lw = 2.0f - p * 0.7f;
        if (lw < 1.0f) lw = 1.0f;

        Vector2 t2 = {(float)px,              (float)(py)            + yOff};
        Vector2 r2 = {(float)(px+ISO_HALF_W), (float)(py+ISO_HALF_H) + yOff};
        Vector2 b2 = {(float)px,              (float)(py+ISO_TILE_H) + yOff};
        Vector2 l2 = {(float)(px-ISO_HALF_W), (float)(py+ISO_HALF_H) + yOff};

        Color c = Color{255, 70, 70, ba};
        DrawLineEx(t2, r2, lw, c);
        DrawLineEx(r2, b2, lw, c);
        DrawLineEx(b2, l2, lw, c);
        DrawLineEx(l2, t2, lw, c);
    }
}

void GridView::renderAttackableHoverLayer(int px, int py) {
    const float t = (float)GetTime();

    // Crossing animation: swords start apart, slide together, hold, separate
    float phase = fmodf(t * 1.7f, 1.0f);
    float sep;
    if (phase < 0.35f)
        sep = 10.0f * (1.0f - phase / 0.35f);   // approaching: 10→0
    else if (phase < 0.65f)
        sep = 0.0f;                               // crossed: hold
    else
        sep = 10.0f * ((phase - 0.65f) / 0.35f); // separating: 0→10

    float pulse = 0.5f + 0.5f * sinf(t * 6.0f);
    unsigned char alpha = (unsigned char)(200 + 55 * pulse);

    // Anchor above the unit sprite centre
    const float ix = (float)px;
    const float iy = (float)(py - ISO_HALF_H - 22);

    const float D  = 0.707f;
    const float BL = 12.0f;
    const float GW = 5.0f;
    const float HX = 3.5f;
    const float GUARD_OFFSET = 5.0f;

    const Color blade = {225, 28, 28, alpha};
    const Color guard = {190, 145, 40, alpha};
    const Color shd   = {0, 0, 0, (unsigned char)(alpha / 2)};

    // ── Sword 1: hilt bottom-left, tip top-right (/) ─────────────────────────
    float s1x = ix - sep;
    Vector2 s1tip  = {s1x + D*BL,  iy - D*BL};
    Vector2 s1hilt = {s1x - D*BL,  iy + D*BL};
    float s1gx = s1x - D*GUARD_OFFSET, s1gy = iy + D*GUARD_OFFSET;
    Vector2 s1gA  = {s1gx + D*GW, s1gy + D*GW};
    Vector2 s1gB  = {s1gx - D*GW, s1gy - D*GW};
    Vector2 s1hX  = {s1hilt.x - D*HX, s1hilt.y + D*HX};

    DrawLineEx({s1hilt.x+1,s1hilt.y+1},{s1tip.x+1,s1tip.y+1}, 3.5f, shd);
    DrawLineEx({s1gA.x+1,  s1gA.y+1},  {s1gB.x+1, s1gB.y+1}, 3.0f, shd);
    DrawLineEx(s1hilt, s1tip, 3.5f, blade);
    DrawLineEx(s1gA,   s1gB,  4.0f, guard);
    DrawLineEx(s1hilt, s1hX,  5.0f, guard);

    // ── Sword 2: hilt bottom-right, tip top-left (\) ─────────────────────────
    float s2x = ix + sep;
    Vector2 s2tip  = {s2x - D*BL,  iy - D*BL};
    Vector2 s2hilt = {s2x + D*BL,  iy + D*BL};
    float s2gx = s2x + D*GUARD_OFFSET, s2gy = iy + D*GUARD_OFFSET;
    Vector2 s2gA  = {s2gx - D*GW, s2gy + D*GW};
    Vector2 s2gB  = {s2gx + D*GW, s2gy - D*GW};
    Vector2 s2hX  = {s2hilt.x + D*HX, s2hilt.y + D*HX};

    DrawLineEx({s2hilt.x+1,s2hilt.y+1},{s2tip.x+1,s2tip.y+1}, 3.5f, shd);
    DrawLineEx({s2gA.x+1,  s2gA.y+1},  {s2gB.x+1, s2gB.y+1}, 3.0f, shd);
    DrawLineEx(s2hilt, s2tip, 3.5f, blade);
    DrawLineEx(s2gA,   s2gB,  4.0f, guard);
    DrawLineEx(s2hilt, s2hX,  5.0f, guard);
}

void GridView::renderBuildingLayer(const World& world, const Position& pos, int px, int py) {
    const Tile* tile = &world.getTileAt(pos);
    if (!tile->hasCity()) return;

    const City* city = tile->getCity();
    Color faction = city->hasOwner()
        ? playerColor(city->getOwner().getId())
        : Color{160, 160, 160, 255};

    static const BuildingType DRAWABLE[] = { BuildingType::BARRACK, BuildingType::FOUNDRY };
    std::vector<BuildingType> present;
    for (BuildingType bt : DRAWABLE) {
        int count = city->countBuildings(bt);
        for (int i = 0; i < count; ++i) present.push_back(bt);
    }

    bool hasScaffold = false;
    BuildingType scaffoldType = BuildingType::BARRACK;
    int scaffoldTurns = 0;
    for (const auto& entry : world.getConstructionQueue()) {
        if (entry.pos == pos) {
            hasScaffold   = true;
            scaffoldType  = entry.type;
            scaffoldTurns = entry.turnsRemaining;
            break;
        }
    }

    const int total = (int)present.size() + (hasScaffold ? 1 : 0);
    if (total == 0) return;

    for (int i = 0; i < (int)present.size(); ++i)
        Sprites::building(present[i], px, py, faction, i, total);

    if (hasScaffold)
        Sprites::buildingScaffold(scaffoldType, px, py, scaffoldTurns, (int)present.size(), total);
}

void GridView::renderCell(const World& world, const Position& pos,
                          bool isHovered, bool isSelected, bool isReachable, bool isAttackable, bool isLethal) {
    int px, py;
    isoTopVertex(pos.row(), pos.col(), px, py);
    const Tile& tile = world.getTileAt(pos);

    // ── Tile-level layers (bottom → top, all drawn before the unit) ──────────
    renderTerrainLayer(tile, px, py);
    renderCityLayer(tile, px, py);
    renderBuildingLayer(world, pos, px, py);
    if (isReachable)                       renderReachableLayer(px, py, isHovered);
    if (isAttackable)                      renderAttackableLayer(px, py);
    if (isAttackable && isHovered && !isLethal) renderAttackableRingsLayer(px, py);
    if (isHovered)                         renderHoverLayer(px, py);
    if (isSelected)                        renderSelectionLayer(px, py);

    // ── Unit sprite (always on top of tile decorations) ───────────────────────
    renderUnitLayer(world, tile, pos, px, py);

    // ── Above-unit overlays ───────────────────────────────────────────────────
    if (isLethal)                          renderLethalLayer(px, py);
    else if (isAttackable && isHovered)    renderAttackableHoverLayer(px, py);
}

void GridView::renderPathArrows(const std::vector<Position>& path) {
    if (path.size() < 2) return;

    // Convert a grid position to the screen-space diamond centre
    auto tileCenter = [&](const Position& pos) -> Vector2 {
        int px, py;
        isoTopVertex(pos.row(), pos.col(), px, py);
        return { (float)px, (float)(py + ISO_HALF_H) };
    };

    const Color lineGlow = Color{40, 190, 255, 110};
    const Color lineCol  = Color{30, 160, 255, 210};
    const Color dotCol   = Color{80, 210, 255, 200};
    const Color headCol  = Color{30, 220, 255, 240};

    // Draw glow + line between each pair of consecutive tile centres
    for (int i = 0; i + 1 < (int)path.size(); ++i) {
        Vector2 a = tileCenter(path[i]);
        Vector2 b = tileCenter(path[i + 1]);
        DrawLineEx(a, b, 5.0f, lineGlow);
        DrawLineEx(a, b, 2.5f, lineCol);
    }

    // Small circles at intermediate waypoints (not origin, not destination)
    for (int i = 1; i + 1 < (int)path.size(); ++i) {
        Vector2 c = tileCenter(path[i]);
        DrawCircleV(c, 4.0f, dotCol);
    }

    // Arrowhead at the destination
    {
        Vector2 from = tileCenter(path[path.size() - 2]);
        Vector2 to   = tileCenter(path[path.size() - 1]);
        float dx = to.x - from.x, dy = to.y - from.y;
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 0.001f) {
            dx /= len; dy /= len;
            float px2 = -dy, py2 = dx;     // perpendicular
            const float AW = 7.0f, AH = 12.0f;
            // tip, left-base, right-base — CCW in Y-down screen space
            Vector2 tip  = to;
            Vector2 left = {to.x - dx*AH + px2*AW, to.y - dy*AH + py2*AW};
            Vector2 right= {to.x - dx*AH - px2*AW, to.y - dy*AH - py2*AW};
            DrawTriangle(tip, left, right, headCol);
        }
    }
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
