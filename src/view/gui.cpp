#include "view/gui.h"
#include "view/grid_view.h"
#include "view/panel_views.h"
#include "view/sprites.h"
#include "view/layout.h"
#include "model/util.h"
#include "raylib.h"
#include <cmath>

using namespace Layout;

// ---------------------------------------------------------------------------
// GUI
// ---------------------------------------------------------------------------

GUI::GUI(int width, int height)
    : screenWidth(width), screenHeight(height)
{
    for (int i = 0; i < 3; ++i)
        modeButtonSlots.push_back(Rect{ACT_X + i*(ICON_SIZE+BTN_GAP), MODE_BTN_Y, ICON_SIZE, ICON_SIZE});

    for (int i = 0; i < 8; ++i)
        actionButtonSlots.push_back(Rect{ACT_X, ACT_BTN_Y + i*(BTN_H+BTN_GAP), BTN_W, BTN_H});

    // END TURN sits below the 8 action button slots with a small gap
    endTurnButton = Rect{ACT_X, ACT_BTN_Y + 8*(BTN_H+BTN_GAP) + 16, BTN_W + 24, BTN_H + 8};

    errorView       = new ErrorView();
    informationView = new InformationView();
    actionView      = new ActionView();
    gridView        = new GridView();
}

GUI::~GUI() {
    delete errorView;
    delete informationView;
    delete actionView;
    delete gridView;
}

void GUI::render(const World& world,
                 const Position& hoverPos,
                 const Position* selectedPos,
                 const std::vector<std::string>& actionLabels,
                 ControllerMode currentMode,
                 int pendingActionIndex)
{
    ClearBackground(Color{16, 16, 26, 255});

    // Title bar
    int cp = world.getCurrentPlayer().getId();
    DrawText(TextFormat("Player %d  |  Turn %d", cp+1, world.getTurn()),
             screenWidth/2 - 80, 16, 16, playerColor(cp));

    // Compute movement reach and attackable tiles for the selected unit
    std::vector<Position> reachable;
    std::vector<Position> attackable;
    std::vector<bool>     enabledActions(actionLabels.size(), true);

    if (selectedPos && world.hasUnitAt(*selectedPos)) {
        const Unit* selUnit = world.getUnitAt(*selectedPos);
        if (selUnit) {
            reachable  = world.getMovementSnapshot(*selectedPos);
            attackable = world.getAttackSnapshot(*selectedPos);
            // Index 0 = MOV, index 1 = ATT in tactic mode
            if (currentMode == ControllerMode::TACTIC) {
                if (actionLabels.size() > 0) enabledActions[0] = selUnit->canMove();
                if (actionLabels.size() > 1) enabledActions[1] = selUnit->canAttack();
            }
        }
    }

    // Grid
    gridView->render(world, &hoverPos, selectedPos, reachable, attackable);

    // Error overlay
    errorView->render(ERR_X, ERR_Y);

    // Mode icon row (left panel)
    static const ControllerMode MODES[] = {
        ControllerMode::TACTIC, ControllerMode::TRAINING, ControllerMode::BUILDING
    };
    for (int i = 0; i < 3; ++i) {
        const Rect& r = modeButtonSlots[i];
        Sprites::modeIcon(MODES[i], r.x, r.y, ICON_SIZE, static_cast<int>(currentMode) == i);
    }

    // Action buttons (left panel)
    DrawText("ACTIONS", ACT_X, ACT_BTN_Y-20, 14, Color{160, 160, 180, 255});
    actionView->render(actionLabels, actionButtonSlots, pendingActionIndex, enabledActions);

    // Info panel (right panel)
    informationView->render(world, &hoverPos, selectedPos);

    // END TURN button
    Color btnBg   = Color{45, 110, 55, 255};
    Color btnBdr  = Color{80, 190, 95, 255};
    DrawRectangle(endTurnButton.x, endTurnButton.y, endTurnButton.w, endTurnButton.h, btnBg);
    DrawRectangleLines(endTurnButton.x, endTurnButton.y, endTurnButton.w, endTurnButton.h, btnBdr);
    DrawText("END TURN", endTurnButton.x + 8, endTurnButton.y + 14, 16, WHITE);

    DrawFPS(10, 10);
}

bool GUI::pollEndTurn() const {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return false;
    return endTurnButton.contains(GetMouseX(), GetMouseY());
}

std::optional<ClickTarget> GUI::pollClick(const std::vector<std::string>& actionLabels) const {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return std::nullopt;
    int mx = GetMouseX(), my = GetMouseY();

    if (auto pos = pixelToTile(mx, my)) return ClickTarget{*pos};

    for (int i = 0; i < (int)actionLabels.size() && i < (int)actionButtonSlots.size(); ++i)
        if (actionButtonSlots[i].contains(mx, my)) return ClickTarget{i};

    for (int i = 0; i < (int)modeButtonSlots.size(); ++i)
        if (modeButtonSlots[i].contains(mx, my)) return ClickTarget{static_cast<ControllerMode>(i)};

    return std::nullopt;
}

std::optional<Position> GUI::pollHover() const {
    return pixelToTile(GetMouseX(), GetMouseY());
}

// Isometric inverse transform:
//   screen_x = GRID_ORIG_X + (col - row) * ISO_HALF_W
//   screen_y = GRID_ORIG_Y + (col + row) * ISO_HALF_H
//   => col - row = (screen_x - GRID_ORIG_X) / ISO_HALF_W  = ax
//   => col + row = (screen_y - GRID_ORIG_Y) / ISO_HALF_H  = ay
//   => col = floor((ax + ay) / 2),  row = floor((ay - ax) / 2)
std::optional<Position> GUI::pixelToTile(int mx, int my) const {
    auto [scrollX, scrollY] = gridView->getScrollOffset();
    float ax = (float)(mx - GRID_ORIG_X + scrollX) / (float)ISO_HALF_W;
    float ay = (float)(my - GRID_ORIG_Y + scrollY) / (float)ISO_HALF_H;
    int col = (int)std::floor((ax + ay) / 2.0f);
    int row = (int)std::floor((ay - ax) / 2.0f);
    if (row < 0 || row >= Game::HEIGHT || col < 0 || col >= Game::WIDTH)
        return std::nullopt;
    return Position(row, col);
}

void GUI::setError(PlayerError error) { errorView->setError(error); }
void GUI::clearError()                { errorView->clearError(); }
void GUI::scrollGrid(int dpx, int dpy){ gridView->scrollBy(dpx, dpy); }
