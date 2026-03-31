#pragma once
#include <optional>
#include <string>
#include <vector>
#include "controller/action.h"
#include "controller/error.h"
#include "model/world.h"
#include "model/util.h"
#include "view/layout.h"

// Sub-views are owned by GUI via pointers; headers included in gui.cpp.
class GridView;
class ActionView;
class InformationView;
class ErrorView;

// ---------------------------------------------------------------------------
// GUI — top-level view.  Owns all sub-views and the screen layout.
// ---------------------------------------------------------------------------
class GUI {
public:
    GUI(int width, int height);
    ~GUI();

    /** Render the full frame. */
    void render(const World& world,
                const Position& hoverPos,
                const Position* selectedPos,
                const std::vector<std::string>& actionLabels,
                ControllerMode currentMode);

    /** Resolve a left-click to a ClickTarget.  Priority: grid > action > mode. */
    std::optional<ClickTarget> pollClick(const std::vector<std::string>& actionLabels) const;

    /** Grid tile under the cursor, or nullopt if off-grid. */
    std::optional<Position> pollHover() const;

    void setError(PlayerError error);
    void clearError();

    /** Pan the grid viewport by (dpx, dpy) pixels. */
    void scrollGrid(int dpx, int dpy);

    /** Returns true if the END TURN button was clicked this frame. */
    bool pollEndTurn() const;

private:
    int screenWidth, screenHeight;

    std::vector<Rect> actionButtonSlots;
    std::vector<Rect> modeButtonSlots;
    Rect endTurnButton;

    GridView*        gridView;
    ActionView*      actionView;
    InformationView* informationView;
    ErrorView*       errorView;

    std::optional<Position> pixelToTile(int px, int py) const;
};
