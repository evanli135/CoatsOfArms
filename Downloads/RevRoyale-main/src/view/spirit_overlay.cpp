#include "view/spirit_overlay.h"
#include "model/magic.h"
#include "raylib.h"
#include <cmath>
#include <string>
#include <algorithm>

// ---------------------------------------------------------------------------
// Layout constants
// ---------------------------------------------------------------------------
static constexpr int CARD_W    = 224;
static constexpr int CARD_H    = 370;
static constexpr int CARD_GAP  = 22;
static constexpr int PANEL_PAD = 42;
static constexpr int TITLE_H   = 68;   // header height above the cards

static constexpr int PANEL_W = 3 * CARD_W + 2 * CARD_GAP + 2 * PANEL_PAD;
static constexpr int PANEL_H = CARD_H + TITLE_H + PANEL_PAD;

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

SpiritOverlay::CardGeom SpiritOverlay::cardGeom(int slot, int screenW, int screenH) {
    int panelX = (screenW - PANEL_W) / 2;
    int panelY = (screenH - PANEL_H) / 2;
    int x = panelX + PANEL_PAD + slot * (CARD_W + CARD_GAP);
    int y = panelY + TITLE_H;
    return { x, y, CARD_W, CARD_H };
}

// ---------------------------------------------------------------------------
// Per-spirit colour palette
// ---------------------------------------------------------------------------

Color SpiritOverlay::spiritAccent(SpiritType s) {
    switch (s) {
        case SpiritType::FLAME:   return Color{255, 108, 25, 255};
        case SpiritType::SHADOW:  return Color{175,  85, 255, 255};
        case SpiritType::GALE:    return Color{ 55, 200, 255, 255};
        case SpiritType::MARTIAL: return Color{230, 185,  50, 255};
    }
    return Color{200, 200, 200, 255};
}

Color SpiritOverlay::spiritBase(SpiritType s) {
    switch (s) {
        case SpiritType::FLAME:   return Color{ 88, 20, 10, 235};
        case SpiritType::SHADOW:  return Color{ 20,  8, 48, 235};
        case SpiritType::GALE:    return Color{  8, 40, 70, 235};
        case SpiritType::MARTIAL: return Color{ 52, 36,  8, 235};
    }
    return Color{30, 30, 35, 235};
}

Color SpiritOverlay::spiritGlow(SpiritType s) {
    switch (s) {
        case SpiritType::FLAME:   return Color{240,  60,   0, 160};
        case SpiritType::SHADOW:  return Color{130,  45, 220, 140};
        case SpiritType::GALE:    return Color{ 35, 175, 240, 140};
        case SpiritType::MARTIAL: return Color{215, 170,  35, 150};
    }
    return Color{180, 180, 180, 140};
}

// ---------------------------------------------------------------------------
// Animated spirit effects
// (cx, cy) = centre of the 140-pixel icon zone inside the card
// ---------------------------------------------------------------------------

void SpiritOverlay::drawFlameEffect(int cx, int cy) {
    float t = (float)GetTime();

    // Glowing coal base
    DrawCircle(cx, cy + 32, 38, Color{180, 38, 0, 32});
    DrawCircle(cx, cy + 32, 22, Color{220, 75, 0, 48});

    // Eight flame tongues
    for (int i = 0; i < 8; ++i) {
        float xOff  = (i - 3.5f) * 12.0f;
        float h     = 34.0f + 18.0f * sinf(t * (4.2f + i * 0.28f) + i * 1.1f);
        float baseW = 9.5f - std::abs(xOff) * 0.1f;
        float glow  = 0.5f + 0.5f * sinf(t * 5.5f + i * 0.85f);

        unsigned char green = (unsigned char)(50 + 90 * glow);
        // Outer orange tongue
        DrawTriangle(
            {(float)cx + xOff,         (float)(cy + 30) - h},
            {(float)cx + xOff - baseW, (float)(cy + 32)},
            {(float)cx + xOff + baseW, (float)(cy + 32)},
            Color{255, green, 0, 205});
        // Inner yellow core
        DrawTriangle(
            {(float)cx + xOff,                 (float)(cy + 30) - h * 0.55f},
            {(float)cx + xOff - baseW * 0.42f, (float)(cy + 32)},
            {(float)cx + xOff + baseW * 0.42f, (float)(cy + 32)},
            Color{255, 210, 70, 235});
    }

    // Rising ember sparks
    for (int i = 0; i < 6; ++i) {
        float phase = fmodf(t * 1.35f + i * 0.38f, 1.0f);
        float sx    = (float)cx + (i - 2.5f) * 14.0f + 4.0f * sinf(t * 2.4f + i);
        float sy    = (float)(cy + 32) - phase * 60.0f;
        unsigned char sa = (unsigned char)(215.0f * (1.0f - phase));
        DrawCircle((int)sx, (int)sy, 2, Color{255, 195, 90, sa});
    }
}

