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

void building(BuildingType type, int px, int py, Color factionColor, int slot, int total) {
    // Anchor all buildings in a row along the front base of the city tile.
    // Centre the row horizontally at px, 24px spacing between slots.
    const int spacing = 26;
    const int rowCenterX = px;
    const int startX = rowCenterX - (total - 1) * spacing / 2;
    const int bx = startX + slot * spacing;
    const int by = py + ISO_HALF_H + 18;   // just in front of the castle base

    switch (type) {
        case BuildingType::BARRACK:  drawBarracks(bx, by, factionColor); break;
        case BuildingType::FOUNDRY:  drawFoundry(bx, by);                break;
        default: break;
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
    Color typeCol = (type == BuildingType::BARRACK) ? Color{200, 80, 80, 255}
                 : (type == BuildingType::FOUNDRY)  ? Color{255, 140, 30, 255}
                 :                                    Color{180, 180, 180, 255};
    DrawCircle(bx, by + 4, 4, typeCol);
}

} // namespace Sprites
