#include "view/info_view.h"
#include "view/layout.h"
#include <algorithm>
#include "model/tile.h"
#include "model/unit.h"
#include "model/city.h"
#include "model/world.h"
#include "raylib.h"

// ---------------------------------------------------------------------------
// String helpers
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

// ---------------------------------------------------------------------------
// Drawing primitives
// ---------------------------------------------------------------------------

static void drawDivider(int cx, int cw, int& y) {
    DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
    y += 16;
}

struct StatBox { const char* label; int val; Color lc; Color vc; };

// Primary stat row — tall boxes (50px), 13/18pt text
static void drawWideStatRow(int cx, int cw, int& y, const StatBox* stats, int n) {
    const int boxW = cw / n;
    for (int i = 0; i < n; ++i) {
        const int sx = cx + i * boxW;
        DrawRectangle(sx, y, boxW - 6, 50, Color{28, 28, 42, 255});
        DrawRectangleLines(sx, y, boxW - 6, 50, Color{45, 45, 62, 255});
        DrawText(stats[i].label, sx + 8, y + 7,  13, stats[i].lc);
        DrawText(TextFormat("%d", stats[i].val),  sx + 8, y + 26, 18, stats[i].vc);
    }
    y += 58;
}

// Bonus stat row — compact boxes (44px), 11/16pt text
static void drawCompactStatRow(int cx, int cw, int& y, const StatBox* stats, int n) {
    const int boxW = cw / n;
    for (int i = 0; i < n; ++i) {
        const int sx = cx + i * boxW;
        DrawRectangle(sx, y, boxW - 5, 44, Color{28, 28, 42, 255});
        DrawRectangleLines(sx, y, boxW - 5, 44, Color{45, 45, 62, 255});
        DrawText(stats[i].label, sx + 6, y + 5,  11, stats[i].lc);
        DrawText(TextFormat("%d", stats[i].val),  sx + 6, y + 22, 16, stats[i].vc);
    }
    y += 52;
}

static void drawHitBadge(int cx, int cw, int y, int hitPct) {
    const char* hitStr = TextFormat("%d%%", hitPct);
    const Color hitCol = hitPct >= 85 ? Color{ 90, 210,  90, 255}
                       : hitPct >= 65 ? Color{215, 195,  60, 255}
                       :                Color{210,  80,  80, 255};
    const int hw = MeasureText(hitStr, 12);
    DrawRectangle(cx + cw - hw - 16, y - 1, hw + 16, 18, Color{25, 30, 40, 210});
    DrawRectangleLines(cx + cw - hw - 16, y - 1, hw + 16, 18, hitCol);
    DrawText("HIT ", cx + cw - hw - 12, y + 2, 11, Color{140, 140, 160, 255});
    DrawText(hitStr,  cx + cw - hw -  2, y + 2, 12, hitCol);
}

static void drawForecastRow(int cx, int cw, int& y,
                            const char* label, Color labelCol,
                            int dmg, int hpBefore, int hpAfter, bool dies) {
    DrawText(label, cx, y, 14, labelCol);
    const char* dmgStr = TextFormat("-%d HP", dmg);
    const int   dmgW   = MeasureText(dmgStr, 13);
    DrawRectangle(cx + cw - dmgW - 14, y - 1, dmgW + 14, 20, Color{130, 35, 35, 200});
    DrawRectangleLines(cx + cw - dmgW - 14, y - 1, dmgW + 14, 20, Color{210, 75, 75, 255});
    DrawText(dmgStr, cx + cw - dmgW - 7, y + 2, 13, Color{255, 150, 150, 255});
    y += 22;

    const Color hpCol = dies            ? Color{220,  60,  60, 255}
                      : hpAfter >= hpBefore / 2 ? Color{180, 215, 180, 255}
                      :                           Color{230, 155,  60, 255};
    DrawText(TextFormat("HP  %d -> %d", hpBefore, hpAfter), cx + 8, y, 13, hpCol);
    if (dies) {
        const char* tag = "DESTROYED";
        DrawText(tag, cx + cw - MeasureText(tag, 12) - 2, y + 1, 12, Color{220, 80, 80, 255});
    }
    y += 20;
}

// ---------------------------------------------------------------------------
// Section renderers
// ---------------------------------------------------------------------------

static void renderTerrainSection(int cx, int cw, int& y, Terrain ter) {
    DrawText(terrainName(ter), cx, y, 20, Color{200, 220, 200, 255});
    DrawText(TextFormat("COST  %s", terrainCost(ter)),
             cx + cw - 90, y + 3, 12, Color{145, 175, 145, 255});
    y += 28;
    DrawText(terrainDesc(ter), cx, y, 13, Color{115, 130, 115, 255});
    y += 28;
}

