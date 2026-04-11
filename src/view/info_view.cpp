#include "view/info_view.h"
#include "view/layout.h"
#include <algorithm>
#include <cmath>
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
// Drawing primitives — shared
// ---------------------------------------------------------------------------

static void drawDivider(int cx, int cw, int& y) {
    DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
    y += 16;
}

struct StatBox { const char* label; int val; Color lc; Color vc; };

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

// ---------------------------------------------------------------------------
// Combat forecast primitives
// ---------------------------------------------------------------------------

// Two name cards with an arrow — shows who attacks whom.
static void drawCombatMatchup(int cx, int cw, int& y,
                               const char* atkName, Color atkCol,
                               const char* defName, Color defCol) {
    const int arrowZone = 28;
    const int cardW     = (cw - arrowZone) / 2;
    const int cardH     = 34;

    // Attacker card (left) — dimmed fill, colored border
    Color atkDim = {(unsigned char)(atkCol.r / 5), (unsigned char)(atkCol.g / 5),
                    (unsigned char)(atkCol.b / 5), 255};
    DrawRectangle(cx, y, cardW, cardH, atkDim);
    DrawRectangleLines(cx, y, cardW, cardH, atkCol);
    // Top accent bar
    DrawRectangle(cx, y, cardW, 3, atkCol);
    DrawText(atkName, cx + 8, y + 10, 15, atkCol);

    // Arrow
    const char* arrow = "-->";
    int aw = MeasureText(arrow, 12);
    DrawText(arrow, cx + cardW + (arrowZone - aw) / 2, y + (cardH - 12) / 2,
             12, Color{170, 170, 195, 180});

    // Defender card (right)
    Color defDim = {(unsigned char)(defCol.r / 5), (unsigned char)(defCol.g / 5),
                    (unsigned char)(defCol.b / 5), 255};
    const int rx = cx + cardW + arrowZone;
    DrawRectangle(rx, y, cardW, cardH, defDim);
    DrawRectangleLines(rx, y, cardW, cardH, defCol);
    DrawRectangle(rx, y, cardW, 3, defCol);
    DrawText(defName, rx + 8, y + 10, 15, defCol);

    y += cardH + 10;
}

// Hit-chance row: label on left, big coloured % on right — no bar.
static void drawHitChanceRow(int cx, int cw, int& y, int hitPct) {
    const Color hitCol = hitPct >= 85 ? Color{ 90, 210,  90, 255}
                       : hitPct >= 65 ? Color{215, 195,  60, 255}
                       :                Color{210,  80,  80, 255};

    DrawText("HIT CHANCE", cx, y + 5, 11, Color{130, 130, 155, 255});
    const char* pctStr = TextFormat("%d%%", hitPct);
    DrawText(pctStr, cx + cw - MeasureText(pctStr, 22), y, 22, hitCol);
    y += 30;
}

// HP bar: green=surviving, flashing red=damage zone, numeric label below.
// flash: 0.0–1.0 pulse value driven by GetTime() in the caller.
static void drawHpDamageBar(int cx, int cw, int& y,
                             int hpBefore, int hpAfter, int maxHp, bool dies,
                             float flash) {
    const float afterFrac  = std::max(0.0f, (float)hpAfter  / (float)maxHp);
    const float beforeFrac = std::min(1.0f, (float)hpBefore / (float)maxHp);

    // Background track
    DrawRectangle(cx, y, cw, 12, Color{25, 25, 38, 255});

    // Surviving HP — green
    if (hpAfter > 0)
        DrawRectangle(cx, y, (int)(cw * afterFrac), 12, Color{55, 175, 75, 255});

    // Damage zone — flashing red
    const int dmgBarX = (int)(cw * afterFrac);
    const int dmgBarW = (int)(cw * (beforeFrac - afterFrac));
    if (dmgBarW > 0) {
        const unsigned char alpha = (unsigned char)(90 + (int)(flash * 165.0f));
        DrawRectangle(cx + dmgBarX, y, dmgBarW, 12, Color{230, 55, 55, alpha});
        // Bright inner highlight at peak flash
        if (flash > 0.6f) {
            const unsigned char hi = (unsigned char)((flash - 0.6f) / 0.4f * 120.0f);
            DrawRectangle(cx + dmgBarX, y + 2, dmgBarW, 4, Color{255, 130, 130, hi});
        }
    }

    DrawRectangleLines(cx, y, cw, 12, Color{55, 55, 75, 200});
    y += 15;

    // Numeric label
    DrawText(TextFormat("%d / %d", hpBefore, maxHp), cx, y, 12, Color{130, 130, 155, 255});
    if (dies) {
        DrawText("FATAL", cx + cw - MeasureText("FATAL", 12), y, 12, Color{220, 60, 60, 255});
    } else {
        const char* outcomeStr = TextFormat("-> %d  (-%d)", hpAfter, hpBefore - hpAfter);
        DrawText(outcomeStr, cx + cw - MeasureText(outcomeStr, 12), y, 12, Color{200, 100, 100, 255});
    }
    y += 18;
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

static void renderUnitSection(int cx, int cw, int& y, const Unit* u, bool isActor) {
    drawDivider(cx, cw, y);

    const Color pc    = playerColor(u->getOwner().getId());
    const Color dimPc = {(unsigned char)(pc.r / 2),
                         (unsigned char)(pc.g / 2),
                         (unsigned char)(pc.b / 2), 255};

    // Label: "ACTOR" when this is the selected/attacking unit, "UNIT" otherwise
    DrawText(isActor ? "ACTOR" : "UNIT", cx, y, 12, Color{130, 130, 155, 255});
    DrawRectangle(cx + cw - 66, y, 66, 20, dimPc);
    DrawText(TextFormat("Player %d", u->getOwner().getId() + 1),
             cx + cw - 60, y + 4, 12, pc);
    y += 22;
    DrawText(unitName(u->getType()), cx, y, 22, pc);
    y += 32;

    const float hpFrac = (float)u->getHealth() / (float)u->getMaxHealth();
    DrawText("HP", cx, y, 13, Color{150, 150, 170, 255});
    DrawText(TextFormat("%d / %d", u->getHealth(), u->getMaxHealth()), cx + 30, y, 15,
             hpFrac >= 0.5f ? Color{200, 220, 200, 255} : Color{220, 60, 60, 255});
    y += 26;

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
            trainLabel = "Tile occupied -- move unit first";
            trainCol   = Color{150, 130, 60, 255};
        } else {
            trainLabel = TextFormat("Ready  (%d/%d slots)", owned, TrainingSystem::MAX_UNITS_PER_PLAYER);
            trainCol   = Color{90, 210, 110, 255};
        }
        DrawText(trainLabel, cx, y, 13, trainCol);
        y += 22;
    }
}

