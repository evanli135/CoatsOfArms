#include "view/gui.h"
#include "view/grid_view.h"
#include "view/panel_views.h"
#include "view/sprites.h"
#include "view/layout.h"
#include "model/util.h"
#include "raylib.h"
#include <cmath>
#include <cstdio>
#include <unordered_set>

using namespace Layout;

// ---------------------------------------------------------------------------
// GUI
// ---------------------------------------------------------------------------

void GUI::updateLayout() {
    screenWidth  = GetScreenWidth();
    screenHeight = GetScreenHeight();
    frameLayout_ = makeViewLayout(screenWidth, screenHeight);

    int sminX, smaxX, sminY, smaxY;
    gridScrollBounds(frameLayout_,
                     LEFT_UI_RESERVE, frameLayout_.infoPanelX,
                     TITLE_TOP_MARGIN, frameLayout_.screenH - BOTTOM_MARGIN,
                     sminX, smaxX, sminY, smaxY);
    gridView->applyScrollBounds(sminX, smaxX, sminY, smaxY);

    modeButtonSlots.clear();
    for (int i = 0; i < 3; ++i)
        modeButtonSlots.push_back(Rect{
            frameLayout_.actX + i * (ICON_SIZE + BTN_GAP),
            frameLayout_.modeBtnY, ICON_SIZE, ICON_SIZE});

    actionButtonSlots.clear();
    for (int i = 0; i < 8; ++i)
        actionButtonSlots.push_back(Rect{
            frameLayout_.actX,
            frameLayout_.actBtnY + i * (BTN_H + BTN_GAP),
            BTN_W, BTN_H});

    endTurnButton = Rect{
        frameLayout_.actX,
        frameLayout_.actBtnY + 8 * (BTN_H + BTN_GAP) + 16,
        BTN_W + 24, BTN_H + 8};
}