static void renderUnitSection(int cx, int cw, int& y, const Unit* u) {
    drawDivider(cx, cw, y);

    const Color pc    = playerColor(u->getOwner().getId());
    const Color dimPc = {(unsigned char)(pc.r / 2),
                         (unsigned char)(pc.g / 2),
                         (unsigned char)(pc.b / 2), 255};

    DrawText("UNIT", cx, y, 12, Color{130, 130, 155, 255});
    DrawRectangle(cx + cw - 66, y, 66, 20, dimPc);
    DrawText(TextFormat("Player %d", u->getOwner().getId() + 1),
             cx + cw - 60, y + 4, 12, pc);
    y += 22;
    DrawText(unitName(u->getType()), cx, y, 22, pc);
    y += 32;

    const float hpFrac = (float)u->getHealth() / (float)u->getMaxHealth();
    const Color hpCol  = hpFrac >= 0.5f ? Color{200, 220, 200, 255} : Color{220, 60, 60, 255};
    DrawText("HP", cx, y, 13, Color{150, 150, 170, 255});
    DrawText(TextFormat("%d / %d", u->getHealth(), u->getMaxHealth()), cx + 30, y, 15, hpCol);
    y += 30;

    const StatBox coreStats[2] = {
        {"MOV", u->getMovement(), {120, 160, 220, 255}, {160, 200, 255, 255}},
        {"RNG", u->getRange(),    {180, 210, 130, 255}, {210, 240, 160, 255}},
    };
    drawWideStatRow(cx, cw, y, coreStats, 2);

    const StatBox offStats[4] = {
        {"STR", u->getStrength(),   {230, 140,  80, 255}, {255, 175, 110, 255}},
        {"SPC", u->getSpecial(),    {185, 110, 230, 255}, {215, 150, 255, 255}},
        {"PRE", u->getPrecision(),  {180, 210,  80, 255}, {210, 240, 110, 255}},
        {"INI", u->getInitiative(), {230, 190,  60, 255}, {255, 220,  90, 255}},
    };
    drawCompactStatRow(cx, cw, y, offStats, 4);

    const StatBox defStats[4] = {
        {"DEF", u->getDefense(),    {100, 150, 220, 255}, {130, 185, 255, 255}},
        {"RES", u->getResistance(), {160,  90, 220, 255}, {195, 130, 255, 255}},
        {"AGI", u->getAgility(),    { 90, 215, 195, 255}, {120, 250, 225, 255}},
        {"GRD", u->getGuard(),      {215, 165,  90, 255}, {250, 200, 120, 255}},
    };
    drawCompactStatRow(cx, cw, y, defStats, 4);

    const int sw = cw / 3;
    auto pill = [&](const char* text, bool ok, int x) {
        const Color bg  = ok ? Color{20,  60,  30, 255} : Color{ 50, 20,  20, 255};
        const Color bdr = ok ? Color{60, 170,  80, 255} : Color{130, 50,  50, 255};
        const Color tc  = ok ? Color{90, 220, 110, 255} : Color{170, 70,  70, 255};
        DrawRectangle(x, y, sw - 6, 28, bg);
        DrawRectangleLines(x, y, sw - 6, 28, bdr);
        DrawText(text, x + 8, y + 8, 12, tc);
    };
    pill("MOVE",   u->canMove(),   cx);
    pill("ATTACK", u->canAttack(), cx + sw);
    pill(u->isExhausted() ? "DONE" : "READY", !u->isExhausted(), cx + sw * 2);
    y += 38;
}