void SpiritOverlay::drawShadowEffect(int cx, int cy) {
    float t = (float)GetTime();

    // Core orb
    DrawCircle(cx, cy, 24, Color{75, 28, 145, 180});
    DrawCircle(cx - 6, cy - 6, 8, Color{155, 95, 225, 115});

    // Expanding void rings
    for (int ring = 0; ring < 3; ++ring) {
        float phase = fmodf(t * 0.68f + ring * 0.34f, 1.0f);
        float r     = 14.0f + phase * 46.0f;
        unsigned char a = (unsigned char)(200.0f * (1.0f - phase));
        DrawCircleLines(cx, cy, (int)r, Color{165, 78, 255, a});
    }

    // Four orbiting motes
    for (int i = 0; i < 4; ++i) {
        float angle = t * 1.2f + i * (3.14159f * 0.5f);
        float r2    = 32.0f + 5.0f * sinf(t * 1.9f + i);
        int mx2 = cx + (int)(cosf(angle) * r2);
        int my2 = cy + (int)(sinf(angle) * r2);
        DrawCircle(mx2, my2, 4, Color{195, 115, 255, 185});
        DrawCircle(mx2, my2, 2, Color{255, 200, 255, 215});
    }

    // Upward wisps
    for (int i = 0; i < 5; ++i) {
        float phase  = fmodf(t * 0.88f + i * 0.23f, 1.0f);
        float angle2 = t * 0.6f + i * 1.26f;
        float r3     = 12.0f + phase * 30.0f;
        float wx     = (float)cx + cosf(angle2) * r3;
        float wy     = (float)cy + sinf(angle2) * r3;
        unsigned char wa = (unsigned char)(175.0f * (1.0f - phase));
        DrawCircle((int)wx, (int)wy, 3, Color{118, 48, 200, wa});
    }
}

void SpiritOverlay::drawGaleEffect(int cx, int cy) {
    float t = (float)GetTime();

    // Central wind orb
    DrawCircle(cx, cy, 18, Color{18, 138, 215, 155});
    DrawCircleLines(cx, cy, 22, Color{55, 200, 255, 95});

    // Three rotating spiral arms
    for (int arm = 0; arm < 3; ++arm) {
        float baseAngle = t * 2.6f + arm * (2.0f * 3.14159f / 3.0f);
        for (int seg = 0; seg < 8; ++seg) {
            float frac = (float)seg / 8.0f;
            float a1   = baseAngle + frac * 2.2f;
            float a2   = baseAngle + (frac + 0.125f) * 2.2f;
            float r1   = 10.0f + frac * 40.0f;
            float r2   = 10.0f + (frac + 0.125f) * 40.0f;
            unsigned char ca = (unsigned char)(225.0f * (1.0f - frac));
            Vector2 p1 = {(float)cx + cosf(a1) * r1, (float)cy + sinf(a1) * r1};
            Vector2 p2 = {(float)cx + cosf(a2) * r2, (float)cy + sinf(a2) * r2};
            DrawLineEx(p1, p2, 2.5f - frac * 1.5f, Color{55, 200, 255, ca});
        }
    }

    // Horizontal wind streaks
    for (int i = 0; i < 7; ++i) {
        float phase = fmodf(t * 1.9f + i * 0.17f, 1.0f);
        float wx    = (float)cx - 44.0f + phase * 88.0f;
        float wy    = (float)cy - 22.0f + i * 7.5f;
        unsigned char wa = (unsigned char)(175.0f * sinf(phase * 3.14159f));
        float len = 10.0f + 8.0f * sinf((float)i * 1.4f);
        DrawLineEx({wx, wy}, {wx + len, wy}, 1.5f, Color{125, 220, 255, wa});
    }
}