GUI::GUI(int width, int height)
    : screenWidth(width), screenHeight(height)
{
    errorView       = new ErrorView();
    informationView = new InformationView();
    actionView      = new ActionView();
    gridView        = new GridView();
    updateLayout();
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
    updateLayout();

    ClearBackground(Color{16, 16, 26, 255});

    // Title bar
    int cp = world.getCurrentPlayer().getId();
    DrawText(TextFormat("Player %d  |  Turn %d", cp+1, world.getTurn()),
             screenWidth/2 - 80, 16, 16, playerColor(cp));

    // Compute movement reach and attackable tiles for the selected unit
    std::vector<Position> reachable;
    std::vector<Position> attackable;
    std::vector<Position> lethal;
    std::vector<Position> path;
    std::vector<bool>     enabledActions(actionLabels.size(), true);

    if (selectedPos && world.hasUnitAt(*selectedPos)) {
        const Unit* selUnit = world.getUnitAt(*selectedPos);
        if (selUnit) {
            reachable  = world.getMovementSnapshot(*selectedPos);
            attackable = world.getAttackSnapshot(*selectedPos);
            // Mark which attackable tiles are one-shot kills
            for (const auto& apos : attackable) {
                if (world.getCombatForecast(*selectedPos, apos).lethal)
                    lethal.push_back(apos);
            }
            // Path from selected unit to hovered tile (only when hovering a reachable tile)
            {
                std::unordered_set<Position> reachSet(reachable.begin(), reachable.end());
                if (reachSet.count(hoverPos) && hoverPos != *selectedPos)
                    path = world.getPath(*selectedPos, hoverPos);
            }
            // Index 0 = MOV, index 1 = ATT in tactic mode
            if (currentMode == ControllerMode::TACTIC) {
                if (actionLabels.size() > 0) enabledActions[0] = selUnit->canMove();
                if (actionLabels.size() > 1) enabledActions[1] = selUnit->canAttack();
            }
        }
    }

    // Grid
    gridView->render(frameLayout_, world, &hoverPos, selectedPos, reachable, attackable, lethal, path);

    // Damage indicators + explosions (above grid, below UI panels)
    damageIndicators.update(GetFrameTime());
    damageIndicators.render();
    explosions.update(GetFrameTime());
    explosions.render();

    // Error overlay
    errorView->render(frameLayout_.errX, frameLayout_.errY);

    // Mode icon row (left panel)
    static const ControllerMode MODES[] = {
        ControllerMode::TACTIC, ControllerMode::TRAINING, ControllerMode::BUILDING
    };
    for (int i = 0; i < 3; ++i) {
        const Rect& r = modeButtonSlots[i];
        Sprites::modeIcon(MODES[i], r.x, r.y, ICON_SIZE, static_cast<int>(currentMode) == i);
    }

    // Action buttons (left panel)
    DrawText("ACTIONS", frameLayout_.actX, frameLayout_.actBtnY - 20, 14, Color{160, 160, 180, 255});
    actionView->render(actionLabels, actionButtonSlots, pendingActionIndex, enabledActions);

    // Info panel (right panel)
    informationView->render(world, &hoverPos, selectedPos,
                              frameLayout_.infoPanelX, frameLayout_.infoPanelW, frameLayout_.screenH);

    // END TURN button
    bool allDone = world.allUnitsExhausted();
    {
        const float t     = (float)GetTime();
        const float pulse = 0.5f + 0.5f * sinf(t * 4.0f);

        Color btnBg, btnBdr, textCol;
        if (allDone) {
            // Urgent gold/amber pulse — nothing left to do, prompt end turn
            unsigned char gr = (unsigned char)(160 + 55 * pulse);
            unsigned char gg = (unsigned char)(110 + 40 * pulse);
            btnBg  = Color{gr, gg, 20, 255};
            btnBdr = Color{255, (unsigned char)(200 + 55 * pulse), 60, 255};
            textCol = Color{255, 255, 200, 255};

            // Outer glow rings
            unsigned char ga = (unsigned char)(60 + 80 * pulse);
            DrawRectangleLines(endTurnButton.x - 2, endTurnButton.y - 2,
                               endTurnButton.w + 4, endTurnButton.h + 4,
                               Color{255, 220, 60, ga});
            DrawRectangleLines(endTurnButton.x - 4, endTurnButton.y - 4,
                               endTurnButton.w + 8, endTurnButton.h + 8,
                               Color{255, 200, 40, (unsigned char)(ga / 2)});
        } else {
            btnBg   = Color{45, 110, 55, 255};
            btnBdr  = Color{80, 190, 95, 255};
            textCol = WHITE;
        }

        DrawRectangle(endTurnButton.x, endTurnButton.y, endTurnButton.w, endTurnButton.h, btnBg);
        DrawRectangleLines(endTurnButton.x, endTurnButton.y, endTurnButton.w, endTurnButton.h, btnBdr);
        DrawText("END TURN", endTurnButton.x + 8, endTurnButton.y + 8, 15, textCol);
        // Hotkey label
        DrawText("[SHIFT]", endTurnButton.x + 8, endTurnButton.y + 26, 11,
                 Color{180, 210, 180, 180});
    }

    // Turn-change banner
    turnBannerAge += GetFrameTime();
    if (turnBannerAge < BANNER_DURATION) {
        float t   = turnBannerAge / BANNER_DURATION;   // 0→1 over lifetime
        // Fade: quick in (first 15%), hold, then fade out (last 30%)
        float alpha;
        if      (t < 0.15f) alpha = t / 0.15f;
        else if (t < 0.70f) alpha = 1.0f;
        else                alpha = 1.0f - (t - 0.70f) / 0.30f;
        alpha = std::max(0.0f, std::min(1.0f, alpha));
        unsigned char a = (unsigned char)(alpha * 255.0f);

        const Color pc      = playerColor(turnBannerPlayerId);
        const int   bw      = 520, bh = 90;
        const int   bx      = screenWidth  / 2 - bw / 2;
        const int   by      = screenHeight / 2 - bh / 2;

        // Dark backing panel
        DrawRectangle(bx, by, bw, bh, Color{10, 10, 20, (unsigned char)(a * 0.88f)});
        // Coloured top/bottom border strips
        DrawRectangle(bx, by,      bw, 4, Color{pc.r, pc.g, pc.b, a});
        DrawRectangle(bx, by+bh-4, bw, 4, Color{pc.r, pc.g, pc.b, a});
        // Outer border
        DrawRectangleLines(bx, by, bw, bh, Color{pc.r, pc.g, pc.b, a});

        // Player label (coloured)
        const char* pLabel = TextFormat("PLAYER %d", turnBannerPlayerId + 1);
        int pw = MeasureText(pLabel, 22);
        DrawText(pLabel, screenWidth/2 - pw/2, by + 14, 22,
                 Color{pc.r, pc.g, pc.b, a});

        // "YOUR TURN" text
        const char* tLabel = "YOUR TURN";
        int tw = MeasureText(tLabel, 36);
        DrawText(tLabel, screenWidth/2 - tw/2, by + 40, 36,
                 Color{240, 240, 255, a});
    }

    DrawFPS(10, 10);
}

