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

} // namespace Sprites
