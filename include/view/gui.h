#pragma once
#include <optional>
#include <string>
#include <vector>
#include "controller/action.h"
#include "controller/error.h"
#include "controller/observer.h"
#include "model/world.h"
#include "model/util.h"
#include "view/damage_indicators.h"
#include "view/explosion.h"
#include "view/layout.h"

// Sub-views are owned by GUI via pointers; headers included in gui.cpp.
class GridView;
class ActionView;
class InformationView;
class ErrorView;

// ---------------------------------------------------------------------------
// GUI — top-level view.  Owns all sub-views and the screen layout.
// ---------------------------------------------------------------------------
class GUI : public ModelObserver {
public:
    GUI(int width, int height);
    ~GUI();

    /** Render the full frame. */
    void render(const World& world,
                const Position& hoverPos,
                const Position* selectedPos,
                const std::vector<std::string>& actionLabels,
                ControllerMode currentMode,
                int pendingActionIndex = -1);

    /** Resolve a left-click to a ClickTarget.  Priority: grid > action > mode. */
    std::optional<ClickTarget> pollClick(const std::vector<std::string>& actionLabels);

    /** Grid tile under the cursor, or nullopt if off-grid. */
    std::optional<Position> pollHover();

    void onModelChanged(const ModelEvent& event) override;

    void setError(PlayerError error);
    void clearError();

    /** Pan the grid viewport by (dpx, dpy) pixels. */
    void scrollGrid(int dpx, int dpy);

    /**
     * Click-and-drag on the map (outside chrome) pans the grid.
     * Call once per frame before pollHover / pollClick.
     */
    void pollMapPan();

    /** Trackpad / mouse wheel zoom over the map (ignored over chrome). */
    void pollMapZoom();

    /** Returns true if the END TURN button was clicked this frame. */
    bool pollEndTurn();

private:
    int screenWidth, screenHeight;
    Layout::ViewLayout frameLayout_{};

    void updateLayout();

    std::vector<Rect> actionButtonSlots;
    std::vector<Rect> modeButtonSlots;
    Rect endTurnButton;

    GridView*        gridView;
    ActionView*      actionView;
    InformationView* informationView;
    ErrorView*       errorView;

    DamageIndicatorSystem damageIndicators;
    ExplosionSystem       explosions;

    // Turn-change banner
    float turnBannerAge      = 999.0f;
    int   turnBannerPlayerId = 0;
    static constexpr float BANNER_DURATION = 2.2f;

    // Training-started toast
    float    trainingToastAge      = 999.0f;
    UnitType trainingToastUnitType = UnitType::WARRIOR;
    int      trainingToastPlayerId = 0;
    static constexpr float TOAST_DURATION = 2.0f;

    std::optional<Position> pixelToTile(int px, int py) const;
    std::pair<int,int>      tileToPixel(const Position& pos) const;

    bool isOverChrome(int mx, int my) const;

    Camera2D mapCamera() const;

    float mapZoom_ = 1.0f;

    enum class MapDragPhase { None, Armed, Panning };
    MapDragPhase mapDragPhase_      = MapDragPhase::None;
    int          mapDragStartX_     = 0;
    int          mapDragStartY_     = 0;
    int          mapDragLastX_      = 0;
    int          mapDragLastY_      = 0;
    bool         mapDragStartedOnGrid_ = false;
};