bool GUI::pollEndTurn() {
    updateLayout();
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return false;
    return endTurnButton.contains(GetMouseX(), GetMouseY());
}

std::optional<ClickTarget> GUI::pollClick(const std::vector<std::string>& actionLabels) {
    updateLayout();
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return std::nullopt;
    int mx = GetMouseX(), my = GetMouseY();

    if (auto pos = pixelToTile(mx, my)) return ClickTarget{*pos};

    for (int i = 0; i < (int)actionLabels.size() && i < (int)actionButtonSlots.size(); ++i)
        if (actionButtonSlots[i].contains(mx, my)) return ClickTarget{i};

    for (int i = 0; i < (int)modeButtonSlots.size(); ++i)
        if (modeButtonSlots[i].contains(mx, my)) return ClickTarget{static_cast<ControllerMode>(i)};

    return std::nullopt;
}

std::optional<Position> GUI::pollHover() {
    updateLayout();
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
    float ax = (float)(mx - frameLayout_.gridOrigX + scrollX) / (float)ISO_HALF_W;
    float ay = (float)(my - frameLayout_.gridOrigY + scrollY) / (float)ISO_HALF_H;
    int col = (int)std::floor((ax + ay) / 2.0f);
    int row = (int)std::floor((ay - ax) / 2.0f);
    if (row < 0 || row >= Game::HEIGHT || col < 0 || col >= Game::WIDTH)
        return std::nullopt;
    return Position(row, col);
}

void GUI::setError(PlayerError error) { errorView->setError(error); }
void GUI::clearError()                { errorView->clearError(); }
void GUI::scrollGrid(int dpx, int dpy){ gridView->scrollBy(dpx, dpy); }

std::pair<int,int> GUI::tileToPixel(const Position& pos) const {
    auto [scrollX, scrollY] = gridView->getScrollOffset();
    int px = frameLayout_.gridOrigX + (pos.col() - pos.row()) * ISO_HALF_W - scrollX;
    int py = frameLayout_.gridOrigY + (pos.col() + pos.row()) * ISO_HALF_H - scrollY;
    return {px, py};
}

void GUI::onModelChanged(const ModelEvent& event) {
    std::visit([&](auto&& e) {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, DamageDealtEvent>) {
            auto [px, py] = tileToPixel(e.targetPos);
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%d", e.damage);
            float jitter = (float)((px * 1234567 + py * 7654321) % 21) - 10.0f;
            damageIndicators.spawn(buf,
                (float)px + jitter,
                (float)(py - ISO_HALF_H - 8),
                Color{255, 80, 80, 255});
        }

        if constexpr (std::is_same_v<T, TurnChangeEvent>) {
            turnBannerAge      = 0.0f;
            turnBannerPlayerId = e.newPlayerId;
        }

        if constexpr (std::is_same_v<T, UnitDiedEvent>) {
            auto [px, py] = tileToPixel(e.pos);
            // Centre the burst on the unit sprite (shifted up from tile top)
            explosions.spawn((float)px, (float)(py - ISO_HALF_H / 2),
                             Color{255, 140, 30, 255});
        }
    }, event);
}
