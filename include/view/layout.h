#pragma once
#include "model/util.h"
#include "raylib.h"
#include <algorithm>
#include <utility>

// ---------------------------------------------------------------------------
// Layout constants — fake-isometric 12×12 grid (tile geometry is fixed).
//
// Tile (row, col) top vertex in screen space:
//   px = gridOrigX + (col - row) * ISO_HALF_W - scrollX
//   py = gridOrigY + (col + row) * ISO_HALF_H - scrollY
//
// Screen positions (grid origin, panels) come from ViewLayout / makeViewLayout()
// so the game fits different window sizes and aspect ratios.
// ---------------------------------------------------------------------------
namespace Layout {
    inline constexpr int ISO_TILE_W  = 96;
    inline constexpr int ISO_TILE_H  = 48;
    inline constexpr int ISO_HALF_W  = ISO_TILE_W / 2;   // 48
    inline constexpr int ISO_HALF_H  = ISO_TILE_H / 2;   // 24

    inline constexpr int BTN_W      = 140;
    inline constexpr int BTN_H      = 40;
    inline constexpr int BTN_GAP    = 8;
    inline constexpr int ICON_SIZE  = 44;

    /** Horizontal half-span from grid origin to farthest diamond edge (12×12). */
    inline constexpr int gridHalfSpanX() {
        return (Game::HEIGHT - 1) * ISO_HALF_W + ISO_HALF_W;
    }

    /** Total vertical pixel extent of the iso map (top of (0,0) to bottom of farthest tile). */
    inline constexpr int gridPixelHeight() {
        return (Game::HEIGHT + Game::WIDTH - 2) * ISO_HALF_H + ISO_TILE_H;
    }

    inline constexpr int TITLE_TOP_MARGIN = 56;
    inline constexpr int BOTTOM_MARGIN    = 20;
    /** Space reserved left of the map for mode icons + action column (see GUI chrome). */
    inline constexpr int LEFT_UI_RESERVE  = 214;

    // -----------------------------------------------------------------------
    // Per-frame layout for the current framebuffer size.
    // -----------------------------------------------------------------------
    struct ViewLayout {
        int screenW   = 1920;
        int screenH   = 1080;
        int gridOrigX = 960;
        int gridOrigY = 220;
        int actX      = 30;
        int modeBtnY  = 220;
        int actBtnY   = 286;
        int errX      = 853;
        int errY      = 190;
        int infoPanelX = 1555;
        int infoPanelW = 365;
    };

    /** Build layout: centre the map between the left chrome and right info panel. */
    inline ViewLayout makeViewLayout(int screenW, int screenH) {
        ViewLayout L;
        L.screenW = std::max(640, screenW);
        L.screenH = std::max(480, screenH);

        int infoW = std::clamp(L.screenW / 5, 260, 380);
        int infoX = L.screenW - infoW;
        int playLeft  = LEFT_UI_RESERVE;
        int playRight = infoX;
        if (playRight - playLeft < 240) {
            infoW = std::clamp(L.screenW - playLeft - 280, 200, infoW);
            infoX = L.screenW - infoW;
            playRight = infoX;
        }
        L.infoPanelW = infoW;
        L.infoPanelX = infoX;

        L.gridOrigX = (playLeft + playRight) / 2;

        const int gh = gridPixelHeight();
        L.gridOrigY = TITLE_TOP_MARGIN
            + (L.screenH - TITLE_TOP_MARGIN - BOTTOM_MARGIN - gh) / 2;
        if (L.gridOrigY < TITLE_TOP_MARGIN)
            L.gridOrigY = TITLE_TOP_MARGIN;

        L.actX     = 30;
        L.modeBtnY = L.gridOrigY;
        L.actBtnY  = L.modeBtnY + ICON_SIZE + BTN_GAP + 14;
        L.errX     = L.gridOrigX - 107;
        L.errY     = L.gridOrigY - 30;
        return L;
    }

    /** Scroll range so the whole map can be panned within the play rectangle. */
    inline void gridScrollBounds(const ViewLayout& L,
                                 int playLeft, int playRight, int playTop, int playBottom,
                                 int& scrollMinX, int& scrollMaxX,
                                 int& scrollMinY, int& scrollMaxY) {
        const int halfW = gridHalfSpanX();
        const int gh    = gridPixelHeight();
        const int ox = L.gridOrigX;
        const int oy = L.gridOrigY;

        // Scroll must stay in [min, max] with min <= max. When the viewport is
        // narrower/shorter than the map, the raw formulas swap; collapsing to
        // a midpoint (old behaviour) locked one axis — e.g. no horizontal pan
        // on laptop widths while vertical still moved.
        scrollMinX = ox + halfW - playRight;
        scrollMaxX = ox - halfW - playLeft;
        if (scrollMinX > scrollMaxX)
            std::swap(scrollMinX, scrollMaxX);

        scrollMinY = oy + gh - playBottom;
        scrollMaxY = oy - playTop;
        if (scrollMinY > scrollMaxY)
            std::swap(scrollMinY, scrollMaxY);
    }
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
