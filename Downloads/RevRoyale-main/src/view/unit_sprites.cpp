#include "view/sprites.h"
#include "view/layout.h"
#include "raylib.h"

using namespace Layout;

namespace Sprites {

// ---- Unit sprite drawing ---------------------------------------------------
// cy = visual torso centre. Sprites must stay within py+0 to py+54
// (health bar occupies py+55 to py+63).

void unit(UnitType type, int px, int py, Color tint) {
    const int cx = px;                    // iso: px is already the horizontal centre
    const int cy = py + ISO_HALF_H - 4;   // iso: slightly above the tile equator

    const Color dk   = darken(tint, 0.50f);
    const Color dk2  = darken(tint, 0.35f);
    const unsigned char ma = tint.a;
    const Color steel = Color{205, 210, 222, ma};
    const Color gold  = Color{212, 170,  55, ma};

    switch (type) {

        case UnitType::WARRIOR: {
            DrawRectangleRounded({(float)(cx-27), (float)(cy-13), 14.f, 28.f}, 0.3f, 8, dk);
            DrawLine(cx-20, cy-13, cx-20, cy+15, Color{255,255,255,35});
            DrawRectangle(cx-25, cy-2, 10, 3, darken(tint, 0.40f));
            DrawRectangle(cx-10, cy-11, 22, 23, tint);
            DrawRectangle(cx-10, cy-11, 22,  5, Color{255,255,255,28});
            DrawLine(cx+1, cy-11, cx+1, cy+12, Color{0,0,0,22});
            DrawRectangle(cx-14, cy-11, 5, 9, dk);
            DrawRectangle(cx+11, cy-11, 5, 9, dk);
            DrawRectangle(cx-10, cy+12, 10, 14, dk);
            DrawRectangle(cx+ 1, cy+12, 10, 14, dk);
            DrawLine(cx-5, cy+12, cx-5, cy+26, Color{255,255,255,16});
            DrawLine(cx+6, cy+12, cx+6, cy+26, Color{255,255,255,16});
            DrawCircle(cx, cy-21, 10, tint);
            DrawRectangle(cx-10, cy-28, 20, 7, dk2);
            DrawRectangle(cx- 7, cy-19, 14, 3, dk2);
            DrawLine(cx, cy-21, cx, cy-15, Color{0,0,0,30});
            DrawRectangle(cx+13, cy-24, 4, 34, steel);
            DrawRectangle(cx+ 7, cy-12, 16,  4, gold);
            DrawCircle(cx+15, cy+10, 3, gold);
            break;
        }

        case UnitType::SCOUT: {
            DrawTriangle(
                {(float)(cx+ 2), (float)(cy-16)},
                {(float)(cx-16), (float)(cy+24)},
                {(float)(cx+18), (float)(cy+24)}, dk);
            DrawLineEx({(float)(cx+2),(float)(cy-16)}, {(float)(cx+18),(float)(cy+24)}, 1.5f, Color{255,255,255,22});
            DrawTriangle(
                {(float)(cx+ 2), (float)(cy-9)},
                {(float)(cx- 7), (float)(cy+24)},
                {(float)(cx+ 9), (float)(cy+24)}, dk2);
            DrawRectangle(cx-5, cy-9, 12, 20, tint);
            DrawCircle(cx,   cy-15, 8, dk);
            DrawCircle(cx+1, cy-14, 6, tint);
            DrawCircle(cx-1, cy-18, 8, dk);
            DrawLineEx({(float)(cx+6),(float)(cy-5)}, {(float)(cx+19),(float)(cy+10)}, 3.2f, steel);
            DrawTriangle(
                {(float)(cx+21),(float)(cy+12)},
                {(float)(cx+16),(float)(cy+ 8)},
                {(float)(cx+18),(float)(cy+15)}, steel);
            DrawLineEx({(float)(cx+4),(float)(cy-2)}, {(float)(cx+15),(float)(cy+11)}, 2.0f, darken(steel, 0.7f));
            break;
        }

        case UnitType::RANGER: {
            DrawRectangle(cx-6, cy+10, 7, 16, dk);
            DrawRectangle(cx+1, cy+10, 7, 16, dk);
            DrawRectangle(cx-6, cy-9, 14, 21, tint);
            DrawRectangle(cx-6, cy-9, 14,  5, Color{255,255,255,24});
            DrawCircle(cx, cy-15, 7, tint);
            DrawTriangle(
                {(float)cx,     (float)(cy-30)},
                {(float)(cx-9), (float)(cy-10)},
                {(float)(cx+9), (float)(cy-10)}, dk);
            DrawTriangle(
                {(float)cx,     (float)(cy-30)},
                {(float)(cx-4), (float)(cy-16)},
                {(float)(cx+4), (float)(cy-16)}, dk2);
            DrawRectangle(cx-20, cy-3, 15, 5, dk);
            DrawRing({(float)(cx-14), (float)cy}, 13.f, 17.f, 108.f, 252.f, 16, dk);
            DrawLineEx({(float)(cx-9),(float)(cy-13)}, {(float)(cx-9),(float)(cy+13)}, 1.5f, steel);
            DrawLineEx({(float)(cx+8),(float)cy}, {(float)(cx+24),(float)(cy-4)}, 2.0f, gold);
            DrawTriangle(
                {(float)(cx+26),(float)(cy-5)},
                {(float)(cx+21),(float)(cy-2)},
                {(float)(cx+22),(float)(cy-8)}, steel);
            DrawTriangle(
                {(float)(cx+ 8),(float)cy},
                {(float)(cx+12),(float)(cy-5)},
                {(float)(cx+11),(float)(cy+4)},
                Color{220,70,50,(unsigned char)(ma > 40 ? ma : 200)});
            break;
        }

        case UnitType::CAVALRY: {
            DrawRectangle(cx-20, cy+ 4, 36, 14, dk);
            DrawRectangle(cx+12, cy- 2, 11, 10, dk);
            DrawRectangle(cx+14, cy-15,  9, 14, dk);
            DrawRectangle(cx+14, cy-26, 13, 12, dk);
            DrawCircle(cx+27, cy-21, 2, dk2);
            DrawTriangle({(float)(cx+16),(float)(cy-26)},{(float)(cx+14),(float)(cy-32)},{(float)(cx+20),(float)(cy-26)}, dk2);
            DrawRectangle(cx+15, cy-17, 4, 14, dk2);
            for (int i = 0; i < 4; i++) {
                DrawRectangle(cx-16+i*11, cy+18, 5, 10, dk);
                DrawRectangle(cx-16+i*11, cy+25, 5,  4, darken(dk2, 0.7f));
            }
            DrawLineEx({(float)(cx-20),(float)(cy+8)}, {(float)(cx-28),(float)(cy+22)}, 3.f, dk2);
            DrawRectangle(cx-2, cy-19, 15, 25, tint);
            DrawRectangle(cx-2, cy-19, 15,  5, Color{255,255,255,28});
            DrawCircle(cx+6, cy-25, 8, tint);
            DrawRectangle(cx-1, cy-30, 16, 6, dk2);
            DrawRectangle(cx+2, cy-24, 10, 3, dk2);
            DrawLineEx({(float)(cx+4),(float)(cy-9)}, {(float)(cx+30),(float)(cy-22)}, 3.0f, gold);
            DrawTriangle(
                {(float)(cx+32),(float)(cy-23)},
                {(float)(cx+26),(float)(cy-18)},
                {(float)(cx+27),(float)(cy-28)}, steel);
            break;
        }

        case UnitType::MAGE: {
            DrawTriangle(
                {(float)cx,      (float)(cy-10)},
                {(float)(cx-22), (float)(cy+25)},
                {(float)(cx+22), (float)(cy+25)}, tint);
            DrawTriangle(
                {(float)cx,     (float)(cy-6)},
                {(float)(cx-7), (float)(cy+25)},
                {(float)(cx+7), (float)(cy+25)},
                Color{255,255,255,18});
            DrawRectangle(cx-22, cy+21, 44, 5, darken(tint, 0.60f));
            DrawRectangle(cx-7, cy-10, 15, 14, tint);
            DrawRectangle(cx-7, cy-10, 15,  4, Color{255,255,255,28});
            DrawCircle(cx, cy-18, 8, tint);
            DrawRectangle(cx-12, cy-22, 24, 5, dk);
            DrawLine(cx-12, cy-22, cx+12, cy-22, Color{255,255,255,18});
            DrawTriangle(
                {(float)cx,      (float)(cy-28)},
                {(float)(cx-11), (float)(cy-20)},
                {(float)(cx+11), (float)(cy-20)}, dk);
            DrawTriangle(
                {(float)cx,     (float)(cy-28)},
                {(float)(cx-3), (float)(cy-24)},
                {(float)(cx+3), (float)(cy-24)},
                Color{255,255,255,18});
            DrawRectangle(cx-9, cy-24, 18, 3, Color{212,170,55,200});
            DrawLineEx({(float)(cx-12),(float)(cy+23)}, {(float)(cx-12),(float)(cy-12)}, 3.0f, gold);
            DrawCircle(cx-12, cy-17, 8, dk2);
            DrawCircle(cx-12, cy-17, 6, Color{168, 135, 255, ma});
            DrawCircle(cx-12, cy-19, 3, Color{218, 205, 255, (unsigned char)(ma > 40 ? 210 : ma)});
            DrawCircle(cx-22, cy-14, 2, Color{195, 150, 255, 155});
            DrawCircle(cx-18, cy-26, 2, Color{178, 135, 255, 130});
            DrawCircle(cx- 4, cy-26, 2, Color{205, 175, 255, 110});
            DrawCircle(cx-24, cy-22, 1, Color{210, 190, 255, 125});
            break;
        }
    }
}

// ---- Mode icon drawing -----------------------------------------------------

void modeIcon(ControllerMode mode, int ix, int iy, int sz, bool active) {
    int cx = ix + sz / 2;
    int cy = iy + sz / 2;
    Color ico = active ? WHITE : Color{175, 178, 200, 255};
    Color acc = active ? Color{212, 170, 55, 255} : Color{140, 115, 42, 200};
    Color bg  = active ? Color{35, 130, 55, 255} : Color{35, 35, 52, 255};

    DrawRectangle(ix, iy, sz, sz, bg);
    DrawRectangleLines(ix, iy, sz, sz, active ? Color{80, 210, 100, 255} : Color{70, 72, 95, 255});

    switch (mode) {
        case ControllerMode::TACTIC: {
            float p = sz * 0.17f;
            DrawLineEx({ix+p,    iy+p},    {ix+sz-p, iy+sz-p}, 3.0f, ico);
            DrawLineEx({ix+sz-p, iy+p},    {ix+p,    iy+sz-p}, 3.0f, ico);
            DrawLineEx({(float)cx-5, (float)cy}, {(float)cx+5, (float)cy}, 3.0f, acc);
            DrawLineEx({(float)cx, (float)cy-5}, {(float)cx, (float)cy+5}, 3.0f, acc);
            DrawCircle((int)(ix+p),    (int)(iy+p),    2, acc);
            DrawCircle((int)(ix+sz-p), (int)(iy+sz-p), 2, acc);
            break;
        }
        case ControllerMode::TRAINING: {
            DrawCircleLines(cx, cy, sz/2 - 4, Color{ico.r,ico.g,ico.b,(unsigned char)(ico.a/2)});
            DrawCircleLines(cx, cy, sz/3 - 1, ico);
            DrawCircle(cx, cy, sz / 6, ico);
            DrawCircle(cx, cy, 3,      Color{210, 50, 50, (unsigned char)(active ? 255 : 200)});
            DrawLine(cx, iy+3,    cx, iy+8,    ico);
            DrawLine(cx, iy+sz-8, cx, iy+sz-3, ico);
            DrawLine(ix+3,    cy, ix+8,    cy, ico);
            DrawLine(ix+sz-8, cy, ix+sz-3, cy, ico);
            break;
        }
        case ControllerMode::BUILDING: {
            int tw = sz / 2 + 2;
            int tx = cx - tw / 2;
            int ty = iy + 8;
            int th = sz - 14;
            DrawRectangle(tx,            ty + 7, tw,       th - 7, ico);
            DrawRectangle(tx + 1,        ty + 1, tw/3 - 2, 8,      ico);
            DrawRectangle(tx + tw/3 + 1, ty + 1, tw/3 - 2, 8,      ico);
            DrawRectangle(tx+tw*2/3 + 1, ty + 1, tw/3 - 2, 8,      ico);
            DrawRectangle(tx + tw/2 - 3, ty + th - 8, 7, 8, bg);
            DrawCircle(tx + tw/2, ty + th - 8, 3, bg);
            DrawLine(tx + tw/2, ty + 1, tx + tw/2, ty - 4, acc);
            DrawTriangle({(float)(tx+tw/2),(float)(ty-4)},{(float)(tx+tw/2),(float)(ty+1)},{(float)(tx+tw/2+5),(float)(ty-2)}, acc);
            break;
        }
        case ControllerMode::PRAY: {
            // Hovering orb with diamond outline — spirit energy
            float r = sz * 0.28f;
            Color orbCol = active ? Color{190, 150, 255, 230} : Color{130, 100, 180, 180};
            Color orbHi  = active ? Color{230, 205, 255, 255} : Color{170, 145, 210, 200};
            // Soft outer ring
            DrawCircleLines(cx, cy, (int)(r + 2), Color{orbCol.r, orbCol.g, orbCol.b, 80});
            // Solid orb
            DrawCircle(cx, cy, (int)r, orbCol);
            DrawCircle(cx - 2, cy - 2, (int)(r * 0.4f), orbHi);
            // Diamond radiants
            DrawLineEx({(float)cx, (float)(cy - (int)(r*1.5f))},
                        {(float)cx, (float)(cy - (int)r)}, 1.5f, acc);
            DrawLineEx({(float)cx, (float)(cy + (int)(r*1.5f))},
                        {(float)cx, (float)(cy + (int)r)}, 1.5f, acc);
            DrawLineEx({(float)(cx - (int)(r*1.5f)), (float)cy},
                        {(float)(cx - (int)r), (float)cy}, 1.5f, acc);
            DrawLineEx({(float)(cx + (int)(r*1.5f)), (float)cy},
                        {(float)(cx + (int)r), (float)cy}, 1.5f, acc);
            break;
        }
    }
}

} // namespace Sprites
