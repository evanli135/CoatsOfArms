#include "view/sprites.h"
#include "view/layout.h"
#include "raylib.h"

using namespace Layout;

namespace Sprites {

// ---------------------------------------------------------------------------
// drawDiamond — fills the iso tile face with two triangles.
//   px,py  = top vertex of diamond
//   lc     = colour for the left half (slightly darker = west face)
//   rc     = colour for the right half (slightly lighter = east face)
//
// Winding is CCW (screen-space, y-down) as required by DrawTriangle.
// ---------------------------------------------------------------------------
static void drawDiamond(int px, int py, Color lc, Color rc) {
    Vector2 top = {(float)px,            (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,            (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};
    DrawTriangle(top, lt, bot, lc);   // left half
    DrawTriangle(top, bot, rt, rc);   // right half
}

// Thin grid outline on all four diamond edges.
static void diamondOutline(int px, int py, Color c) {
    Vector2 top = {(float)px,            (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,            (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};
    DrawLineEx(top, rt,  1.0f, c);
    DrawLineEx(rt,  bot, 1.0f, c);
    DrawLineEx(bot, lt,  1.0f, c);
    DrawLineEx(lt,  top, 1.0f, c);
}

// ---------------------------------------------------------------------------
// terrain — procedural iso tile drawing.
//   px,py = top vertex of diamond (same convention used throughout view layer).
// ---------------------------------------------------------------------------
void terrain(Terrain t, int px, int py) {
    const int cx = px;              // horizontal centre of diamond
    const int cy = py + ISO_HALF_H; // vertical centre (equator)

    switch (t) {

        case Terrain::GRASS: {
            drawDiamond(px, py, Color{68, 152, 40, 255}, Color{86, 175, 52, 255});
            // Scattered darker variation patches — stay near the centre to avoid clipping edges
            DrawRectangle(cx-18, cy-7,  13,  7, Color{48, 112, 24, 110});
            DrawRectangle(cx+ 5, cy-5,  11,  6, Color{48, 112, 24, 95});
            DrawRectangle(cx-12, cy+ 5,  9,  5, Color{48, 112, 24, 110});
            DrawRectangle(cx+ 7, cy+ 3, 10,  5, Color{48, 112, 24, 95});
            // Highlight specks
            DrawRectangle(cx-22, cy-2,  3, 2, Color{118, 210, 72, 105});
            DrawRectangle(cx+16, cy+2,  3, 2, Color{118, 210, 72, 100});
            DrawRectangle(cx-4,  cy+8,  3, 2, Color{118, 210, 72, 95});
            // Small wildflower near center-right
            DrawCircle(cx+14, cy-5, 2, Color{255, 255, 255, 200});
            DrawCircle(cx+17, cy-3, 2, Color{255, 255, 255, 200});
            DrawCircle(cx+11, cy-3, 2, Color{255, 255, 255, 200});
            DrawCircle(cx+14, cy-1, 2, Color{255, 255, 255, 200});
            DrawCircle(cx+14, cy-3, 2, Color{245, 210, 48, 230});
            diamondOutline(px, py, Color{0, 0, 0, 45});
            break;
        }

        case Terrain::FOREST: {
            drawDiamond(px, py, Color{24, 66, 16, 255}, Color{32, 82, 22, 255});
            // Forest floor detail
            DrawRectangle(cx-18, cy-4, 10, 5, Color{18, 52, 12, 120});
            DrawRectangle(cx+ 6, cy+2,  9, 4, Color{18, 52, 12, 100});
            diamondOutline(px, py, Color{0, 0, 0, 55});
            // Pine tree — trunk anchored at the tile equator so only the top
            // tier slightly clears the tile face (12 px vs the old 40 px).
            const int tb = py + ISO_HALF_H;  // tree base = equator (py+24)
            DrawRectangle(cx-2, tb-16, 5, 16, Color{78, 46, 14, 255}); // trunk py+8→py+24
            DrawTriangle(                                             // bottom tier
                {(float)cx,      (float)(tb-20)},   // py+4
                {(float)(cx-14), (float)(tb- 4)},   // py+20
                {(float)(cx+14), (float)(tb- 4)},
                Color{20, 72, 16, 255});
            DrawTriangle(                                             // middle tier
                {(float)cx,      (float)(tb-28)},   // py-4
                {(float)(cx-10), (float)(tb-14)},   // py+10
                {(float)(cx+10), (float)(tb-14)},
                Color{30, 95, 22, 255});
            DrawTriangle(                                             // top tier
                {(float)cx,     (float)(tb-36)},    // py-12
                {(float)(cx-7), (float)(tb-22)},    // py+2
                {(float)(cx+7), (float)(tb-22)},
                Color{42, 122, 32, 255});
            break;
        }

        case Terrain::MOUNTAIN: {
            drawDiamond(px, py, Color{84, 82, 100, 255}, Color{106, 104, 124, 255});
            diamondOutline(px, py, Color{0, 0, 0, 60});
            // Mountain — base spans the upper tile face (equator), peak 30px above tile top.
            const int pk  = py - 30;          // peak
            const int mby = py + ISO_HALF_H;  // base at tile equator (py+24)
            // Left face (darker)
            DrawTriangle(
                {(float)cx,                  (float)pk},
                {(float)(cx - ISO_HALF_W/2), (float)mby},
                {(float)cx,                  (float)mby},
                Color{80, 78, 98, 255});
            // Right face (lighter)
            DrawTriangle(
                {(float)cx,                  (float)pk},
                {(float)cx,                  (float)mby},
                {(float)(cx + ISO_HALF_W/2), (float)mby},
                Color{118, 116, 140, 255});
            // Rock shadow line
            DrawLine(cx-5, pk+14, cx-12, pk+22, Color{50, 48, 64, 140});
            // Snow cap
            DrawTriangle(
                {(float)cx,      (float)pk},
                {(float)(cx-10), (float)(pk+12)},
                {(float)(cx+10), (float)(pk+12)},
                Color{228, 235, 255, 255});
            // Snow glint
            DrawTriangle(
                {(float)cx,     (float)pk},
                {(float)(cx-4), (float)(pk+7)},
                {(float)(cx+4), (float)(pk+7)},
                Color{255, 255, 255, 210});
            break;
        }

        case Terrain::OCEAN: {
            drawDiamond(px, py, Color{12, 38, 135, 255}, Color{16, 48, 160, 255});
            // Depth gradient toward bottom vertex
            DrawTriangle(
                {(float)(cx-26), (float)(cy-4)},
                {(float)(cx+26), (float)(cy-4)},
                {(float)cx,      (float)(py+ISO_TILE_H)},
                Color{8, 22, 88, 55});
            // Wave shimmer
            DrawLineEx({(float)(cx-20),(float)(cy-7)}, {(float)(cx-8),(float)(cy-11)}, 1.5f, Color{80,155,255,100});
            DrawLineEx({(float)(cx-8), (float)(cy-11)},{(float)cx,   (float)(cy-9)},  1.5f, Color{80,155,255,100});
            DrawLineEx({(float)cx,     (float)(cy-9)}, {(float)(cx+12),(float)(cy-13)},1.5f, Color{80,155,255,100});
            DrawLineEx({(float)(cx+2), (float)(cy+1)}, {(float)(cx+14),(float)(cy-3)}, 1.2f, Color{80,155,255,65});
            diamondOutline(px, py, Color{0, 0, 0, 55});
            break;
        }

        case Terrain::RIVER: {
            drawDiamond(px, py, Color{38, 100, 172, 255}, Color{48, 122, 198, 255});
            // Central channel running top-to-bottom through the diamond
            DrawLineEx({(float)cx,(float)py}, {(float)cx,(float)(py+ISO_TILE_H)}, 18.0f, Color{60,140,215,190});
            DrawLineEx({(float)cx,(float)py}, {(float)cx,(float)(py+ISO_TILE_H)},  7.0f, Color{95,172,238,170});
            // Ripple chevrons
            for (int i = 0; i < 3; ++i) {
                float ry = py + 7.0f + i * 11.0f;
                DrawLineEx({(float)(cx-9), ry+5.0f}, {(float)cx,      ry},       1.5f, Color{155,210,255,125});
                DrawLineEx({(float)cx,     ry},       {(float)(cx+9),  ry+5.0f}, 1.5f, Color{155,210,255,125});
            }
            diamondOutline(px, py, Color{0, 0, 0, 55});
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// city — small iso castle drawn above the tile centre.
// ---------------------------------------------------------------------------
void city(int px, int py) {
    const int cx = px;
    const Color wall = Color{218, 190, 124, 248};
    const Color dark = Color{145, 112,  68, 248};
    const Color gate = Color{ 55,  36,  18, 248};
    const Color flag = Color{186,  52,  52, 248};

    // Castle sits above the tile's top vertex
    const int bx = cx - 10;
    const int by = py - 26;

    DrawRectangle(bx,      by+8,  20, 14, wall);
    DrawLine(bx,     by+12, bx+20, by+12, Color{0,0,0,28});
    DrawRectangle(bx+1,    by+3,   5,  7, wall);
    DrawRectangle(bx+8,    by+3,   5,  7, wall);
    DrawRectangle(bx+14,   by+3,   5,  7, wall);
    DrawRectangle(bx+8,    by+5,   5,  6, dark);
    DrawRectangle(bx+6,    by+14,  8,  8, gate);
    DrawCircle(bx+10, by+14, 4, gate);
    DrawLine(bx+10, by+3, bx+10, by-4, dark);
    DrawTriangle(
        {(float)(bx+10),(float)(by-4)},
        {(float)(bx+10),(float)(by+1)},
        {(float)(bx+15),(float)(by-2)},
        flag);
}

} // namespace Sprites