void SpiritOverlay::drawMartialEffect(int cx, int cy) {
    float t = (float)GetTime();

    // Shield body
    int shW = 32, shH = 40;
    DrawRectangle(cx - shW / 2, cy - shH / 2, shW, shH, Color{158, 128, 38, 185});
    DrawRectangleLines(cx - shW / 2, cy - shH / 2, shW, shH, Color{230, 190, 68, 225});
    // Shield cross
    DrawLineEx({(float)cx, (float)(cy - shH/2 + 5)},
               {(float)cx, (float)(cy + shH/2 - 5)}, 2.5f, Color{255, 220, 100, 200});
    DrawLineEx({(float)(cx - shW/2 + 5), (float)cy},
               {(float)(cx + shW/2 - 5), (float)cy}, 2.5f, Color{255, 220, 100, 200});

    // Eight spinning golden rays
    for (int i = 0; i < 8; ++i) {
        float a     = (float)i * (2.0f * 3.14159f / 8.0f) + t * 0.62f;
        float pulse = 0.5f + 0.5f * sinf(t * 2.6f + i * 0.95f);
        float rIn   = 28.0f;
        float rOut  = 40.0f + 10.0f * pulse;
        unsigned char ra = (unsigned char)(155.0f + 80.0f * pulse);
        Vector2 p1 = {(float)cx + cosf(a) * rIn,  (float)cy + sinf(a) * rIn};
        Vector2 p2 = {(float)cx + cosf(a) * rOut, (float)cy + sinf(a) * rOut};
        DrawLineEx(p1, p2, 1.5f + pulse, Color{230, 190, 58, ra});
    }

    // Pulsing corner glints
    float gp = 0.5f + 0.5f * sinf(t * 4.2f);
    unsigned char ga = (unsigned char)(210.0f * gp);
    DrawCircle(cx,              cy - shH/2 - 7,  3, Color{255, 242, 155, ga});
    DrawCircle(cx + shW/2 + 7,  cy,              2, Color{255, 242, 155, (unsigned char)(ga * 0.7f)});
    DrawCircle(cx - shW/2 - 7,  cy,              2, Color{255, 242, 155, (unsigned char)(ga * 0.7f)});
    DrawCircle(cx,              cy + shH/2 + 7,  2, Color{255, 242, 155, (unsigned char)(ga * 0.5f)});
}

// ---------------------------------------------------------------------------
// Wrapped text helper
// ---------------------------------------------------------------------------

void SpiritOverlay::drawWrapped(const char* text, int x, int y,
                                 int maxW, int fontSize, Color color) {
    std::string s(text);
    if (MeasureText(s.c_str(), fontSize) <= maxW) {
        DrawText(s.c_str(), x, y, fontSize, color);
        return;
    }
    // Find last space before overflow
    int split = (int)s.size();
    for (int i = (int)s.size() - 1; i > 0; --i) {
        if (s[i] == ' ' &&
            MeasureText(s.substr(0, i).c_str(), fontSize) <= maxW)
        {
            split = i;
            break;
        }
    }
    DrawText(s.substr(0, split).c_str(), x, y, fontSize, color);
    if (split < (int)s.size())
        DrawText(s.substr(split + 1).c_str(), x, y + fontSize + 3, fontSize, color);
}

// ---------------------------------------------------------------------------
// Card rendering
// ---------------------------------------------------------------------------

