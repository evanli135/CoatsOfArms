#pragma once
#include "model/world.h"
#include "model/util.h"

// ---------------------------------------------------------------------------
// InformationView — hover-driven right panel: terrain, unit stats, city info,
// and combat forecast when a friendly unit is selected.
// ---------------------------------------------------------------------------
class InformationView {
public:
    InformationView();
    void render(const World& world,
                const Position* viewPos,    // hovered or pinned tile
                const Position* selectedPos,
                int panelX, int panelW, int screenH,
                bool isPinned = false) const;
};
