#pragma once
#include "model/spirit.h"
#include "raylib.h"
#include <array>
#include <optional>

// ---------------------------------------------------------------------------
// SpiritOverlay — centered blessing-selection modal (PRAY mode, phase 2).
//
// Three spirit-themed cards appear in the middle of the screen.
// Each card is tinted and animated according to its spirit type.
// ---------------------------------------------------------------------------
class SpiritOverlay {
public:
    /** Render the overlay on top of everything.
     *  Call at the very end of a BeginDrawing/EndDrawing block. */
    void render(const std::array<Blessing, 3>& choices, int screenW, int screenH) const;

    /** Return the card index (0–2) whose bounds contain (mx, my), else nullopt.
     *  Does NOT check IsMouseButtonPressed — caller decides when to query. */
    std::optional<int> hitTest(int mx, int my, int screenW, int screenH) const;

private:
    struct CardGeom { int x, y, w, h; };

    static CardGeom cardGeom(int slot, int screenW, int screenH);

    static Color spiritAccent(SpiritType s);  // bright border / title colour
    static Color spiritBase(SpiritType s);    // dark card background
    static Color spiritGlow(SpiritType s);    // effect-zone tint

    static void drawCard(int slot, const Blessing& blessing,
                         int x, int y, int w, int h, bool hovered);

    static void drawFlameEffect  (int cx, int cy);
    static void drawShadowEffect (int cx, int cy);
    static void drawGaleEffect   (int cx, int cy);
    static void drawMartialEffect(int cx, int cy);

    /** Draw text that wraps to a second line if wider than maxW. */
    static void drawWrapped(const char* text, int x, int y,
                            int maxW, int fontSize, Color color);
};