static void renderChargeForecast(int cx, int cw, int& y,
                                  const Unit* cav, const Unit* enemy) {
    DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
    y += 14;

    DrawText("CHARGE FORECAST", cx, y, 13, Color{255, 160, 60, 255});
    y += 24;

    const Color cavCol   = playerColor(cav->getOwner().getId());
    const Color enemyCol = playerColor(enemy->getOwner().getId());
    drawCombatMatchup(cx, cw, y, unitName(cav->getType()), cavCol,
                      unitName(enemy->getType()), enemyCol);

    DrawText("CHARGE", cx, y, 16, Color{255, 160, 60, 255});
    const char* gLabel = "(guaranteed hit)";
    DrawText(gLabel, cx + cw - MeasureText(gLabel, 11), y + 4, 11,
             Color{130, 130, 155, 255});
    y += 24;

    int dmg    = cav->computeChargeDamageAgainst(*enemy);
    int hpLeft = std::max(0, enemy->getHealth() - dmg);
    bool fatal = (hpLeft == 0);

    const float flash = (sinf((float)GetTime() * 6.0f) + 1.0f) * 0.5f;

    DrawText(unitName(enemy->getType()), cx, y, 14, enemyCol);
    DrawText("loses HP", cx + cw - MeasureText("loses HP", 11), y + 2, 11,
             Color{175, 100, 100, 255});
    y += 18;
    drawHpDamageBar(cx, cw, y,
                    enemy->getHealth(), hpLeft, enemy->getMaxHealth(), fatal, flash);

    y += 8;
    DrawText("No retaliation vs. charges.", cx, y, 12, Color{100, 175, 100, 255});
    y += 20;
}

