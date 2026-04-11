#pragma once
#include <optional>
#include "controller/action.h"
#include "model/world.h"
#include "model/util.h"

// ---------------------------------------------------------------------------
// InformationView — hover-driven right panel: terrain, unit stats, city info,
// and combat/charge forecast when a friendly unit is selected.
// ---------------------------------------------------------------------------
class InformationView {
public:
    InformationView();
    void render(const World& world,
                const Position* hoverPos,
                const Position* selectedPos,
                int panelX, int panelW, int screenH,
                std::optional<ControllerAction> pendingAction = std::nullopt) const;
};