void SpiritOverlay::drawCard(int slot, const Blessing& blessing,
                              int x, int y, int w, int h, bool hovered) {
    Color accent = spiritAccent(blessing.spirit);
    Color base   = spiritBase(blessing.spirit);
    Color glow   = spiritGlow(blessing.spirit);
    float t      = (float)GetTime();
    float pulse  = 0.5f + 0.5f * sinf(t * 4.0f);

    // ── Background ────────────────────────────────────────────────────────────
    DrawRectangle(x, y, w, h, base);
    if (hovered)
        DrawRectangle(x, y, w, h, Color{255, 255, 255, 20});

    // Border (thicker + glowing on hover)
    DrawRectangleLines(x, y, w, h,
                       Color{accent.r, accent.g, accent.b,
                             (unsigned char)(hovered ? 255 : 175)});
    if (hovered) {
        unsigned char ga = (unsigned char)(75 + 65 * pulse);
        DrawRectangleLines(x - 2, y - 2, w + 4, h + 4,
                           Color{accent.r, accent.g, accent.b, ga});
        DrawRectangleLines(x - 4, y - 4, w + 8, h + 8,
                           Color{accent.r, accent.g, accent.b, (unsigned char)(ga / 2)});
    }

    // ── Magic ability badge (top-right corner) ────────────────────────────────
    if (blessing.isMagic) {
        SpellId sid = static_cast<SpellId>(static_cast<int>(blessing.effect));
        const SpellDef& sd = MagicSystem::getSpellDef(sid);

        // Badge background
        int bx = x + w - 62, by = y + 7;
        int bw = 56, bh = 30;
        DrawRectangle(bx, by, bw, bh, Color{28, 8, 58, 220});
        DrawRectangleLines(bx, by, bw, bh, Color{160, 80, 255, 210});

        // Pulsing inner glow
        unsigned char ga = (unsigned char)(35 + 30 * pulse);
        DrawRectangle(bx + 1, by + 1, bw - 2, bh - 2, Color{90, 30, 160, ga});

        // Arcane crystal (◆) drawn as two triangles
        float dcx = (float)(bx + 11), dcy = (float)(by + 15);
        // top half (CCW in screen coords: left, top, right -> reversed to top, right, left)
        DrawTriangle({dcx, dcy - 8}, {dcx + 6, dcy}, {dcx - 6, dcy},
                     Color{175, 100, 255, 240});
        // bottom half
        DrawTriangle({dcx - 6, dcy}, {dcx + 6, dcy}, {dcx, dcy + 8},
                     Color{140, 65, 225, 240});
        // Centre sparkle
        DrawCircle((int)dcx, (int)dcy, 2, Color{235, 200, 255, 255});

        // Text
        DrawText("MAGIC",                   bx + 22, by + 5,  10, Color{200, 150, 255, 255});
        DrawText(TextFormat("%d MP", sd.cost), bx + 22, by + 17, 10, Color{160, 115, 230, 215});
    }

    // ── Effect zone (top 142px) ───────────────────────────────────────────────
    // Subtle spirit-coloured tint behind the animation
    DrawRectangle(x + 1, y + 1, w - 2, 140,
                  Color{glow.r, glow.g, glow.b, 28});

    int effectCX = x + w / 2;
    int effectCY = y + 72;

    switch (blessing.spirit) {
        case SpiritType::FLAME:   drawFlameEffect  (effectCX, effectCY); break;
        case SpiritType::SHADOW:  drawShadowEffect (effectCX, effectCY); break;
        case SpiritType::GALE:    drawGaleEffect   (effectCX, effectCY); break;
        case SpiritType::MARTIAL: drawMartialEffect(effectCX, effectCY); break;
    }

    // Separator
    DrawLineEx({(float)(x + 12),     (float)(y + 144)},
               {(float)(x + w - 12), (float)(y + 144)},
               1.0f, Color{accent.r, accent.g, accent.b, 110});

    // ── Text zone ─────────────────────────────────────────────────────────────
    // Spirit name (centred, accent colour)
    const char* sName  = spiritName(blessing.spirit);
    int nameW = MeasureText(sName, 18);
    DrawText(sName, x + w / 2 - nameW / 2, y + 151, 18,
             Color{accent.r, accent.g, accent.b, 255});

    // Short divider below name
    DrawLineEx({(float)(x + w/2 - 28), (float)(y + 172)},
               {(float)(x + w/2 + 28), (float)(y + 172)},
               1.0f, Color{accent.r, accent.g, accent.b, 70});

    // Effect name (white, slightly smaller)
    const char* eName  = blessingEffectName(blessing.effect);
    int eW = MeasureText(eName, 15);
    DrawText(eName, x + w / 2 - eW / 2, y + 178, 15,
             Color{230, 230, 230, 255});

    // Target unit type (muted, small)
    const char* uLabel = TextFormat("for %s", unitTypeName(blessing.targetUnit));
    int uW = MeasureText(uLabel, 12);
    DrawText(uLabel, x + w / 2 - uW / 2, y + 200, 12,
             Color{175, 178, 195, 205});

    // Description (word-wrapped into 2 lines if needed)
    drawWrapped(blessingDescription(blessing.effect),
                x + 12, y + 222, w - 24, 11,
                Color{155, 160, 170, 200});

    // ── Key hint (bottom) ─────────────────────────────────────────────────────
    const char* hint = TextFormat("[%d]", slot + 1);
    int hintW = MeasureText(hint, 14);
    int hbx = x + w / 2 - hintW / 2 - 7;
    int hby = y + h - 28;
    DrawRectangle(hbx, hby, hintW + 14, 20,
                  Color{accent.r, accent.g, accent.b, 38});
    DrawRectangleLines(hbx, hby, hintW + 14, 20,
                       Color{accent.r, accent.g, accent.b, 90});
    DrawText(hint, x + w/2 - hintW/2, hby + 3, 14,
             Color{accent.r, accent.g, accent.b, 210});
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void SpiritOverlay::render(const std::array<Blessing, 3>& choices,
                            int screenW, int screenH) const {
    // Full-screen dim
    DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 10, 125});

    int panelX = (screenW - PANEL_W) / 2;
    int panelY = (screenH - PANEL_H) / 2;

    // Panel backing
    DrawRectangle(panelX, panelY, PANEL_W, PANEL_H, Color{10, 12, 22, 238});
    DrawRectangleLines(panelX, panelY, PANEL_W, PANEL_H, Color{75, 80, 115, 200});
    DrawRectangleLines(panelX + 1, panelY + 1, PANEL_W - 2, PANEL_H - 2,
                       Color{50, 55, 80, 120});

    // Title
    const char* title = "SPIRIT BLESSING";
    int titleW = MeasureText(title, 23);
    DrawText(title, screenW / 2 - titleW / 2, panelY + 14, 23,
             Color{205, 210, 235, 255});

    // Subtitle
    const char* sub = "Choose one blessing for your unit";
    int subW = MeasureText(sub, 13);
    DrawText(sub, screenW / 2 - subW / 2, panelY + 42, 13,
             Color{135, 140, 162, 200});

    // Cards
    int mx = GetMouseX(), my = GetMouseY();
    for (int i = 0; i < 3; ++i) {
        auto g = cardGeom(i, screenW, screenH);
        bool hovered = (mx >= g.x && mx < g.x + g.w &&
                        my >= g.y && my < g.y + g.h);
        drawCard(i, choices[i], g.x, g.y, g.w, g.h, hovered);
    }
}

std::optional<int> SpiritOverlay::hitTest(int mx, int my,
                                           int screenW, int screenH) const {
    for (int i = 0; i < 3; ++i) {
        auto g = cardGeom(i, screenW, screenH);
        if (mx >= g.x && mx < g.x + g.w &&
            my >= g.y && my < g.y + g.h)
            return i;
    }
    return std::nullopt;
}
