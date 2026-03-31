#pragma once
#include <optional>
#include <string>
#include <vector>
#include "model/world.h"
#include "model/util.h"
#include "controller/error.h"
#include "view/layout.h"

// ---------------------------------------------------------------------------
// ErrorView — pill overlay shown in the top-right corner of the grid.
// ---------------------------------------------------------------------------
class ErrorView {
public:
    ErrorView();
    void setError(PlayerError error);
    void clearError();
    void render(int x, int y) const;
private:
    std::optional<PlayerError> currentError;
};

// ---------------------------------------------------------------------------
// InformationView — hover-driven info panel: terrain, unit stats, city.
// ---------------------------------------------------------------------------
class InformationView {
public:
    InformationView();
    void render(const World& world, const Position* hoverPos) const;
};

// ---------------------------------------------------------------------------
// ActionView — action button strip (context-sensitive labels).
// ---------------------------------------------------------------------------
class ActionView {
public:
    ActionView();
    void render(const std::vector<std::string>& labels,
                const std::vector<Rect>& buttonSlots,
                int pendingIndex) const;
};
