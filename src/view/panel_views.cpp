#include "view/panel_views.h"
#include "view/sprites.h"
#include <cmath>
#include "view/layout.h"
#include "model/world.h"
#include "model/tile.h"
#include "model/unit.h"
#include "raylib.h"

using namespace Layout;

// ---------------------------------------------------------------------------
// Local helpers (used only in this translation unit)
// ---------------------------------------------------------------------------

static const char* terrainName(Terrain t) {
    switch (t) {
        case Terrain::GRASS:    return "Grassland";
        case Terrain::FOREST:   return "Forest";
        case Terrain::MOUNTAIN: return "Mountain";
        case Terrain::OCEAN:    return "Ocean";
        case Terrain::RIVER:    return "River";
        default: return "Unknown";
    }
}

static const char* terrainCost(Terrain t) {
    switch (t) {
        case Terrain::GRASS:    return "1.0x";
        case Terrain::FOREST:   return "1.5x";
        case Terrain::MOUNTAIN: return "2.0x";
        case Terrain::OCEAN:    return "impassable";
        case Terrain::RIVER:    return "2.5x";
        default: return "?";
    }
}

static const char* terrainDesc(Terrain t) {
    switch (t) {
        case Terrain::GRASS:    return "Open ground. Easy to traverse.";
        case Terrain::FOREST:   return "Dense trees. Slows movement.";
        case Terrain::MOUNTAIN: return "Rugged peaks. Hard to traverse.";
        case Terrain::OCEAN:    return "Deep water. Impassable on foot.";
        case Terrain::RIVER:    return "Swift current. Costly to cross.";
        default: return "";
    }
}

static const char* unitName(UnitType type) {
    switch (type) {
        case UnitType::WARRIOR: return "Warrior";
        case UnitType::RANGER:  return "Ranger";
        case UnitType::SCOUT:   return "Scout";
        case UnitType::CAVALRY: return "Cavalry";
        case UnitType::MAGE:    return "Mage";
        default: return "Unknown";
    }
}

static const char* unitSymbol(UnitType type) {
    switch (type) {
        case UnitType::WARRIOR: return "W";
        case UnitType::RANGER:  return "R";
        case UnitType::SCOUT:   return "S";
        case UnitType::CAVALRY: return "C";
        case UnitType::MAGE:    return "M";
        default: return "?";
    }
}

// ---------------------------------------------------------------------------
// ErrorView
// ---------------------------------------------------------------------------

ErrorView::ErrorView() : currentError(std::nullopt) {}

void ErrorView::setError(PlayerError error) { currentError = error; }
void ErrorView::clearError()                { currentError = std::nullopt; }

void ErrorView::render(int x, int y) const {
    if (!currentError) return;
    DrawRectangle(x-6, y-4, 214, 36, Color{80, 10, 10, 200});
    DrawRectangleLines(x-6, y-4, 214, 36, Color{200, 60, 60, 255});
    DrawText(playerErrorToString(*currentError).c_str(), x, y+6, 15, Color{255, 160, 160, 255});
}

// ---------------------------------------------------------------------------
// InformationView
// ---------------------------------------------------------------------------

InformationView::InformationView() {}

