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
                const Position* hoverPos,
                const Position* selectedPos = nullptr) const;
};
