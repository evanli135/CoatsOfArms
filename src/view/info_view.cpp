#include "view/info_view.h"
#include "view/layout.h"
#include "model/tile.h"
#include "model/unit.h"
#include "model/city.h"
#include "raylib.h"

using namespace Layout;

// ---------------------------------------------------------------------------
// Local helpers — only used by InformationView
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

            DrawText("UNIT", cx, y, 12, Color{130, 130, 155, 255});
            DrawRectangle(cx + cw - 66, y, 66, 20, dimPc);
            DrawText(TextFormat("Player %d", u->getOwner().getId() + 1),
                     cx + cw - 60, y + 4, 12, pc);
            y += 22;
            DrawText(unitName(u->getType()), cx, y, 22, pc);
            y += 32;

            const float hp    = (float)u->getHealth() / (float)u->getMaxHealth();
            const Color hpCol = hp >= 0.5f ? Color{200, 220, 200, 255}
                                           : Color{220,  60,  60, 255};
            DrawText("HP", cx, y, 13, Color{150, 150, 170, 255});
            DrawText(TextFormat("%d / %d", u->getHealth(), u->getMaxHealth()),
                     cx + 30, y, 15, hpCol);
            y += 24;

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
        const City* city = tile.getCity();
        DrawLine(cx, y, cx + cw, y, Color{55, 55, 75, 200});
        y += 16;
        DrawText("CITY", cx, y, 12, Color{130, 130, 155, 255});
        y += 22;

        DrawText(city->getName().c_str(), cx, y, 20, Color{255, 220, 80, 255});
        y += 30;

        if (city->hasOwner()) {
            const Color ownerCol = playerColor(city->getOwner().getId());
            DrawText(TextFormat("Owner: Player %d", city->getOwner().getId() + 1),
                     cx, y, 14, ownerCol);
        } else {
            DrawText("Unclaimed", cx, y, 14, Color{140, 140, 160, 255});
        }
        y += 22;

        bool tileOccupied  = tile.hasUnit();
        bool alreadyTrained = city->hasTrainedThisTurn();
        const char* trainLabel;
        Color       trainCol;
        if (alreadyTrained) {
            trainLabel = "Trained this turn";
            trainCol   = Color{150, 70, 70, 255};
        } else if (tileOccupied) {
            trainLabel = "Tile occupied";
            trainCol   = Color{150, 130, 60, 255};
        } else {
            trainLabel = "Ready to train";
            trainCol   = Color{90, 210, 110, 255};
        }
        DrawText(trainLabel, cx, y, 13, trainCol);
        y += 22;
    }

    // ── Combat Forecast ─────────────────────────────────────────────────────
    if (!selectedPos || !tile.hasUnit()) return;
    if (!world.hasUnitAt(*selectedPos))  return;

    const Unit* sel = world.getUnitAt(*selectedPos);
    const Unit* def = world.getUnit(tile.getUnit().value());
    if (!sel || !def || sel->sameOwner(*def)) return;

    const World::CombatForecast fc = world.getCombatForecast(*selectedPos, *hoverPos);

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

    auto drawForecastRow = [&](const char* label, Color labelCol,
                               int dmg, int hpBefore, int hpAfter, int maxHp,
                               bool dies) {
        DrawText(label, cx, y, 14, labelCol);

        const char* dmgStr = TextFormat("-%d HP", dmg);
        const int   dmgW   = MeasureText(dmgStr, 13);
        DrawRectangle(cx + cw - dmgW - 14, y - 1, dmgW + 14, 20, Color{130, 35, 35, 200});
        DrawRectangleLines(cx + cw - dmgW - 14, y - 1, dmgW + 14, 20, Color{210, 75, 75, 255});
        DrawText(dmgStr, cx + cw - dmgW - 7, y + 2, 13, Color{255, 150, 150, 255});
        y += 22;

        const Color hpCol = dies ? Color{220, 60, 60, 255}
                          : hpAfter >= hpBefore / 2 ? Color{180, 215, 180, 255}
                          :                           Color{230, 155, 60, 255};
        DrawText(TextFormat("HP  %d → %d", hpBefore, hpAfter), cx + 8, y, 13, hpCol);
        if (dies) {
            const char* tag = "DESTROYED";
            DrawText(tag, cx + cw - MeasureText(tag, 12) - 2, y + 1, 12, Color{220, 80, 80, 255});
        }
        y += 20;
        (void)maxHp;
    };

    const Color atkCol = playerColor(sel->getOwner().getId());
    DrawText("ATTACK", cx, y, 11, Color{130, 130, 155, 255});
    y += 16;
    drawForecastRow(unitName(sel->getType()), atkCol,
                    fc.damage,
                    fc.defenderHpBefore, fc.defenderHpAfter, def->getMaxHealth(),
                    fc.lethal);

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
