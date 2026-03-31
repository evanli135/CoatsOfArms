#include "view/panel_views.h"
#include "view/sprites.h"
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

void InformationView::render(const World& world, const Position* hoverPos) const {
    int x = INFO_X;
    int y = BOTTOM_Y;
    const int lineH = 22;

    DrawText("INFO", x, y, 16, Color{160, 160, 180, 255});
    y += lineH;

    if (!hoverPos) {
        DrawText("Hover over a tile", x, y, 15, GRAY);
        return;
    }

    const Tile& tile = world.getTileAt(*hoverPos);

    DrawText(TextFormat("%-12s  cost: %s", terrainName(tile.getTerrain()), terrainCost(tile.getTerrain())),
             x, y, 15, Color{180, 200, 180, 255});
    y += lineH;

    if (tile.hasUnit()) {
        const Unit* u = world.getUnit(tile.getUnit().value());
        if (u) {
            Color c = playerColor(u->getOwner().getId());
            DrawText(TextFormat("%s %s  (P%d)",
                     unitSymbol(u->getType()), unitName(u->getType()),
                     u->getOwner().getId() + 1), x, y, 15, c);
            y += lineH;
            DrawText(TextFormat("HP %d/%d  ATK %d  MOV %d  RNG %d  %s",
                     u->getHealth(), u->getMaxHealth(),
                     u->getDamage(), u->getMovement(), u->getRange(),
                     u->canMove() ? "ready" : "spent"), x, y, 14, LIGHTGRAY);
            y += lineH;
        }
    }

    if (tile.hasCity())
        DrawText("City present", x, y, 14, Color{255, 220, 80, 200});
}

// ---------------------------------------------------------------------------
// ActionView
// ---------------------------------------------------------------------------

ActionView::ActionView() {}

void ActionView::render(const std::vector<std::string>& labels,
                        const std::vector<Rect>& buttonSlots,
                        int pendingIndex) const
{
    for (int i = 0; i < (int)labels.size() && i < (int)buttonSlots.size(); ++i) {
        const Rect& r = buttonSlots[i];
        bool active   = (i == pendingIndex);
        Color bg      = active ? Color{60, 180, 80, 255} : Color{45, 45, 65, 255};
        DrawRectangle(r.x, r.y, r.w, r.h, bg);
        DrawRectangleLines(r.x, r.y, r.w, r.h, active ? GREEN : Color{90, 90, 110, 255});
        DrawText(labels[i].c_str(), r.x+10, r.y+11, 16, WHITE);
    }
}
