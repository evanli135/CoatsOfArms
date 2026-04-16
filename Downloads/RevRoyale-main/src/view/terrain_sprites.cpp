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
void terrain(Terrain t, TileResourceValue rv, int px, int py) {
    const int cx = px;
    const int cy = py + ISO_HALF_H;   // equator (py+24)

    switch (t) {

        // ------------------------------------------------------------------ GRASS
        case Terrain::GRASS: {
            // LOW  = dry/sparse — yellower tone, few patches, no flowers
            // MED  = default — patches + one wildflower cluster
            // HIGH = lush     — brighter, more patches, multiple flower clusters
            if (rv == TileResourceValue::LOW) {
                // Dry, yellowed grass
                drawDiamond(px, py, Color{110, 138, 30, 255}, Color{130, 158, 38, 255});
                DrawRectangle(cx-18, cy-7,  13, 7, Color{85, 108, 18, 100});
                DrawRectangle(cx+ 5, cy-5,  11, 6, Color{85, 108, 18,  80});
                // A few dry straw-colored specks
                DrawRectangle(cx-22, cy-2, 3, 2, Color{180, 175, 60, 90});
                DrawRectangle(cx+16, cy+2, 3, 2, Color{180, 175, 60, 80});
                diamondOutline(px, py, Color{0, 0, 0, 40});
            } else if (rv == TileResourceValue::HIGH) {
                // Rich, lush green
                drawDiamond(px, py, Color{52, 170, 30, 255}, Color{72, 200, 45, 255});
                // More varied dark-green patches
                DrawRectangle(cx-18, cy-7,  14, 8, Color{32,  95, 14, 130});
                DrawRectangle(cx+ 5, cy-5,  12, 7, Color{32,  95, 14, 120});
                DrawRectangle(cx-12, cy+ 5, 10, 6, Color{32,  95, 14, 130});
                DrawRectangle(cx+ 7, cy+ 3, 11, 6, Color{32,  95, 14, 115});
                DrawRectangle(cx- 5, cy-10, 10, 5, Color{32,  95, 14, 100});
                // Bright highlight specks
                DrawRectangle(cx-22, cy-2, 3, 2, Color{140, 240,  80, 120});
                DrawRectangle(cx+16, cy+2, 3, 2, Color{140, 240,  80, 115});
                DrawRectangle(cx- 4, cy+8, 3, 2, Color{140, 240,  80, 110});
                DrawRectangle(cx+  0, cy-8, 3, 2, Color{140, 240,  80, 100});
                // Wildflower cluster 1 (white/yellow — right side)
                DrawCircle(cx+14, cy-5, 2, Color{255, 255, 255, 210});
                DrawCircle(cx+17, cy-3, 2, Color{255, 255, 255, 210});
                DrawCircle(cx+11, cy-3, 2, Color{255, 255, 255, 210});
                DrawCircle(cx+14, cy-1, 2, Color{255, 255, 255, 210});
                DrawCircle(cx+14, cy-3, 2, Color{245, 210,  48, 240});
                // Wildflower cluster 2 (pink/lavender — left side)
                DrawCircle(cx-16, cy+2, 2, Color{240, 170, 220, 190});
                DrawCircle(cx-13, cy+0, 2, Color{240, 170, 220, 190});
                DrawCircle(cx-16, cy-2, 2, Color{240, 170, 220, 190});
                DrawCircle(cx-19, cy+0, 2, Color{240, 170, 220, 190});
                DrawCircle(cx-16, cy+0, 2, Color{250, 210,  60, 220});
                // Extra tiny blooms near center
                DrawCircle(cx- 4, cy-6, 1, Color{255, 230, 80, 180});
                DrawCircle(cx+  3, cy+5, 1, Color{255, 230, 80, 160});
                diamondOutline(px, py, Color{0, 0, 0, 45});
            } else {
                // MEDIUM — original default
                drawDiamond(px, py, Color{68, 152, 40, 255}, Color{86, 175, 52, 255});
                DrawRectangle(cx-18, cy-7,  13,  7, Color{48, 112, 24, 110});
                DrawRectangle(cx+ 5, cy-5,  11,  6, Color{48, 112, 24,  95});
                DrawRectangle(cx-12, cy+ 5,  9,  5, Color{48, 112, 24, 110});
                DrawRectangle(cx+ 7, cy+ 3, 10,  5, Color{48, 112, 24,  95});
                DrawRectangle(cx-22, cy-2,  3, 2, Color{118, 210, 72, 105});
                DrawRectangle(cx+16, cy+2,  3, 2, Color{118, 210, 72, 100});
                DrawRectangle(cx- 4, cy+8,  3, 2, Color{118, 210, 72,  95});
                DrawCircle(cx+14, cy-5, 2, Color{255, 255, 255, 200});
                DrawCircle(cx+17, cy-3, 2, Color{255, 255, 255, 200});
                DrawCircle(cx+11, cy-3, 2, Color{255, 255, 255, 200});
                DrawCircle(cx+14, cy-1, 2, Color{255, 255, 255, 200});
                DrawCircle(cx+14, cy-3, 2, Color{245, 210,  48, 230});
                diamondOutline(px, py, Color{0, 0, 0, 45});
            }
            break;
        }

        // ------------------------------------------------------------------ FOREST
        // LOW  = small sparse tree — single trunk + modest canopy
        // MED  = original default — overlapping cluster
        // HIGH = massive dense forest — extra large circles + dark undergrowth
        case Terrain::FOREST: {
            drawDiamond(px, py, Color{20, 55, 12, 255}, Color{28, 70, 18, 255});
            diamondOutline(px, py, Color{0, 0, 0, 55});

            const int tb = py + 4;

            if (rv == TileResourceValue::LOW) {
                // Thin trunk + small canopy only
                DrawRectangle(cx-2, tb+4, 4, 14, Color{58, 34, 10, 255});
                DrawCircle(cx,    py -  2, 12, Color{20, 72, 14, 255});
                DrawCircle(cx,    py +  2, 11, Color{28, 88, 20, 255});
                DrawCircle(cx,    py -  4,  5, Color{42, 120, 28, 255});
            } else if (rv == TileResourceValue::HIGH) {
                // Dark dense undergrowth layer first
                DrawRectangle(cx-26, cy-4, 52, 16, Color{10, 30, 6, 80});
                // Wide trunk
                DrawRectangle(cx-4, tb, 8, 22, Color{45, 26, 8, 255});
                // Huge overlapping canopy
                DrawCircle(cx,    py -  8, 22, Color{12, 60,  8, 255});
                DrawCircle(cx-18, py -  2, 17, Color{16, 70, 12, 255});
                DrawCircle(cx+18, py -  2, 17, Color{16, 70, 12, 255});
                DrawCircle(cx,    py +  2, 20, Color{22, 88, 16, 255});
                DrawCircle(cx-14, py +  6, 15, Color{28, 100, 20, 255});
                DrawCircle(cx+14, py +  6, 15, Color{28, 100, 20, 255});
                DrawCircle(cx,    py +  8, 16, Color{34, 114, 24, 255});
                DrawCircle(cx,    py - 10,  9, Color{48, 140, 32, 255});
                DrawCircle(cx- 8, py -  6,  7, Color{44, 132, 28, 255});
                DrawCircle(cx+ 8, py -  6,  7, Color{44, 132, 28, 255});
                // Bright highlight tips
                DrawCircle(cx,    py - 10,  4, Color{72, 180, 48, 200});
                DrawCircle(cx-16, py -  2,  4, Color{60, 165, 40, 180});
                DrawCircle(cx+16, py -  2,  4, Color{60, 165, 40, 180});
            } else {
                // MEDIUM — original default
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
            }
            break;
        }

        // ------------------------------------------------------------------ MOUNTAIN
        // LOW  = bare rock — no snow, darker/duller stone
        // MED  = original default — grey stone + snow cap
        // HIGH = rich ore — snow cap + amber/gold vein lines
        case Terrain::MOUNTAIN: {
            const int h = 30;

            Vector2 P = {(float)cx,              (float)(py - h)};
            Vector2 T = {(float)cx,              (float)py};
            Vector2 R = {(float)(cx+ISO_HALF_W), (float)(py+ISO_HALF_H)};
            Vector2 B = {(float)cx,              (float)(py+ISO_TILE_H)};
            Vector2 L = {(float)(cx-ISO_HALF_W), (float)(py+ISO_HALF_H)};

            if (rv == TileResourceValue::LOW) {
                // Dull, bare rock — slightly warmer brown-grey tones, no snow
                drawDiamond(px, py, Color{72, 68, 62, 255}, Color{85, 81, 75, 255});
                DrawTriangle(L, T, P, Color{85,  80,  72, 255}); // NW
                DrawTriangle(T, R, P, Color{100,  95,  88, 255}); // NE
                DrawTriangle(L, B, P, Color{85,  80,  72, 255}); // SW
                DrawTriangle(B, R, P, Color{110, 105,  98, 255}); // SE
                // Ridge seams
                DrawLineEx(P, T, 1.2f, Color{40, 36, 30, 255});
                DrawLineEx(P, L, 1.2f, Color{40, 36, 30, 255});
                DrawLineEx(P, R, 1.2f, Color{40, 36, 30, 255});
                DrawLineEx(P, B, 1.2f, Color{40, 36, 30, 255});
                // Scattered pebble/scree marks — short lines
                DrawLineEx({(float)(cx-12),(float)(cy-8)},{(float)(cx-8),(float)(cy-4)}, 1.0f, Color{58,54,48,160});
                DrawLineEx({(float)(cx+ 8),(float)(cy-6)},{(float)(cx+14),(float)(cy-2)},1.0f, Color{58,54,48,140});
                DrawLineEx({(float)(cx- 4),(float)(cy+2)},{(float)(cx+ 2),(float)(cy+6)},1.0f, Color{58,54,48,130});
            } else if (rv == TileResourceValue::HIGH) {
                // Rich ore — same silhouette + snow cap + amber vein lines
                drawDiamond(px, py, Color{60, 58, 64, 255}, Color{72, 70, 76, 255});
                DrawTriangle(L, T, P, Color{95,  93,  99, 255});
                DrawTriangle(T, R, P, Color{112, 110, 116, 255});
                DrawTriangle(L, B, P, Color{95,  93,  99, 255});
                DrawTriangle(B, R, P, Color{128, 126, 132, 255});
                DrawLineEx(P, T, 1.2f, Color{38, 36, 42, 255});
                DrawLineEx(P, L, 1.2f, Color{38, 36, 42, 255});
                DrawLineEx(P, R, 1.2f, Color{38, 36, 42, 255});
                DrawLineEx(P, B, 1.2f, Color{38, 36, 42, 255});
                // Snow cap (same as MEDIUM)
                const int sc = 8;
                Vector2 CL = {(float)(cx - sc), (float)(py - h + sc + 2)};
                Vector2 CR = {(float)(cx + sc), (float)(py - h + sc + 2)};
                DrawTriangle(CL, {(float)cx, (float)(py - h + sc + 2)}, P, Color{210, 215, 220, 255});
                DrawTriangle({(float)cx, (float)(py - h + sc + 2)}, CR, P, Color{235, 238, 242, 255});
                // Amber/gold ore veins on NE and SE faces
                const Color vein  = Color{210, 155, 28, 200};
                const Color vein2 = Color{240, 195, 50, 160};
                // NE face veins
                DrawLineEx({(float)(cx+ 6),(float)(py-14)},{(float)(cx+22),(float)(py+8)},  1.5f, vein);
                DrawLineEx({(float)(cx+14),(float)(py-10)},{(float)(cx+28),(float)(py+12)}, 1.2f, vein2);
                DrawLineEx({(float)(cx+ 4),(float)(py-20)},{(float)(cx+18),(float)(py+0)},  1.0f, vein2);
                // SE face veins (smaller)
                DrawLineEx({(float)(cx+10),(float)(py+10)},{(float)(cx+22),(float)(py+22)}, 1.2f, vein);
                DrawLineEx({(float)(cx+18),(float)(py+14)},{(float)(cx+28),(float)(py+24)}, 1.0f, vein2);
                // Ore glint specks
                DrawCircle(cx+16, py-4, 2, Color{255, 220, 80, 200});
                DrawCircle(cx+22, py+8, 2, Color{255, 220, 80, 180});
                DrawCircle(cx+20, py+18, 2, Color{255, 210, 60, 160});
            } else {
                // MEDIUM — original default
                drawDiamond(px, py, Color{60, 58, 64, 255}, Color{72, 70, 76, 255});
                DrawTriangle(L, T, P, Color{95,  93,  99,  255});
                DrawTriangle(T, R, P, Color{112, 110, 116, 255});
                DrawTriangle(L, B, P, Color{95,  93,  99,  255});
                DrawTriangle(B, R, P, Color{128, 126, 132, 255});
                DrawLineEx(P, T, 1.2f, Color{38, 36, 42, 255});
                DrawLineEx(P, L, 1.2f, Color{38, 36, 42, 255});
                DrawLineEx(P, R, 1.2f, Color{38, 36, 42, 255});
                DrawLineEx(P, B, 1.2f, Color{38, 36, 42, 255});
                const int sc = 8;
                Vector2 CL = {(float)(cx - sc), (float)(py - h + sc + 2)};
                Vector2 CR = {(float)(cx + sc), (float)(py - h + sc + 2)};
                DrawTriangle(CL, {(float)cx, (float)(py - h + sc + 2)}, P, Color{210, 215, 220, 255});
                DrawTriangle({(float)cx, (float)(py - h + sc + 2)}, CR, P, Color{235, 238, 242, 255});
            }
            break;
        }

        // ------------------------------------------------------------------ OCEAN
        // LOW  = calm — minimal waves, muted highlights
        // MED  = original default — moderate waves
        // HIGH = stormy — more/brighter whitecaps, extra chop
        case Terrain::OCEAN: {
            if (rv == TileResourceValue::LOW) {
                // Calmer, slightly lighter blue
                drawDiamond(px, py, Color{14, 44, 140, 255}, Color{20, 56, 168, 255});
                // Gentle depth shadow
                DrawTriangle(
                    {(float)(cx-20), (float)(cy-2)},
                    {(float)(cx+20), (float)(cy-2)},
                    {(float)cx,      (float)(py+ISO_TILE_H)},
                    Color{8, 22, 88, 35});
                // Just one subtle wave line
                DrawLineEx({(float)(cx-18),(float)(cy-8)},{(float)(cx-8),(float)(cy-11)},1.2f,Color{88,168,255, 70});
                DrawLineEx({(float)(cx-8), (float)(cy-11)},{(float)cx,   (float)(cy- 9)},1.2f,Color{88,168,255, 70});
                DrawLineEx({(float)cx,     (float)(cy- 9)},{(float)(cx+12),(float)(cy-12)},1.2f,Color{88,168,255,70});
                DrawCircle(cx-4, cy-10, 1, Color{155,210,255, 50});
                diamondOutline(px, py, Color{0, 0, 0, 45});
            } else if (rv == TileResourceValue::HIGH) {
                // Stormy deep ocean
                drawDiamond(px, py, Color{8, 28, 110, 255}, Color{12, 38, 138, 255});
                // Darker deep-water shadow
                DrawTriangle(
                    {(float)(cx-30), (float)(cy-6)},
                    {(float)(cx+30), (float)(cy-6)},
                    {(float)cx,      (float)(py+ISO_TILE_H)},
                    Color{4, 14, 65, 75});
                // Three full wave lines (brighter)
                DrawLineEx({(float)(cx-22),(float)(cy- 8)},{(float)(cx-10),(float)(cy-12)},2.0f,Color{100,185,255,150});
                DrawLineEx({(float)(cx-10),(float)(cy-12)},{(float)cx,     (float)(cy-10)},2.0f,Color{100,185,255,150});
                DrawLineEx({(float)cx,    (float)(cy-10)},{(float)(cx+14),(float)(cy-14)}, 2.0f,Color{100,185,255,150});
                DrawLineEx({(float)(cx+4),(float)(cy+ 2)},{(float)(cx+18),(float)(cy- 2)}, 1.8f,Color{100,185,255,110});
                DrawLineEx({(float)(cx-18),(float)(cy+3)},{(float)(cx- 6),(float)(cy+  0)},1.8f,Color{100,185,255,110});
                // Extra chop lines
                DrawLineEx({(float)(cx-14),(float)(cy+ 0)},{(float)(cx-4),(float)(cy- 4)}, 1.3f,Color{130,210,255, 90});
                DrawLineEx({(float)(cx+ 6),(float)(cy- 4)},{(float)(cx+20),(float)(cy- 8)},1.3f,Color{130,210,255, 90});
                // Whitecap highlights
                DrawCircle(cx-6,  cy-10, 3, Color{200,235,255,110});
                DrawCircle(cx+10, cy-12, 3, Color{200,235,255,110});
                DrawCircle(cx+16, cy-4,  2, Color{200,235,255, 90});
                DrawCircle(cx-14, cy+2,  2, Color{200,235,255, 80});
                diamondOutline(px, py, Color{0, 0, 0, 60});
            } else {
                // MEDIUM — original default
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
            }
            break;
        }

        // ------------------------------------------------------------------ RIVER
        // LOW  = slow/shallow — fewer chevrons, lighter blue, calmer
        // MED  = original default — 3 chevron pairs
        // HIGH = rapid/deep — many bright chevrons, intense blue, extra streaks
        case Terrain::RIVER: {
            if (rv == TileResourceValue::LOW) {
                // Shallow, pale blue
                drawDiamond(px, py, Color{95, 185, 225, 255}, Color{115, 208, 242, 255});
                // Very subtle depth gradient
                DrawTriangle(
                    {(float)(cx-14), (float)(cy+6)},
                    {(float)(cx+14), (float)(cy+6)},
                    {(float)cx,      (float)(py+ISO_TILE_H)},
                    Color{60, 145, 190, 30});
                const float cw = 1.4f;
                const Color curr = Color{210, 248, 255, 140};
                // Just one chevron pair
                DrawLineEx({(float)(cx-14),(float)(cy+ 0)},{(float)cx,      (float)(cy+ 6)},cw,curr);
                DrawLineEx({(float)cx,     (float)(cy+ 6)},{(float)(cx+14),(float)(cy+ 0)}, cw,curr);
                diamondOutline(px, py, Color{50, 145, 185, 60});
            } else if (rv == TileResourceValue::HIGH) {
                // Rapid, deep blue
                drawDiamond(px, py, Color{45, 140, 195, 255}, Color{62, 168, 220, 255});
                // Stronger depth gradient
                DrawTriangle(
                    {(float)(cx-24), (float)(cy+2)},
                    {(float)(cx+24), (float)(cy+2)},
                    {(float)cx,      (float)(py+ISO_TILE_H)},
                    Color{28, 95, 155, 90});
                const float cw = 2.2f;
                const Color curr  = Color{215, 248, 255, 220};
                const Color currD = Color{38, 118, 172, 160};
                // Four chevron pairs — dense and bright
                DrawLineEx({(float)(cx-14),(float)(cy-16)},{(float)cx,      (float)(cy-10)},cw,curr);
                DrawLineEx({(float)cx,     (float)(cy-10)},{(float)(cx+14),(float)(cy-16)}, cw,curr);
                DrawLineEx({(float)(cx-14),(float)(cy- 6)},{(float)cx,      (float)(cy+ 0)},cw,curr);
                DrawLineEx({(float)cx,     (float)(cy+ 0)},{(float)(cx+14),(float)(cy- 6)}, cw,curr);
                DrawLineEx({(float)(cx-14),(float)(cy+ 4)},{(float)cx,      (float)(cy+10)},cw,curr);
                DrawLineEx({(float)cx,     (float)(cy+10)},{(float)(cx+14),(float)(cy+ 4)}, cw,curr);
                DrawLineEx({(float)(cx-10),(float)(cy+12)},{(float)cx,      (float)(cy+18)},1.6f,curr);
                DrawLineEx({(float)cx,     (float)(cy+18)},{(float)(cx+10),(float)(cy+12)}, 1.6f,curr);
                // Heavy undercurrent streaks
                DrawLineEx({(float)(cx- 8),(float)(cy-12)},{(float)(cx-18),(float)(cy- 4)},1.6f,currD);
                DrawLineEx({(float)(cx+ 8),(float)(cy+ 2)},{(float)(cx+18),(float)(cy- 4)},1.6f,currD);
                DrawLineEx({(float)(cx-10),(float)(cy+ 6)},{(float)(cx-20),(float)(cy+12)},1.4f,currD);
                DrawLineEx({(float)(cx+10),(float)(cy- 2)},{(float)(cx+20),(float)(cy+ 4)},1.4f,currD);
                // Foam/spray specks
                DrawCircle(cx,    cy-12, 2, Color{230, 248, 255, 180});
                DrawCircle(cx-12, cy- 4, 2, Color{230, 248, 255, 160});
                DrawCircle(cx+12, cy+ 0, 2, Color{230, 248, 255, 150});
                diamondOutline(px, py, Color{30, 110, 165, 100});
            } else {
                // MEDIUM — original default
                drawDiamond(px, py, Color{72, 168, 212, 255}, Color{92, 192, 232, 255});
                DrawTriangle(
                    {(float)(cx-20), (float)(cy+4)},
                    {(float)(cx+20), (float)(cy+4)},
                    {(float)cx,      (float)(py+ISO_TILE_H)},
                    Color{50, 130, 180, 60});
                const float cw = 1.8f;
                const Color curr  = Color{195, 238, 255, 180};
                const Color currD = Color{55, 140, 190, 120};
                DrawLineEx({(float)(cx-14),(float)(cy-14)},{(float)cx,      (float)(cy- 8)},cw,curr);
                DrawLineEx({(float)cx,     (float)(cy- 8)},{(float)(cx+14),(float)(cy-14)}, cw,curr);
                DrawLineEx({(float)(cx-14),(float)(cy+ 0)},{(float)cx,      (float)(cy+ 6)},cw,curr);
                DrawLineEx({(float)cx,     (float)(cy+ 6)},{(float)(cx+14),(float)(cy+ 0)}, cw,curr);
                DrawLineEx({(float)(cx-10),(float)(cy+10)},{(float)cx,      (float)(cy+16)},cw,curr);
                DrawLineEx({(float)cx,     (float)(cy+16)},{(float)(cx+10),(float)(cy+10)}, cw,curr);
                DrawLineEx({(float)(cx-8),(float)(cy-10)},{(float)(cx-16),(float)(cy- 4)},1.2f,currD);
                DrawLineEx({(float)(cx+8),(float)(cy+ 4)},{(float)(cx+16),(float)(cy-  2)},1.2f,currD);
                diamondOutline(px, py, Color{40, 130, 180, 80});
            }
            break;
        }
    }
}

} // namespace Sprites
// city() is in city_sprite.cpp