static void renderCombatForecast(int cx, int cw, int& y,
                                 const World& world,
                                 const Unit* atk, const Unit* def,
                                 const Position& atkPos, const Position& defPos) {
    const World::CombatForecast fc = world.getCombatForecast(atkPos, defPos);

    DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
    y += 14;

    // Section header
    const Color hdCol = fc.inRange && fc.attackerCanAct
                      ? Color{230, 180,  60, 255}
                      : Color{120, 120, 145, 255};
    DrawText("COMBAT FORECAST", cx, y, 13, hdCol);
    y += 24;

    if (!fc.attackerCanAct) {
        DrawText("Attacker has already acted.", cx, y, 13, Color{130, 100, 100, 255});
        return;
    }
    if (!fc.inRange) {
        DrawText("Target out of attack range.", cx, y, 13, Color{130, 100, 100, 255});
        return;
    }

    // Shared flash pulse — 0.0 to 1.0, ~3 Hz
    const float flash = (sinf((float)GetTime() * 6.0f) + 1.0f) * 0.5f;

    const Color atkCol = playerColor(atk->getOwner().getId());
    const Color defCol = playerColor(def->getOwner().getId());

    // Matchup header: [Attacker] --> [Defender]
    drawCombatMatchup(cx, cw, y,
                      unitName(atk->getType()), atkCol,
                      unitName(def->getType()), defCol);

    // ── ATTACK ───────────────────────────────────────────────────────────────
    DrawText("ATTACK", cx, y, 16, Color{230, 160, 60, 255});
    y += 22;
    drawHitChanceRow(cx, cw, y, fc.attackHitChance);

    // Label the unit whose HP is shown
    DrawText(unitName(def->getType()), cx, y, 14, defCol);
    DrawText("loses HP", cx + cw - MeasureText("loses HP", 11), y + 2, 11,
             Color{175, 100, 100, 255});
    y += 18;
    drawHpDamageBar(cx, cw, y,
                    fc.defenderHpBefore, fc.defenderHpAfter,
                    def->getMaxHealth(), fc.lethal, flash);

    // ── RETALIATION ──────────────────────────────────────────────────────────
    if (fc.lethal) {
        y += 4;
        DrawText("No retaliation -- target eliminated.", cx, y, 12, Color{100, 145, 100, 255});
        y += 18;
    } else if (fc.retaliation > 0) {
        y += 8;
        DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 120});
        y += 12;
        DrawText("RETALIATION", cx, y, 16, Color{160, 110, 220, 255});
        y += 22;
        drawHitChanceRow(cx, cw, y, fc.retaliationHitChance);

        DrawText(unitName(atk->getType()), cx, y, 14, atkCol);
        DrawText("loses HP", cx + cw - MeasureText("loses HP", 11), y + 2, 11,
                 Color{175, 100, 100, 255});
        y += 18;
        drawHpDamageBar(cx, cw, y,
                        fc.attackerHpBefore, fc.attackerHpAfter,
                        atk->getMaxHealth(), fc.attackerDies, flash);
    }
}

// ---------------------------------------------------------------------------
// InformationView
// ---------------------------------------------------------------------------

InformationView::InformationView() {}

void InformationView::render(const World& world,
                             const Position* hoverPos,
                             const Position* selectedPos,
                             int panelX, int panelW, int screenH,
                             std::optional<ControllerAction> pendingAction) const {
    const int PAD = 14;
    const int cx  = panelX + PAD;
    const int cw  = panelW - PAD * 2;

    DrawRectangle(panelX, 0, panelW, screenH, Color{20, 20, 32, 255});
    DrawLine(panelX, 0, panelX, screenH, Color{55, 55, 75, 255});

    int y = 70;
    DrawText("INFO", cx, y, 14, Color{140, 140, 165, 255});
    y += 20;
    DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
    y += 16;

    if (!hoverPos) {
        DrawText("Hover over a tile", cx, y, 15, GRAY);
        return;
    }

    const Tile& tile = world.getTileAt(*hoverPos);

    // 1. Terrain section — always shown for the hovered tile.
    renderTerrainSection(cx, cw, y, tile.getTerrain());

    // 2. Actor section — show the selected unit (the one that will act).
    //    Falls back to the hovered unit when nothing is selected.
    const Unit* actor  = nullptr;
    bool        isActorSelected = false;
    if (selectedPos && world.hasUnitAt(*selectedPos)) {
        actor          = world.getUnitAt(*selectedPos);
        isActorSelected = true;
    } else if (tile.hasUnit()) {
        actor = world.getUnit(tile.getUnit().value());
    }
    if (actor) renderUnitSection(cx, cw, y, actor, isActorSelected);

    // 3. City section — from the hovered tile.
    if (tile.hasCity())
        renderCitySection(cx, cw, y, world, tile, tile.getCity());

    // 4. Forecast section
    if (!selectedPos || !tile.hasUnit()) return;
    const Unit* sel = world.hasUnitAt(*selectedPos) ? world.getUnitAt(*selectedPos) : nullptr;
    if (!sel) return;
    const Unit* def = world.getUnit(tile.getUnit().value());
    if (!def || sel->sameOwner(*def)) return;

    // Charge forecast — when CHARGE is pending and the hovered enemy is in a charge lane
    if (pendingAction == ControllerAction::CHG &&
        sel->getType() == UnitType::CAVALRY && hoverPos) {
        static const int DRS[4] = {-1, 1, 0, 0};
        static const int DCS[4] = { 0, 0, -1, 1};
        bool inLane = false;
        for (int d = 0; d < 4 && !inLane; ++d) {
            auto path = world.previewChargeInDir(*selectedPos, DRS[d], DCS[d]);
            if (path.hitEnemy && path.enemyPos == *hoverPos) inLane = true;
        }
        if (inLane) {
            renderChargeForecast(cx, cw, y, sel, def);
            return;
        }
    }

    // Regular combat forecast
    renderCombatForecast(cx, cw, y, world, sel, def, *selectedPos, *hoverPos);
}
