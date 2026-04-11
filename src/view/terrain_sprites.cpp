#include "view/sprites.h"
#include "view/layout.h"
#include "raylib.h"

using namespace Layout;

namespace Sprites {

// ---------------------------------------------------------------------------
// drawDiamond — fills the iso tile face with two triangles.
// ---------------------------------------------------------------------------
static void drawDiamond(int px, int py, Color lc, Color rc) {
    Vector2 top = {(float)px,             (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,             (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};
    DrawTriangle(top, lt, bot, lc);
    DrawTriangle(top, bot, rt, rc);
}

static void diamondOutline(int px, int py, Color c) {
    Vector2 top = {(float)px,             (float)py};
    Vector2 rt  = {(float)(px+ISO_HALF_W),(float)(py+ISO_HALF_H)};
    Vector2 bot = {(float)px,             (float)(py+ISO_TILE_H)};
    Vector2 lt  = {(float)(px-ISO_HALF_W),(float)(py+ISO_HALF_H)};
    DrawLineEx(top, rt,  1.0f, c);
    DrawLineEx(rt,  bot, 1.0f, c);
    DrawLineEx(bot, lt,  1.0f, c);
    DrawLineEx(lt,  top, 1.0f, c);
}


// ---------------------------------------------------------------------------
// terrain
// ---------------------------------------------------------------------------
void terrain(Terrain t, int px, int py) {
    const int cx = px;
    const int cy = py + ISO_HALF_H;   // equator (py+24)

    switch (t) {

        // ------------------------------------------------------------------ GRASS
        case Terrain::GRASS: {
            drawDiamond(px, py, Color{68, 152, 40, 255}, Color{86, 175, 52, 255});
            // Scattered darker variation patches
            DrawRectangle(cx-18, cy-7,  13,  7, Color{48, 112, 24, 110});
            DrawRectangle(cx+ 5, cy-5,  11,  6, Color{48, 112, 24,  95});
            DrawRectangle(cx-12, cy+ 5,  9,  5, Color{48, 112, 24, 110});
            DrawRectangle(cx+ 7, cy+ 3, 10,  5, Color{48, 112, 24,  95});
            // Highlight specks
            DrawRectangle(cx-22, cy-2,  3, 2, Color{118, 210, 72, 105});
            DrawRectangle(cx+16, cy+2,  3, 2, Color{118, 210, 72, 100});
            DrawRectangle(cx- 4, cy+8,  3, 2, Color{118, 210, 72,  95});
            // Small wildflower near center-right
            DrawCircle(cx+14, cy-5, 2, Color{255, 255, 255, 200});
            DrawCircle(cx+17, cy-3, 2, Color{255, 255, 255, 200});
            DrawCircle(cx+11, cy-3, 2, Color{255, 255, 255, 200});
            DrawCircle(cx+14, cy-1, 2, Color{255, 255, 255, 200});
            DrawCircle(cx+14, cy-3, 2, Color{245, 210,  48, 230});
            diamondOutline(px, py, Color{0, 0, 0, 45});
            break;
        }

        // ------------------------------------------------------------------ FOREST
        case Terrain::FOREST: {
            drawDiamond(px, py, Color{20, 55, 12, 255}, Color{28, 70, 18, 255});
            diamondOutline(px, py, Color{0, 0, 0, 55});

            const int tb = py + 4;
            DrawRectangle(cx-3, tb, 6, 20, Color{58, 34, 10, 255});

            DrawCircle(cx,    py -  6, 18, Color{16, 72, 12, 255});
            DrawCircle(cx-14, py -  2, 14, Color{20, 80, 16, 255});
            DrawCircle(cx+14, py -  2, 14, Color{20, 80, 16, 255});
            DrawCircle(cx,    py +  2, 17, Color{26, 96, 20, 255});
            DrawCircle(cx-10, py +  6, 12, Color{30, 108, 24, 255});
            DrawCircle(cx+10, py +  6, 12, Color{30, 108, 24, 255});
            DrawCircle(cx,    py +  8, 13, Color{36, 120, 28, 255});
            DrawCircle(cx,    py -  8,  7, Color{52, 148, 36, 255});
            DrawCircle(cx-6,  py -  4,  5, Color{48, 140, 32, 255});
            DrawCircle(cx+6,  py -  4,  5, Color{48, 140, 32, 255});
            break;
        }

        // ------------------------------------------------------------------ MOUNTAIN
        // Pass 1: explicit solid base so the diamond is always opaque.
        // Pass 2: four pyramid faces anchored to the diamond vertices so the
        //         peak grows directly from the tile rim.
        case Terrain::MOUNTAIN: {
            drawDiamond(px, py, Color{60, 58, 64, 255}, Color{72, 70, 76, 255});

            const int h = 30;

            Vector2 P = {(float)cx,              (float)(py - h)};
            Vector2 T = {(float)cx,              (float)py};
            Vector2 R = {(float)(cx+ISO_HALF_W), (float)(py+ISO_HALF_H)};
            Vector2 B = {(float)cx,              (float)(py+ISO_TILE_H)};
            Vector2 L = {(float)(cx-ISO_HALF_W), (float)(py+ISO_HALF_H)};

            // CCW in screen-space (Y-down) so Raylib fills the triangles.
            DrawTriangle(L, T, P, Color{95,  93,  99,  255}); // NW
            DrawTriangle(T, R, P, Color{112, 110, 116, 255}); // NE
            DrawTriangle(L, B, P, Color{95,  93,  99,  255}); // SW
            DrawTriangle(B, R, P, Color{128, 126, 132, 255}); // SE

            // Ridge seams — thin dark lines sharpening the silhouette
            DrawLineEx(P, T, 1.2f, Color{38, 36, 42, 255});
            DrawLineEx(P, L, 1.2f, Color{38, 36, 42, 255});
            DrawLineEx(P, R, 1.2f, Color{38, 36, 42, 255});
            DrawLineEx(P, B, 1.2f, Color{38, 36, 42, 255});

            // Snow cap — small triangle at the tip, left half slightly cooler
            const int sc = 8;  // cap half-width
            Vector2 CL = {(float)(cx - sc), (float)(py - h + sc + 2)};
            Vector2 CR = {(float)(cx + sc), (float)(py - h + sc + 2)};
            DrawTriangle(CL, {(float)cx, (float)(py - h + sc + 2)}, P, Color{210, 215, 220, 255}); // left snow (CCW)
            DrawTriangle({(float)cx, (float)(py - h + sc + 2)}, CR, P, Color{235, 238, 242, 255}); // right snow (CCW)
            break;
        }

        // ------------------------------------------------------------------ OCEAN
        case Terrain::OCEAN: {
            drawDiamond(px, py, Color{12, 38, 135, 255}, Color{16, 48, 160, 255});
            DrawTriangle(
                {(float)(cx-26), (float)(cy-4)},
                {(float)(cx+26), (float)(cy-4)},
                {(float)cx,      (float)(py+ISO_TILE_H)},
                Color{8, 22, 88, 55});
            DrawLineEx({(float)(cx-22),(float)(cy- 8)},{(float)(cx-10),(float)(cy-12)},1.5f,Color{88,168,255,110});
            DrawLineEx({(float)(cx-10),(float)(cy-12)},{(float)cx,     (float)(cy-10)},1.5f,Color{88,168,255,110});
            DrawLineEx({(float)cx,    (float)(cy-10)},{(float)(cx+14),(float)(cy-14)}, 1.5f,Color{88,168,255,110});
            DrawLineEx({(float)(cx+4),(float)(cy+ 2)},{(float)(cx+16),(float)(cy- 2)}, 1.2f,Color{88,168,255, 70});
            DrawLineEx({(float)(cx-18),(float)(cy+3)},{(float)(cx- 8),(float)(cy+  0)},1.2f,Color{88,168,255, 70});
            DrawCircle(cx-6,  cy-10, 2, Color{155,210,255, 80});
            DrawCircle(cx+10, cy-12, 2, Color{155,210,255, 80});
            diamondOutline(px, py, Color{0, 0, 0, 55});
            break;
        }

        // ------------------------------------------------------------------ RIVER
        // Clean light-blue water block with flowing current lines — no rocky shores.
        case Terrain::RIVER: {
            // Light blue water base
            drawDiamond(px, py, Color{72, 168, 212, 255}, Color{92, 192, 232, 255});

            // Deeper tone towards the bottom for a subtle gradient
            DrawTriangle(
                {(float)(cx-20), (float)(cy+4)},
                {(float)(cx+20), (float)(cy+4)},
                {(float)cx,      (float)(py+ISO_TILE_H)},
                Color{50, 130, 180, 60});

            // Current lines — diagonal chevrons suggesting water flowing top→bottom.
            // Two tiers: upper half and lower half.
            const float cw = 1.8f;
            const Color curr = Color{195, 238, 255, 180};
            const Color currD = Color{55, 140, 190, 120};

            // Upper chevron pair
            DrawLineEx({(float)(cx-14),(float)(cy-14)},{(float)cx,      (float)(cy- 8)},cw,curr);
            DrawLineEx({(float)cx,     (float)(cy- 8)},{(float)(cx+14),(float)(cy-14)}, cw,curr);
            // Mid chevron pair
            DrawLineEx({(float)(cx-14),(float)(cy+ 0)},{(float)cx,      (float)(cy+ 6)},cw,curr);
            DrawLineEx({(float)cx,     (float)(cy+ 6)},{(float)(cx+14),(float)(cy+ 0)}, cw,curr);
            // Lower chevron pair
            DrawLineEx({(float)(cx-10),(float)(cy+10)},{(float)cx,      (float)(cy+16)},cw,curr);
            DrawLineEx({(float)cx,     (float)(cy+16)},{(float)(cx+10),(float)(cy+10)}, cw,curr);

            // Darker undercurrent streaks for depth
            DrawLineEx({(float)(cx-8),(float)(cy-10)},{(float)(cx-16),(float)(cy- 4)},1.2f,currD);
            DrawLineEx({(float)(cx+8),(float)(cy+ 4)},{(float)(cx+16),(float)(cy-  2)},1.2f,currD);

            diamondOutline(px, py, Color{40, 130, 180, 80});
            break;
        }
    }
}

} // namespace Sprites
// city() is in city_sprite.cpp
