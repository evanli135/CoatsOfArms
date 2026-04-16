#pragma once
#include <optional>
#include <unordered_set>
#include "raylib.h"
#include "controller/action.h"
#include "model/world.h"
#include "model/tile.h"
#include "model/unit.h"
#include "model/util.h"
#include "view/layout.h"

// ---------------------------------------------------------------------------
// GridView — scrollable tile-grid renderer.
//
// Rendering pipeline per cell (bottom → top):
//   Terrain → City → Buildings → Unit → Hover highlight → Selection highlight
// ---------------------------------------------------------------------------
class GridView {
public:
    GridView();
    ~GridView();

    /** Render the full grid for one frame.
     *  visibleTiles: positions the current player can see. Empty = fog disabled.
     *  castable:     enemy positions the selected unit can target with CAST action.
     *  currentMode:  units are grayed/unselectable in TRAINING and BUILDING modes. */
    void render(const Layout::ViewLayout& layout,
                const World& world, const Position* hoverPos, const Position* selectedPos,
                const std::vector<Position>& reachable      = {},
                const std::vector<Position>& attackable     = {},
                const std::vector<Position>& lethal         = {},
                const std::vector<Position>& castable       = {},
                const std::vector<Position>& path           = {},
                const std::unordered_set<Position>& visibleTiles   = {},
                const std::unordered_set<Position>& buildableTiles = {},
                ControllerMode currentMode = ControllerMode::TACTIC);

    /** Pan the camera by (dpx, dpy) pixels, clamped to map bounds. */
    void scrollBy(int dpx, int dpy);

    /** Clamp scroll so the map stays over the play area. */
    void applyScrollBounds(int scrollMinX, int scrollMaxX, int scrollMinY, int scrollMaxY);

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
    int gridOrigX_    = 960;
    int gridOrigY_    = 220;
    int scrollMinX_   = 0;
    int scrollMaxX_   = 0;
    int scrollMinY_   = 0;
    int scrollMaxY_   = 0;

    void isoTopVertex(int row, int col, int& px, int& py) const;

    std::optional<Texture2D> terrainSprites[5];  // indexed by (int)Terrain
    std::optional<Texture2D> unitSprites[5];     // indexed by (int)UnitType

    void renderCell(const World& world, const Position& pos, bool isHovered, bool isSelected, bool isReachable, bool isAttackable, bool isLethal, bool isCastable, bool isFogged, bool isBuildable, bool unitsGrayed);
    void drawFogOverlay(int px, int py);
    void renderTerrainLayer(const Tile& tile, int px, int py);
    void renderCityBorderLayer(const World& world, const Position& pos, int px, int py);
    void renderBuildableTileLayer(int px, int py);
    void renderCityLayer(const Tile& tile, int px, int py);
    void renderBuildingLayer(const World& world, const Position& pos, int px, int py);
    void renderUnitLayer(const World& world, const Tile& tile, const Position& pos, int px, int py, bool unitsGrayed);
    void renderHoverLayer(int px, int py);
    void renderSelectionLayer(int px, int py);
    void renderReachableLayer(int px, int py, bool isHovered);
    void renderAttackableLayer(int px, int py);
    void renderCastableLayer(int px, int py);          // purple overlay for cast targets
    void renderAttackableRingsLayer(int px, int py);   // falling rings (drawn before unit)
    void renderLethalLayer(int px, int py);
    void renderAttackableHoverLayer(int px, int py);   // crossing swords (drawn after unit)
    /** Draw path arrows over all tiles in one post-pass (called after the cell loop). */
    void renderPathArrows(const std::vector<Position>& path);
};
