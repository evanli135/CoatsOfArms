#pragma once
#include "raylib.h"
#include "model/tile.h"
#include "model/unit.h"
#include "controller/action.h"

// ---------------------------------------------------------------------------
// Sprites — stateless pixel-drawing routines.
//
// Each function issues raw Raylib draw calls for one visual element.
// No game logic or state is read; all inputs are plain value types.
// ---------------------------------------------------------------------------
namespace Sprites {

    /** Draw a 64×64 terrain tile at top-left pixel (px, py). */
    void terrain(Terrain t, int px, int py);

    /** Draw a unit silhouette at (px, py) tinted by player color.
     *  For a drop shadow, call first with a dark/translucent tint at (+1, +2). */
    void unit(UnitType type, int px, int py, Color tint);

    /** Draw a castle on a city tile, tinted by the owning player's colour.
     *  Pass GRAY (or any neutral) for an unclaimed city. */
    void city(int px, int py, Color factionColor);

    /** Draw a completed building sprite near the city at tile top-vertex (px, py).
     *  slot = 0-based index of this building in the row (for horizontal offset).
     *  total = total number of buildings being drawn this tile. */
    void building(BuildingType type, int px, int py, Color factionColor, int slot, int total);

    /** Draw a scaffold (under-construction indicator) at tile top-vertex (px, py).
     *  slot/total control horizontal offset — same convention as building(). */
    void buildingScaffold(BuildingType type, int px, int py, int turnsLeft, int slot, int total);

    /** Draw a square mode-switch icon at (ix, iy) with side length sz. */
    void modeIcon(ControllerMode mode, int ix, int iy, int sz, bool active);

    /** Darken a color by factor f  (0 = black, 1 = original). */
    inline Color darken(Color c, float f) {
        return Color{
            (unsigned char)(c.r * f),
            (unsigned char)(c.g * f),
            (unsigned char)(c.b * f),
            c.a
        };
    }

} // namespace Sprites