void InformationView::render(const World& world,
                             const Position* hoverPos,
                             const Position* selectedPos) const {
    // Right panel: spans x = [GRID_RIGHT+4, 1916], full screen height
    const int PX  = GRID_RIGHT + 4;
    const int PW  = 1916 - PX;
    const int PAD = 14;
    const int cx  = PX + PAD;        // content left edge
    const int cw  = PW - PAD * 2;    // content width

    // Panel background + left border
    DrawRectangle(PX, 0, PW, 1080, Color{20, 20, 32, 255});
    DrawLine(PX, 0, PX, 1080, Color{55, 55, 75, 255});

    int y = 70;

    // ── Header ──────────────────────────────────────────────────────────────
    DrawText("TILE INFO", cx, y, 14, Color{140, 140, 165, 255});
    y += 20;
    DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
    y += 16;

    if (!hoverPos) {
        DrawText("Hover over a tile", cx, y, 15, GRAY);
        return;
    }

    const Tile& tile  = world.getTileAt(*hoverPos);
    const Terrain ter = tile.getTerrain();

    // ── Terrain ─────────────────────────────────────────────────────────────
    DrawText(terrainName(ter), cx, y, 20, Color{200, 220, 200, 255});
    DrawText(TextFormat("COST  %s", terrainCost(ter)),
             cx + cw - 90, y + 3, 12, Color{145, 175, 145, 255});
    y += 28;
    DrawText(terrainDesc(ter), cx, y, 13, Color{115, 130, 115, 255});
    y += 28;

    // ── Unit ────────────────────────────────────────────────────────────────
    if (tile.hasUnit()) {
        const Unit* u = world.getUnit(tile.getUnit().value());
        if (u) {
            DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
            y += 16;

            const Color pc = playerColor(u->getOwner().getId());
            const Color dimPc = {
                (unsigned char)(pc.r / 2),
                (unsigned char)(pc.g / 2),
                (unsigned char)(pc.b / 2), 255};

            // Unit name + player badge
            DrawText("UNIT", cx, y, 12, Color{130, 130, 155, 255});
            DrawRectangle(cx + cw - 66, y, 66, 20, dimPc);
            DrawText(TextFormat("Player %d", u->getOwner().getId() + 1),
                     cx + cw - 60, y + 4, 12, pc);
            y += 22;
            DrawText(unitName(u->getType()), cx, y, 22, pc);
            y += 32;

            // HP number
            const float hp    = (float)u->getHealth() / (float)u->getMaxHealth();
            const Color hpCol = hp >= 0.5f ? Color{200, 220, 200, 255}
                                           : Color{220,  60,  60, 255};
            DrawText("HP", cx, y, 13, Color{150, 150, 170, 255});
            DrawText(TextFormat("%d / %d", u->getHealth(), u->getMaxHealth()),
                     cx + 30, y, 15, hpCol);
            y += 24;

            // Stat boxes: DMG / MOV / RNG
            y += 6;
            struct StatBox { const char* label; int val; Color lc; Color vc; };
            const StatBox stats[3] = {
                {"DMG", u->getDamage(),   {200, 120, 120, 255}, {240, 160, 160, 255}},
                {"MOV", u->getMovement(), {120, 160, 220, 255}, {160, 200, 255, 255}},
                {"RNG", u->getRange(),    {180, 210, 130, 255}, {210, 240, 160, 255}},
            };
            const int sw = cw / 3;
            for (int i = 0; i < 3; ++i) {
                const int sx = cx + i * sw;
                DrawRectangle(sx, y, sw - 6, 50, Color{28, 28, 42, 255});
                DrawRectangleLines(sx, y, sw - 6, 50, Color{45, 45, 62, 255});
                DrawText(stats[i].label, sx + 8, y + 7, 13, stats[i].lc);
                DrawText(TextFormat("%d", stats[i].val), sx + 8, y + 26, 18, stats[i].vc);
            }
            y += 60;

            // Status pills: MOVE / ATTACK / overall
            auto pill = [&](const char* text, bool ok, int x) {
                const Color bg  = ok ? Color{20, 60, 30, 255} : Color{50, 20, 20, 255};
                const Color bdr = ok ? Color{60, 170, 80, 255} : Color{130, 50, 50, 255};
                const Color tc  = ok ? Color{90, 220, 110, 255} : Color{170, 70, 70, 255};
                DrawRectangle(x, y, sw - 6, 28, bg);
                DrawRectangleLines(x, y, sw - 6, 28, bdr);
                DrawText(text, x + 8, y + 8, 12, tc);
            };
            pill("MOVE",   u->canMove(),        cx);
            pill("ATTACK", u->canAttack(),       cx + sw);
            pill(u->isExhausted() ? "DONE" : "READY", !u->isExhausted(), cx + sw * 2);
            y += 38;
        }
    }

    // ── City ────────────────────────────────────────────────────────────────
    if (tile.hasCity()) {
        DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
        y += 16;
        DrawText("CITY", cx, y, 12, Color{130, 130, 155, 255});
        y += 22;
        DrawText("Settlement", cx, y, 18, Color{255, 220, 80, 255});
        y += 28;
    }

    // ── Combat Forecast ─────────────────────────────────────────────────────
    // Show when a friendly unit is selected and hovering over an enemy unit.
    if (!selectedPos || !tile.hasUnit()) return;
    if (!world.hasUnitAt(*selectedPos))  return;

    const Unit* sel = world.getUnitAt(*selectedPos);
    const Unit* def = world.getUnit(tile.getUnit().value());
    if (!sel || !def || sel->sameOwner(*def)) return;

    const World::CombatForecast fc = world.getCombatForecast(*selectedPos, *hoverPos);

    DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
    y += 14;

    // Header
    const Color hdCol = fc.inRange && fc.attackerCanAct
                      ? Color{230, 180,  60, 255}
                      : Color{120, 120, 145, 255};
    DrawText("COMBAT FORECAST", cx, y, 13, hdCol);
    y += 22;

    if (!fc.attackerCanAct) {
        DrawText("Attacker has already acted", cx, y, 13, Color{130, 100, 100, 255});
        return;
    }
    if (!fc.inRange) {
        DrawText("Out of attack range", cx, y, 13, Color{130, 100, 100, 255});
        return;
    }

    // Helper: draw one row of the forecast (unit name, damage badge, HP result)
    auto drawForecastRow = [&](const char* label, Color labelCol,
                               int dmg, int hpBefore, int hpAfter, int maxHp,
                               bool dies) {
        // Label (unit name)
        DrawText(label, cx, y, 14, labelCol);

        // Damage badge (right-aligned)
        const char* dmgStr = TextFormat("-%d HP", dmg);
        const int   dmgW   = MeasureText(dmgStr, 13);
        DrawRectangle(cx + cw - dmgW - 14, y - 1, dmgW + 14, 20, Color{130, 35, 35, 200});
        DrawRectangleLines(cx + cw - dmgW - 14, y - 1, dmgW + 14, 20, Color{210, 75, 75, 255});
        DrawText(dmgStr, cx + cw - dmgW - 7, y + 2, 13, Color{255, 150, 150, 255});
        y += 22;

        // HP result line
        const Color hpCol = dies ? Color{220, 60, 60, 255}
                          : hpAfter >= hpBefore / 2 ? Color{180, 215, 180, 255}
                          :                           Color{230, 155, 60, 255};
        DrawText(TextFormat("HP  %d → %d", hpBefore, hpAfter), cx + 8, y, 13, hpCol);
        if (dies) {
            const char* tag = "DESTROYED";
            DrawText(tag, cx + cw - MeasureText(tag, 12) - 2, y + 1, 12, Color{220, 80, 80, 255});
        }
        y += 20;
    };

    // ── Attack row ──────────────────────────────────────────────────────────
    const Color atkCol = playerColor(sel->getOwner().getId());
    DrawText("ATTACK", cx, y, 11, Color{130, 130, 155, 255});
    y += 16;
    drawForecastRow(unitName(sel->getType()), atkCol,
                    fc.damage,
                    fc.defenderHpBefore, fc.defenderHpAfter, def->getMaxHealth(),
                    fc.lethal);

    // ── Retaliation row (only if defender survives and is in range) ─────────
    if (!fc.lethal && fc.retaliation > 0) {
        y += 4;
        DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 160});
        y += 10;
        DrawText("RETALIATION", cx, y, 11, Color{130, 130, 155, 255});
        y += 16;

        const Color defCol = playerColor(def->getOwner().getId());
        drawForecastRow(unitName(def->getType()), defCol,
                        fc.retaliation,
                        fc.attackerHpBefore, fc.attackerHpAfter, sel->getMaxHealth(),
                        fc.attackerDies);
    }
}

