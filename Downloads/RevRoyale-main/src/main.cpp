#include <iostream>
#include <raylib.h>
#include "model/util.h"
#include "model/world.h"
#include "controller/action.h"
#include "controller/error.h"
#include "controller/mouse.h"
#include "view/gui.h"
#include "view/fog.h"


int main() {
    InitWindow(1920, 1080, "RevRoyale");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    MaximizeWindow();
    SetTargetFPS(60);

    Player p1 = Player(0);
    Player p2 = Player(1);

    std::vector<Player> players = {p1, p2};

    World model = WorldFactory::create(WorldLayout::BASIC, players);
    Controller controller1(model, p1);
    Controller controller2(model, p2);
    GUI view(GetScreenWidth(), GetScreenHeight());

    model.addObserver(&controller1);
    model.addObserver(&controller2);
    model.addObserver(&view);
    model.startGame();

    Position keyHover(0, 0);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_Q)) break;

        Controller& active = controller1.isMyTurn() ? controller1 : controller2;

        // --- Arrow key grid scroll (continuous) ---
        const int SCROLL_SPEED = 8;
        if (IsKeyDown(KEY_LEFT))  view.scrollGrid(-SCROLL_SPEED, 0);
        if (IsKeyDown(KEY_RIGHT)) view.scrollGrid( SCROLL_SPEED, 0);
        if (IsKeyDown(KEY_UP))    view.scrollGrid(0, -SCROLL_SPEED);
        if (IsKeyDown(KEY_DOWN))  view.scrollGrid(0,  SCROLL_SPEED);

        // --- Click-drag map pan (must run before hover / click) ---
        view.pollMapPan();
        view.pollMapZoom();

        // --- Mouse hover (syncs keyHover so WASD continues from mouse cursor) ---
        if (auto pos = view.pollHover()) {
            active.setHoverPosition(*pos);
            keyHover = *pos;
        }

        // --- WASD keyboard cursor (overrides mouse hover on key press) ---
        auto tryMoveKb = [&](int dr, int dc) {
            try { keyHover.move(dr, dc); active.setHoverPosition(keyHover); }
            catch (const std::out_of_range&) {}
        };
        if (IsKeyPressed(KEY_W)) tryMoveKb(-1,  0);
        if (IsKeyPressed(KEY_S)) tryMoveKb( 1,  0);
        if (IsKeyPressed(KEY_A)) tryMoveKb( 0, -1);
        if (IsKeyPressed(KEY_D)) tryMoveKb( 0,  1);

        // --- Keyboard game actions ---
        auto applyResult = [&](std::optional<PlayerError> err) {
            if (err) view.setError(*err); else view.clearError();
        };

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if (auto hp = active.getHoverPosition()) {
                applyResult(active.onClick(ClickTarget{*hp}));
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE))
            active.onRightClick();
        if (IsKeyPressed(KEY_LEFT_SHIFT))
            applyResult(active.onEndTurn());
        if (IsKeyPressed(KEY_ONE))   applyResult(active.onClick(ClickTarget{0}));
        if (IsKeyPressed(KEY_TWO))   applyResult(active.onClick(ClickTarget{1}));
        if (IsKeyPressed(KEY_THREE)) applyResult(active.onClick(ClickTarget{2}));
        if (IsKeyPressed(KEY_FOUR))  applyResult(active.onClick(ClickTarget{3}));
        if (IsKeyPressed(KEY_Z))     applyResult(active.onUndo());
        if (IsKeyPressed(KEY_F))     Debug::fogEnabled = !Debug::fogEnabled;

        // --- END TURN button ---
        if (view.pollEndTurn())
            applyResult(active.onEndTurn());

        // --- Left click ---
        if (auto click = view.pollClick(active.getActionLabels())) {
            auto error = active.onClick(*click);
            if (error) {
                std::cout << "Action Failed: " << playerErrorToString(*error) << "\n";
                view.setError(*error);
            } else {
                view.clearError();
            }
        }

        // --- Right click — deselect / cancel ---
        if (pollMouseRightClick())
            active.onRightClick();

        BeginDrawing();

        // Map pending ControllerAction → button index for the active-button highlight
        int pendingIdx = -1;
        if (auto pa = active.getPendingAction()) {
            switch (*pa) {
                case ControllerAction::MOV: pendingIdx = 0; break;
                case ControllerAction::ATT: pendingIdx = 1; break;
                default: break;
            }
        }
        // Mode-specific pending button (e.g. training mode button-first flow)
        if (pendingIdx == -1) {
            if (auto pb = active.getPendingButtonIndex())
                pendingIdx = *pb;
        }

        auto selPos = active.getSelectedPosition();
        view.render(
            model,
            active.getHoverPosition().value_or(Position(0, 0)),
            selPos.has_value() ? &selPos.value() : nullptr,
            active.getActionLabels(),
            active.getCurrentMode(),
            pendingIdx
        );

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