static void renderCitySection(int cx, int cw, int& y,
                              const World& world, const Tile& tile, const City* city) {
    drawDivider(cx, cw, y);
    DrawText("CITY", cx, y, 12, Color{130, 130, 155, 255});
    y += 22;

    DrawText(city->getName().c_str(), cx, y, 20, Color{255, 220, 80, 255});
    y += 30;

    if (city->hasOwner()) {
        DrawText(TextFormat("Owner: Player %d", city->getOwner().getId() + 1),
                 cx, y, 14, playerColor(city->getOwner().getId()));
    } else {
        DrawText("Unclaimed", cx, y, 14, Color{140, 140, 160, 255});
    }
    y += 22;

    if (city->isTraining()) {
        const TrainingSlot* slot = city->getTrainingSlot();
        DrawText("TRAINING", cx, y, 12, Color{140, 190, 100, 255});
        y += 18;

        DrawText(unitName(slot->unitType), cx, y, 17, Color{200, 235, 150, 255});
        const char* turnsBadge = slot->turnsRemaining == 1
            ? "1 turn" : TextFormat("%d turns", slot->turnsRemaining);
        const Color badgeCol = slot->turnsRemaining <= 1
            ? Color{100, 220, 80, 255} : Color{180, 180, 80, 255};
        DrawText(turnsBadge, cx + cw - MeasureText(turnsBadge, 13) - 2, y + 3, 13, badgeCol);
        y += 24;

        const float prog = 1.0f - slot->turnsRemaining / 2.0f;
        DrawRectangle(cx, y, cw, 6, Color{35, 45, 28, 255});
        DrawRectangle(cx, y, (int)(cw * prog), 6, Color{85, 175, 55, 255});
        DrawRectangleLines(cx, y, cw, 6, Color{55, 80, 40, 255});
        y += 16;
    } else {
        const int  owned   = city->hasOwner()
                             ? world.countUnitsForPlayer(city->getOwner().getId()) : 0;
        const bool capFull = owned >= TrainingSystem::MAX_UNITS_PER_PLAYER;
        const bool tileOcc = tile.hasUnit();

        const char* trainLabel;
        Color       trainCol;
        if (capFull) {
            trainLabel = TextFormat("Unit cap (%d/%d)", owned, TrainingSystem::MAX_UNITS_PER_PLAYER);
            trainCol   = Color{180, 80, 80, 255};
        } else if (tileOcc) {
            trainLabel = "Tile occupied \xe2\x80\x94 move unit first";
            trainCol   = Color{150, 130, 60, 255};
        } else {
            trainLabel = TextFormat("Ready  (%d/%d slots)", owned, TrainingSystem::MAX_UNITS_PER_PLAYER);
            trainCol   = Color{90, 210, 110, 255};
        }
        DrawText(trainLabel, cx, y, 13, trainCol);
        y += 22;
    }
}

static void renderCombatForecast(int cx, int cw, int& y,
                                 const World& world,
                                 const Unit* sel, const Unit* def,
                                 const Position& selPos, const Position& hoverPos) {
    const World::CombatForecast fc = world.getCombatForecast(selPos, hoverPos);

    DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
    y += 14;

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

    DrawText("ATTACK", cx, y, 11, Color{130, 130, 155, 255});
    drawHitBadge(cx, cw, y, fc.attackHitChance);
    y += 16;
    drawForecastRow(cx, cw, y, unitName(sel->getType()), playerColor(sel->getOwner().getId()),
                    fc.damage, fc.defenderHpBefore, fc.defenderHpAfter, fc.lethal);

    if (!fc.lethal && fc.retaliation > 0) {
        y += 4;
        DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 160});
        y += 10;
        DrawText("RETALIATION", cx, y, 11, Color{130, 130, 155, 255});
        drawHitBadge(cx, cw, y, fc.retaliationHitChance);
        y += 16;
        drawForecastRow(cx, cw, y, unitName(def->getType()), playerColor(def->getOwner().getId()),
                        fc.retaliation, fc.attackerHpBefore, fc.attackerHpAfter, fc.attackerDies);
    }
}

// ---------------------------------------------------------------------------
// InformationView
// ---------------------------------------------------------------------------

InformationView::InformationView() {}

void InformationView::render(const World& world,
                             const Position* hoverPos,
                             const Position* selectedPos) const {
    const int PX  = GRID_RIGHT + 4;
    const int PW  = 1916 - PX;
    const int PAD = 14;
    const int cx  = PX + PAD;
    const int cw  = PW - PAD * 2;

    DrawRectangle(PX, 0, PW, 1080, Color{20, 20, 32, 255});
    DrawLine(PX, 0, PX, 1080, Color{55, 55, 75, 255});

    int y = 70;
    DrawText("TILE INFO", cx, y, 14, Color{140, 140, 165, 255});
    y += 20;
    DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
    y += 16;

    if (!hoverPos) {
        DrawText("Hover over a tile", cx, y, 15, GRAY);
        return;
    }

    const Tile&   tile = world.getTileAt(*hoverPos);
    const Terrain ter  = tile.getTerrain();

    renderTerrainSection(cx, cw, y, ter);

    if (tile.hasUnit()) {
        const Unit* u = world.getUnit(tile.getUnit().value());
        if (u) renderUnitSection(cx, cw, y, u);
    }

    if (tile.hasCity())
        renderCitySection(cx, cw, y, world, tile, tile.getCity());

    if (!selectedPos || !tile.hasUnit() || !world.hasUnitAt(*selectedPos)) return;
    const Unit* sel = world.getUnitAt(*selectedPos);
    const Unit* def = world.getUnit(tile.getUnit().value());
    if (!sel || !def || sel->sameOwner(*def)) return;

    renderCombatForecast(cx, cw, y, world, sel, def, *selectedPos, *hoverPos);
}
