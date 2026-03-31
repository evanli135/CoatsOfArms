#pragma once
#include <optional>
#include "raylib.h"
#include "model/world.h"
#include "model/tile.h"
#include "model/unit.h"
#include "model/util.h"
#include "view/layout.h"

// ---------------------------------------------------------------------------
// GridView — scrollable tile-grid renderer.
//
// Rendering pipeline per cell (bottom → top):
//   Terrain → City → Unit → Hover highlight → Selection highlight
// ---------------------------------------------------------------------------
class GridView {
public:
    GridView();
    ~GridView();

    /** Render the full grid for one frame. */
    void render(const World& world, const Position* hoverPos, const Position* selectedPos);

    /** Pan the camera by (dpx, dpy) pixels, clamped to map bounds. */
    void scrollBy(int dpx, int dpy);

    /** Reset camera to top-left. */
    void resetScroll();

    /** Current pixel scroll offset. */
    std::pair<int, int> getScrollOffset() const { return {scrollOffsetX, scrollOffsetY}; }

    // Optional sprite overrides — if set, used instead of procedural drawing.
    void loadTerrainSprite(Terrain t, const char* path);
    void loadUnitSprite(UnitType t, const char* path);

    // Scroll-arrow button slots (positioned at construction; wired to input later).
    Rect arrowUp, arrowDown, arrowLeft, arrowRight;

private:
    int scrollOffsetX = 0;
    int scrollOffsetY = 0;

    std::optional<Texture2D> terrainSprites[5];  // indexed by (int)Terrain
    std::optional<Texture2D> unitSprites[5];     // indexed by (int)UnitType

    void renderCell(const World& world, const Position& pos, bool isHovered, bool isSelected);
    void renderTerrainLayer(const Tile& tile, int px, int py);
    void renderCityLayer(const Tile& tile, int px, int py);
    void renderUnitLayer(const World& world, const Tile& tile, int px, int py);
    void renderHoverLayer(int px, int py);
    void renderSelectionLayer(int px, int py);
};
