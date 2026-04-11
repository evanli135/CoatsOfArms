#pragma once
#include <string>
#include <vector>
#include "view/layout.h"

// ---------------------------------------------------------------------------
// ActionView — context-sensitive action button strip (left panel).
// ---------------------------------------------------------------------------
class ActionView {
public:
    ActionView();
    void render(const std::vector<std::string>& labels,
                const std::vector<Rect>& buttonSlots,
                int pendingIndex,
                const std::vector<bool>& enabled = {}) const;
};
