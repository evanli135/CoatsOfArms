# Controller

The controller layer sits between the view (mouse clicks, keyboard input) and the model (`World`). It owns all interaction state — what's selected, what action is pending — and translates player intent into `ControllerRequest`s that the model can execute.

Going forward the **mouse controller (`Controller`) is the primary controller**. The keyboard controller (`KeyboardController`) shares the same mode handler architecture and can coexist, but the mouse + GUI is the target input path.

---

## What was implemented

### `ClickTarget` — the unified click type (`action.h`)

```cpp
using ClickTarget = std::variant<Position, int, ControllerMode>;
```

Every mouse click resolves to exactly one of:
- `Position` — a grid tile (origin or destination, mode decides)
- `int` — a 0-based action button index (meaning is mode-specific)
- `ControllerMode` — a mode-switch button (TACTIC / TRAINING / BUILDING)

`ControllerMode` was moved into `action.h` alongside the other controller types so `ClickTarget` has no circular dependency issues.

### Mode handler architecture (`mode_handler.h`)

`ModeHandler` is a pure virtual base class. The controller owns one instance at a time and delegates every interaction to it. Modes own all selection and pending-action state — none of it leaks into the controller.

```
ModeHandler
├── onTileSelect(Position)   — phase 1 or phase 2 depending on mode state
├── onActionButton(int)      — action button or NUM key (index is mode-specific)
├── onDeselect()             — clear selection + pending action
├── onEnter() / onExit()     — lifecycle hooks on mode transition
├── getSelection()           — renderer query
└── getPendingAction()       — renderer query
```

### Three mode implementations

| Mode | File | Phase 1 | Phase 2 |
|---|---|---|---|
| `TacticMode` | `modes/tactic_mode` | Select own unit (selectOrigin) | Select destination tile (selectDestination) |
| `TrainingMode` | `modes/training_mode` | Select own city (selectOrigin) | Action button chooses unit type — **stubbed** |
| `BuildingMode` | `modes/building_mode` | Select any tile (selectOrigin) | Action button chooses building type — **stubbed** |

**TacticMode action button mapping (index → ControllerAction):**
- `0` → MOV
- `1` → ATT

On a successful request, selection and pending action are cleared. On failure, selection is kept so the player can retry.

`makeModeHandler(ControllerMode, World&, Player)` is the factory that constructs the right subclass.

### Mouse controller (`mouse.h` / `mouse.cpp`)

`Controller::onClick(ClickTarget)` dispatches the three variant arms:
- `Position` → `mode->onTileSelect(pos)`
- `int` → `mode->onActionButton(index)`
- `ControllerMode` → `switchMode(newMode)` (calls `onExit` / `onEnter`)

`switchMode` handles the full lifecycle transition. No special casing in the controller body.

### Keyboard controller (`keyboard.h` / `keyboard.cpp`)

Refactored to use the same mode handler — `waitingForActionInput`, `pendingAction`, `selectedPosition`, and `selectCell()` are all gone. `applyKeyboardAction` is now a flat switch:
- Arrow/WASD → `moveHover`
- SELECT → `mode->onTileSelect(hoverPosition)`
- UNSELECT → `mode->onDeselect()`
- CONFIRM → `endTurn()` + `model.nextTurn()`
- NUM 1–4 → `mode->onActionButton(0–3)`

---

## What still needs to be done

### 1. `GUI::pollClick()` — the missing entry point

The entire click pipeline has no way to start. The view needs a method that:
1. Checks `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)`
2. Gets the pixel position via `GetMousePosition()`
3. Hit-tests against the grid cells → returns `Position`
4. Hit-tests against action view buttons → returns `int` index
5. Hit-tests against mode switch buttons → returns `ControllerMode`
6. Returns `std::optional<ClickTarget>` (nullopt if nothing was hit)

This requires the GUI sub-views to know their own screen bounds, which means layout calculations need to be fleshed out in `GridView`, `ActionView`, and `ControlView`.

### 2. `main.cpp` — wire up `Controller` instead of `KeyboardController`

Currently `main.cpp` uses two `KeyboardController` instances. For the mouse path:
- Instantiate `Controller` for each player
- Register as model observers
- Each frame: call `controller.onClick(*gui.pollClick())` if a click occurred
- Pass `controller.getHoverPosition()` / `getSelectedPosition()` to the renderer

There is also a **bug introduced by the `getSelectedPosition()` return-type change** (now returns `optional<Position>` by value): `main.cpp` line 58 takes the address of a temporary. This needs fixing before the build compiles cleanly.

### 3. End turn

End turn is intentionally deferred. It is not part of `ClickTarget`. Options when implementing:
- A dedicated button outside the action view that calls `controller.endTurn()` + `model.nextTurn()` directly
- A global `pollEndTurn()` alongside `pollClick()`

### 4. Action view labels

The view currently has no way to know what buttons to render for the current mode. `ModeHandler` should expose something like:

```cpp
virtual std::vector<std::string> getActionLabels() const = 0;
```

So `ActionView` can ask the active mode for its button list and render them dynamically, rather than hardcoding labels.

### 5. `TrainingMode` and `BuildingMode` action handling

Both `onActionButton` implementations return `NOTSUPPORTED`. These need real logic once the economy/construction systems are implemented in the model.

### 6. Mode switch button rendering

The mode buttons (TACTIC / TRAINING / BUILDING) need to be rendered somewhere in the GUI (likely `ControlView`) and their bounding boxes registered so `pollClick()` can return the correct `ControllerMode` variant when clicked.
