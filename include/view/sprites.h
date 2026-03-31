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

    /** Draw a small castle icon in the top-right corner of a city tile. */
    void city(int px, int py);

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
