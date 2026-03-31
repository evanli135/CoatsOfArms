#pragma once
#include "model/util.h"
#include "raylib.h"

// ---------------------------------------------------------------------------
// Layout constants — 1920×1080, fake-isometric 12×12 grid
//
// Each tile is a diamond (rhombus) 96×48 px.
// Tile (row, col) top vertex in screen space:
//   px = GRID_ORIG_X + (col - row) * ISO_HALF_W - scrollX
//   py = GRID_ORIG_Y + (col + row) * ISO_HALF_H - scrollY
//
// Bounding box of the 12×12 grid  (no scroll, scrollX=scrollY=0):
//   left  = 960 − 11×48 − 48 = 384     right = 960 + 11×48 + 48 = 1536
//   top   = 60                          bottom = 60 + 22×24 + 48  = 636
// ---------------------------------------------------------------------------
namespace Layout {
    inline constexpr int ISO_TILE_W  = 96;
    inline constexpr int ISO_TILE_H  = 48;
    inline constexpr int ISO_HALF_W  = ISO_TILE_W / 2;   // 48
    inline constexpr int ISO_HALF_H  = ISO_TILE_H / 2;   // 24

    // Top vertex of tile (0,0) — GRID_ORIG_Y is tuned so the grid sits
    // near the vertical centre of the 1080-px screen (grid height = 576 px,
    // so perfect centre would be (1080-576)/2 = 252; we use 220 to leave
    // room for the title bar above and info panel below).
    inline constexpr int GRID_ORIG_X = 960;
    inline constexpr int GRID_ORIG_Y = 220;

    // Grid pixel bounding box
    inline constexpr int GRID_LEFT   = GRID_ORIG_X - (Game::HEIGHT - 1) * ISO_HALF_W - ISO_HALF_W;
    inline constexpr int GRID_RIGHT  = GRID_ORIG_X + (Game::WIDTH  - 1) * ISO_HALF_W + ISO_HALF_W;
    inline constexpr int GRID_TOP    = GRID_ORIG_Y;
    inline constexpr int GRID_BOTTOM = GRID_ORIG_Y + (Game::HEIGHT + Game::WIDTH - 2) * ISO_HALF_H + ISO_TILE_H;

    // UI panels
    inline constexpr int BOTTOM_Y   = GRID_BOTTOM + 20;
    inline constexpr int ACT_X      = 30;                  // left panel  (x 0–380)
    inline constexpr int INFO_X     = 1555;                // right panel (x 1536–1920)
    inline constexpr int ERR_X      = GRID_ORIG_X - 107;  // centred above grid
    inline constexpr int ERR_Y      = GRID_ORIG_Y - 30;   // just above grid top
    inline constexpr int BTN_W      = 140;
    inline constexpr int BTN_H      = 40;
    inline constexpr int BTN_GAP    = 8;
    inline constexpr int MODE_BTN_Y = GRID_ORIG_Y;        // left panel aligns with grid top
    inline constexpr int ICON_SIZE  = 44;
    inline constexpr int ACT_BTN_Y  = MODE_BTN_Y + ICON_SIZE + BTN_GAP + 14;
}

// ---------------------------------------------------------------------------
// Rect — axis-aligned rectangle for hit-testing.
// ---------------------------------------------------------------------------
struct Rect {
    int x, y, w, h;
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

// ---------------------------------------------------------------------------
// Player colour by 0-based ID.
// ---------------------------------------------------------------------------
inline Color playerColor(int id) {
    switch (id) {
        case 0: return Color{100, 150, 255, 255};
        case 1: return Color{255,  80,  80, 255};
        case 2: return Color{ 80, 255, 100, 255};
        case 3: return Color{255, 200,  80, 255};
        default: return WHITE;
    }
}