// ---------------------------------------------------------------------------
// ActionView
// ---------------------------------------------------------------------------

ActionView::ActionView() {}

void ActionView::render(const std::vector<std::string>& labels,
                        const std::vector<Rect>& buttonSlots,
                        int pendingIndex,
                        const std::vector<bool>& enabled) const
{
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 5.0f);

    for (int i = 0; i < (int)labels.size() && i < (int)buttonSlots.size(); ++i) {
        const Rect& r  = buttonSlots[i];
        bool active    = (i == pendingIndex);
        bool isEnabled = (i >= (int)enabled.size()) || enabled[i]; // default true if no mask

        if (!isEnabled) {
            DrawRectangle(r.x, r.y, r.w, r.h, Color{28, 28, 38, 255});
            DrawRectangleLines(r.x, r.y, r.w, r.h, Color{55, 55, 68, 255});
            DrawText(labels[i].c_str(), r.x+10, r.y+11, 16, Color{90, 90, 105, 255});
            continue;
        }

        Color bg  = active ? Color{35, 150, 55, 255} : Color{45, 45, 65, 255};
        Color bdr = active ? Color{80, 220, 100, 255} : Color{90, 90, 110, 255};
        DrawRectangle(r.x, r.y, r.w, r.h, bg);
        DrawRectangleLines(r.x, r.y, r.w, r.h, bdr);

        if (active) {
            unsigned char g1 = (unsigned char)(90 + 90 * pulse);
            unsigned char g2 = (unsigned char)(40 + 40 * pulse);
            DrawRectangleLines(r.x-1, r.y-1, r.w+2, r.h+2, Color{100, 255, 120, g1});
            DrawRectangleLines(r.x-2, r.y-2, r.w+4, r.h+4, Color{100, 255, 120, g2});
            DrawTriangle(
                {(float)(r.x - 12), (float)(r.y + r.h/2 - 5)},
                {(float)(r.x - 12), (float)(r.y + r.h/2 + 5)},
                {(float)(r.x -  4), (float)(r.y + r.h/2)},
                Color{120, 255, 140, 255});
        }

        DrawText(labels[i].c_str(), r.x+10, r.y+11, 16, WHITE);
    }
}
