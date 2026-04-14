#include "view/sprites.h"
#include "view/layout.h"
#include "raylib.h"
#include <algorithm>

using namespace Layout;

namespace Sprites {

// ---------------------------------------------------------------------------
// city — iso castle.  Wider at the base, narrower keep at the top.
//        Roofs, flag, and wall trim are tinted by factionColor.
// px,py = top vertex of the diamond tile.
// ---------------------------------------------------------------------------
void city(int px, int py, Color factionColor) {
    // Stone palette (neutral)
    const Color stone  = Color{210, 185, 130, 255};
    const Color shadow = Color{130, 105,  60, 255};
    const Color dark   = Color{ 70,  48,  18, 255};
    const Color gate   = Color{ 35,  20,   8, 255};
    const Color mortar = Color{170, 148, 100, 130};

    // Faction-tinted colours: roof = faction, flag = faction darkened,
    // trim = faction at low alpha overlaid on stone
    auto darkenC = [](Color c, float f) -> Color {
        return Color{
            (unsigned char)(c.r * f),
            (unsigned char)(c.g * f),
            (unsigned char)(c.b * f),
            c.a };
    };
    const Color roofCol  = factionColor;
    const Color flagCol  = darkenC(factionColor, 0.75f);
    const Color trimCol  = Color{factionColor.r, factionColor.g, factionColor.b, 80};

    const int cx = px;
    const int cy = py + ISO_HALF_H;

    // ── Helper: merlons along the top of a wall/tower ───────────────────────
    auto battlements = [&](int x0, int y0, int w, int mw, int h) {
        for (int i = 0; i + mw <= w; i += mw * 2)
            DrawRectangle(x0 + i, y0 - h, mw, h, stone);
    };

    // ── Helper: draw one circular tower ─────────────────────────────────────
    // tw/th = tower body width/height; roofH = cone height above body
    auto drawTower = [&](int tx, int ty, int tw, int th, int roofH) {
        // Body
        DrawRectangle(tx - tw/2, ty - th, tw, th, shadow);
        // Faction-coloured top trim strip
        DrawRectangle(tx - tw/2, ty - th, tw, 3, trimCol.a > 0 ? stone : stone);
        DrawRectangle(tx - tw/2, ty - th, tw, 3, trimCol);
        // Horizontal mortar bands
        for (int band = 5; band < th; band += 6)
            DrawLine(tx - tw/2, ty - th + band, tx + tw/2, ty - th + band, mortar);
        // Battlements
        battlements(tx - tw/2, ty - th, tw, 3, 4);
        // Pointed conical roof (faction-coloured)
        DrawTriangle(
            {(float)tx,               (float)(ty - th - roofH)},
            {(float)(tx - tw/2 - 1),  (float)(ty - th + 1)},
            {(float)(tx + tw/2 + 1),  (float)(ty - th + 1)},
            roofCol);
        // Arrow-slit window
        DrawRectangle(tx - 1, ty - th + 7, 3, 6, dark);
    };

    // ══════════════════════════════════════════════════════════════
    // 1. CURTAIN WALL  — wide base
    // ══════════════════════════════════════════════════════════════
    const int wallW = 68, wallH = 16;
    const int wallX = cx - wallW / 2, wallY = cy - 12;

    DrawRectangle(wallX, wallY, wallW, wallH, shadow);
    DrawRectangle(wallX, wallY, wallW, 2, trimCol);   // faction trim along top
    DrawLine(wallX, wallY + wallH/2, wallX + wallW, wallY + wallH/2, mortar);
    battlements(wallX + 3, wallY, wallW - 6, 5, 5);

    // Gate arch (slightly wider)
    const int gw = 12, gh = 13;
    const int gx = cx - gw/2, gy = wallY + wallH - gh;
    DrawRectangle(gx, gy, gw, gh, gate);
    DrawCircle(cx, gy, gw/2, gate);
    // Arch trim
    DrawRectangleLines(gx, gy, gw, gh, darkenC(factionColor, 0.5f));

    // ══════════════════════════════════════════════════════════════
    // 2. FLANKING TOWERS  — placed at the wall ends, narrower body
    // ══════════════════════════════════════════════════════════════
    drawTower(wallX + 1,         cy - 4, 14, 30, 16);  // left
    drawTower(wallX + wallW - 1, cy - 4, 14, 30, 16);  // right

    // ══════════════════════════════════════════════════════════════
    // 3. CENTRAL KEEP  — narrower than before, taller pointed roof
    // ══════════════════════════════════════════════════════════════
    const int kw = 14, kh = 44;
    const int kx = cx, ky = cy - 10;

    DrawRectangle(kx - kw/2, ky - kh, kw, kh, shadow);
    DrawRectangle(kx - kw/2, ky - kh, kw, 2, trimCol);
    for (int band = 6; band < kh; band += 7)
        DrawLine(kx - kw/2, ky - kh + band, kx + kw/2, ky - kh + band, mortar);
    battlements(kx - kw/2, ky - kh, kw, 3, 5);

    // Keep windows (two levels)
    DrawRectangle(kx - 3, ky - kh + 11, 3, 7, dark);
    DrawRectangle(kx + 1, ky - kh + 11, 3, 7, dark);
    DrawRectangle(kx - 3, ky - kh + 26, 3, 7, dark);
    DrawRectangle(kx + 1, ky - kh + 26, 3, 7, dark);

    // Keep roof — tall and pointed (faction-coloured)
    const int keepRoofH = 22;
    DrawTriangle(
        {(float)kx,               (float)(ky - kh - keepRoofH)},
        {(float)(kx - kw/2 - 1),  (float)(ky - kh + 2)},
        {(float)(kx + kw/2 + 1),  (float)(ky - kh + 2)},
        roofCol);

    // Flagpole + two-tone pennant (faction colour over white)
    const int poleTop = ky - kh - keepRoofH - 12;
    DrawLine(kx, ky - kh - keepRoofH, kx, poleTop, dark);
    DrawTriangle(
        {(float)kx,        (float)(poleTop)},
        {(float)kx,        (float)(poleTop + 9)},
        {(float)(kx + 12), (float)(poleTop + 4)},
        Color{255, 255, 255, 220});                 // white half
    DrawTriangle(
        {(float)kx,        (float)(poleTop + 4)},
        {(float)kx,        (float)(poleTop + 9)},
        {(float)(kx + 12), (float)(poleTop + 4)},
        flagCol);                                   // faction half
}

// ---------------------------------------------------------------------------
// building — small structure drawn in front of the city castle.
// (bx, by) = bottom-center anchor of the building.
// ---------------------------------------------------------------------------
static void drawBarracks(int bx, int by, Color faction) {
    const Color wall   = Color{175, 140,  85, 255};
    const Color roof   = Color{155,  40,  40, 255};
    const Color dark   = Color{ 35,  18,   6, 220};
    const Color mortar = Color{140, 110,  70, 140};

    // Wall
    DrawRectangle(bx - 9, by - 16, 18, 14, wall);
    DrawLine(bx - 9, by - 10, bx + 9, by - 10, mortar);
    // Roof
    DrawTriangle({(float)bx,        (float)(by - 16 - 9)},
                 {(float)(bx - 10), (float)(by - 16)},
                 {(float)(bx + 10), (float)(by - 16)},
                 roof);
    // Door
    DrawRectangle(bx - 2, by - 9, 5, 9, dark);
    // Window slits
    DrawRectangle(bx - 7, by - 14, 3, 4, dark);
    DrawRectangle(bx + 5, by - 14, 3, 4, dark);
    // Faction pennant at peak
    DrawTriangle({(float)bx,       (float)(by - 25)},
                 {(float)bx,       (float)(by - 21)},
                 {(float)(bx + 6), (float)(by - 23)},
                 faction);
}

static void drawFoundry(int bx, int by) {
    const Color stone  = Color{ 85,  68,  50, 255};
    const Color dark   = Color{ 50,  38,  24, 255};
    const Color ember  = Color{255, 140,  30, 255};
    const Color glow   = Color{200,  75,  15, 210};
    const Color mortar = Color{ 65,  50,  36, 160};

    // Main structure
    DrawRectangle(bx - 9, by - 16, 18, 16, stone);
    DrawLine(bx - 9, by - 10, bx + 9, by - 10, mortar);
    DrawLine(bx - 9, by - 5,  bx + 9, by - 5,  mortar);
    // Chimney
    DrawRectangle(bx - 8, by - 27, 7, 16, dark);
    DrawRectangle(bx - 9, by - 29, 9, 3,  dark);  // cap
    // Ember glow at chimney top
    DrawCircle(bx - 4, by - 29, 4, ember);
    DrawCircle(bx - 4, by - 29, 2, Color{255, 220, 80, 255});
    // Forge window (orange glow)
    DrawRectangle(bx + 2, by - 13, 5, 8, glow);
    DrawRectangle(bx + 3, by - 12, 3, 5, Color{255, 180, 60, 255});
}

static void drawFarm(int bx, int by) {
    const Color soil  = Color{145, 100,  55, 255};
    const Color straw = Color{210, 175,  60, 255};
    const Color wood  = Color{160, 110,  50, 255};
    const Color dark  = Color{ 60,  38,  14, 220};

    // Barn body
    DrawRectangle(bx - 8, by - 14, 16, 12, wood);
    // Pitched roof
    DrawTriangle({(float)bx,        (float)(by - 14 - 8)},
                 {(float)(bx - 9),  (float)(by - 14)},
                 {(float)(bx + 9),  (float)(by - 14)},
                 straw);
    // Door
    DrawRectangle(bx - 2, by - 8, 5, 8, dark);
    // Hay cross on door
    DrawLine(bx - 2, by - 8, bx + 3, by - 2, Color{190, 150, 50, 180});
    DrawLine(bx + 3, by - 8, bx - 2, by - 2, Color{190, 150, 50, 180});
    // Soil strip at base
    DrawRectangle(bx - 10, by - 2, 20, 3, soil);
}

static void drawFishery(int bx, int by) {
    const Color planks = Color{130, 100,  65, 255};
    const Color dark   = Color{ 50,  35,  16, 220};
    const Color water  = Color{ 60, 130, 190, 200};
    const Color rope   = Color{200, 170,  90, 200};

    // Dock platform
    DrawRectangle(bx - 10, by - 5,  20, 4, planks);
    DrawLine(bx - 8, by - 5,  bx - 8, by - 1, dark);
    DrawLine(bx,     by - 5,  bx,     by - 1, dark);
    DrawLine(bx + 8, by - 5,  bx + 8, by - 1, dark);
    // Water shimmer
    DrawRectangle(bx - 10, by - 1,  20, 3, water);
    // Hut on dock
    DrawRectangle(bx - 6, by - 16, 12, 11, planks);
    DrawTriangle({(float)bx,       (float)(by - 16 - 6)},
                 {(float)(bx - 7), (float)(by - 16)},
                 {(float)(bx + 7), (float)(by - 16)},
                 dark);
    // Fishing pole
    DrawLine(bx + 5, by - 20, bx + 12, by - 5, rope);
    DrawCircle(bx + 12, by - 5, 2, Color{255, 255, 255, 200});
}

static void drawLumberCamp(int bx, int by) {
    const Color log   = Color{140,  90,  45, 255};
    const Color bark  = Color{ 90,  60,  25, 255};
    const Color stump = Color{120,  80,  40, 255};
    const Color blade = Color{190, 190, 200, 240};

    // Log pile (three stacked logs)
    DrawRectangle(bx - 9, by - 8,  18, 6, log);
    DrawLine(bx - 9, by - 5, bx + 9, by - 5, bark);
    DrawRectangle(bx - 7, by - 13, 14, 5, log);
    DrawLine(bx - 7, by - 11, bx + 7, by - 11, bark);
    DrawRectangle(bx - 5, by - 17, 10, 4, log);
    // Stump
    DrawRectangle(bx - 5, by - 4,  10, 4, stump);
    // Axe
    DrawLine(bx + 7, by - 22, bx + 3, by - 6, Color{160, 110, 50, 220});
    DrawTriangle({(float)(bx + 7),  (float)(by - 22)},
                 {(float)(bx + 13), (float)(by - 19)},
                 {(float)(bx + 9),  (float)(by - 14)},
                 blade);
}

static void drawMine(int bx, int by) {
    const Color stone  = Color{100,  90,  80, 255};
    const Color dark   = Color{ 25,  18,  12, 255};
    const Color timber = Color{160, 120,  55, 255};
    const Color ore    = Color{200, 160,  50, 230};

    // Rock face
    DrawRectangle(bx - 11, by - 14, 22, 13, stone);
    // Mine entrance arch
    DrawRectangle(bx - 5, by - 12,  10, 11, dark);
    DrawCircle(bx, by - 12, 5, dark);
    // Timber frame
    DrawLine(bx - 6, by - 14, bx - 6, by - 2, timber);
    DrawLine(bx + 6, by - 14, bx + 6, by - 2, timber);
    DrawLine(bx - 6, by - 14, bx + 6, by - 14, timber);
    // Ore glint inside
    DrawCircle(bx - 2, by - 8, 2, ore);
    DrawCircle(bx + 3, by - 10, 2, ore);
    // Cart rail line
    DrawLine(bx - 9, by, bx + 9, by, Color{130, 100, 55, 200});
}

void building(BuildingType type, int px, int py, Color factionColor, int slot, int total) {
    // Anchor all buildings in a row along the front base of the city tile.
    // Centre the row horizontally at px, 24px spacing between slots.
    const int spacing = 26;
    const int rowCenterX = px;
    const int startX = rowCenterX - (total - 1) * spacing / 2;
    const int bx = startX + slot * spacing;
    const int by = py + ISO_HALF_H + 18;   // just in front of the castle base

    switch (type) {
        case BuildingType::BARRACK:      drawBarracks(bx, by, factionColor); break;
        case BuildingType::FOUNDRY:      drawFoundry(bx, by);                break;
        case BuildingType::FARM:         drawFarm(bx, by);                   break;
        case BuildingType::FISHERY:      drawFishery(bx, by);                break;
        case BuildingType::LUMBER_CAMP:  drawLumberCamp(bx, by);             break;
        case BuildingType::MINE:         drawMine(bx, by);                   break;
    }
}

// ---------------------------------------------------------------------------
// buildingScaffold — wooden construction frame for in-progress buildings.
// ---------------------------------------------------------------------------
void buildingScaffold(BuildingType type, int px, int py, int turnsLeft, int slot, int total) {
    const int spacing    = 26;
    const int startX     = px - (total - 1) * spacing / 2;
    const int bx         = startX + slot * spacing;
    const int by         = py + ISO_HALF_H + 18;

    const Color wood  = Color{180, 135,  70, 220};
    const Color plank = Color{155, 110,  50, 200};

    // Vertical poles
    DrawLine(bx - 9, by,      bx - 9, by - 22, wood);
    DrawLine(bx + 9, by,      bx + 9, by - 22, wood);
    // Horizontal cross-planks
    DrawLine(bx - 9, by - 6,  bx + 9, by - 6,  plank);
    DrawLine(bx - 9, by - 13, bx + 9, by - 13, plank);
    DrawLine(bx - 9, by - 20, bx + 9, by - 20, plank);
    // Diagonal bracing
    DrawLine(bx - 9, by,      bx + 9, by - 13, Color{140, 100, 45, 160});
    DrawLine(bx + 9, by,      bx - 9, by - 13, Color{140, 100, 45, 160});

    // "Turns left" label above the scaffold
    const char* label = TextFormat("%d", turnsLeft);
    int lw = MeasureText(label, 11);
    DrawText(label, bx - lw / 2, by - 30, 11, Color{220, 220, 100, 255});

    // Building-type icon below (tiny coloured dot so player knows what's coming)
    Color typeCol = (type == BuildingType::BARRACK)     ? Color{200,  80,  80, 255}
                 : (type == BuildingType::FOUNDRY)      ? Color{255, 140,  30, 255}
                 : (type == BuildingType::FARM)         ? Color{160, 200,  60, 255}
                 : (type == BuildingType::FISHERY)      ? Color{ 60, 160, 220, 255}
                 : (type == BuildingType::LUMBER_CAMP)  ? Color{160, 110,  50, 255}
                 : (type == BuildingType::MINE)         ? Color{200, 160,  50, 255}
                 :                                        Color{180, 180, 180, 255};
    DrawCircle(bx, by + 4, 4, typeCol);
}

// ---------------------------------------------------------------------------
// shrine — stone altar with a hovering spirit orb.
// px, py = top vertex of the isometric diamond tile (standard convention).
// ---------------------------------------------------------------------------
void shrine(int px, int py) {
    const int cx = px;
    const int cy = py + ISO_HALF_H;   // tile centre y

    const Color stone  = Color{130, 118, 105, 255};
    const Color dark   = Color{ 80,  70,  60, 255};
    const Color edge   = Color{165, 152, 135, 200};
    const Color orb    = Color{170, 130, 255, 230};
    const Color orbHi  = Color{220, 195, 255, 255};
    const Color rune   = Color{155, 120, 210, 210};

    // ── Raised stone platform (flat diamond) ────────────────────────────────
    DrawTriangle({(float)cx,       (float)(cy - 5)},
                 {(float)(cx - 16),(float)(cy + 2)},
                 {(float)cx,       (float)(cy + 9)}, stone);
    DrawTriangle({(float)cx,       (float)(cy - 5)},
                 {(float)cx,       (float)(cy + 9)},
                 {(float)(cx + 16),(float)(cy + 2)},
                 Color{(unsigned char)(stone.r-20), (unsigned char)(stone.g-18), (unsigned char)(stone.b-16), 255});

    // ── Stone pillar ─────────────────────────────────────────────────────────
    DrawRectangle(cx - 3, cy - 25, 7, 20, dark);
    DrawLine(cx - 3, cy - 25, cx - 3, cy - 5, edge);   // left highlight
    // Horizontal mortar bands
    DrawLine(cx - 3, cy - 18, cx + 4, cy - 18, Color{100, 90, 80, 160});
    DrawLine(cx - 3, cy - 11, cx + 4, cy - 11, Color{100, 90, 80, 160});
    // Rune glyphs on pillar face
    DrawLine(cx - 1, cy - 22, cx + 2, cy - 22, rune);
    DrawLine(cx,     cy - 22, cx,     cy - 20, rune);
    DrawLine(cx - 1, cy - 15, cx + 2, cy - 15, rune);
    DrawLine(cx - 1, cy - 13, cx + 2, cy - 13, rune);

    // ── Pillar capital (wider top plate) ─────────────────────────────────────
    DrawRectangle(cx - 5, cy - 27, 11, 3, stone);

    // ── Hovering spirit orb ───────────────────────────────────────────────────
    // Outer soft glow
    DrawCircle(cx, cy - 33, 8, Color{orb.r, orb.g, orb.b, 60});
    DrawCircle(cx, cy - 33, 6, Color{orb.r, orb.g, orb.b, 120});
    // Solid orb
    DrawCircle(cx, cy - 33, 5, orb);
    // Bright highlight
    DrawCircle(cx - 1, cy - 34, 2, orbHi);

    // ── Thin tether line from capital to orb ─────────────────────────────────
    DrawLine(cx, cy - 27, cx, cy - 28, Color{rune.r, rune.g, rune.b, 180});
}

} // namespace Sprites
