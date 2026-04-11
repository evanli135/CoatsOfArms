# Graphics Skills — RevRoyale Rendering Guide for Agents

This document describes how graphics code is written in RevRoyale. Follow these patterns when adding or modifying any rendering code.

---

## Rendering Stack at a Glance

```
GUI (orchestrator)
 ├── GridView          — isometric game board, all tile layers
 ├── ActionView        — left panel action buttons
 ├── InformationView   — right panel tile/unit/combat info
 ├── ErrorView         — centered error overlay
 ├── DamageIndicatorSystem  — rising damage numbers (particle)
 └── ExplosionSystem        — particle burst on unit death
```

`GUI::render()` is the single entry point called each frame. Sub-views are owned as pointers; they do not know about each other.

---

## Coordinate Systems

Three spaces exist. Never mix them.

| Space | Description | Example |
|---|---|---|
| Model space | Grid tile `(row, col)`, 0-based | `Position{3, 5}` |
| Isometric screen space | Pixel position after projection | `px=960, py=300` |
| UI space | Fixed-position panel rects | `Rect{30, 200, 380, 40}` |

**Tile → Pixel (forward transform):**
```cpp
int px = Layout::GRID_ORIG_X + (col - row) * Layout::ISO_HALF_W - scrollX;
int py = Layout::GRID_ORIG_Y + (col + row) * Layout::ISO_HALF_H - scrollY;
```

**Pixel → Tile (inverse transform):**
```cpp
float ax = (mx - Layout::GRID_ORIG_X + scrollX) / (float)Layout::ISO_HALF_W;
float ay = (my - Layout::GRID_ORIG_Y + scrollY) / (float)Layout::ISO_HALF_H;
col = (int)floor((ax + ay) / 2.0f);
row = (int)floor((ay - ax) / 2.0f);
```

Constants live in `include/view/layout.h`. Always use them — never hard-code pixel offsets.

---

## Painter's Algorithm — Tile Draw Order

Isometric depth ordering is achieved by drawing tiles in ascending `row + col` sum order:

```cpp
for (int sum = 0; sum < Game::HEIGHT + Game::WIDTH - 1; ++sum) {
    for (int row = ...; row >= 0; --row) {
        int col = sum - row;
        // render tile at (row, col)
    }
}
```

Tiles with a larger `row + col` are visually closer to the camera. **Never render tiles in row-major or column-major order** — it breaks depth.

---

## Per-Tile Layer Order

Every tile renders up to 8 layers in this order (bottom to top):

1. `renderTerrainLayer` — terrain diamond
2. `renderCityLayer` — castle sprite (if city present)
3. `renderUnitLayer` — unit sprite + HP + animations
4. `renderReachableLayer` — blue rings/arrows (movement overlay)
5. `renderAttackableLayer` — red outline
6. `renderLethalLayer` — crossing swords
7. `renderHoverLayer` — yellow diamond + corner ticks
8. `renderSelectionLayer` — pulsing cyan + bracket marks

**Rule:** New tile overlays always go as a new named layer method in `GridView`, inserted at the correct depth in this stack.

---

## Sprite Drawing — `Sprites::` Namespace

All sprite drawing lives in the `Sprites::` namespace (`include/view/sprites.h`). Functions are stateless — they only accept position and type parameters, then immediately issue Raylib draw calls.

```cpp
// Terrain diamond
Sprites::terrain(Terrain::GRASS, px, py);

// Unit sprite with faction tint
Sprites::unit(UnitType::WARRIOR, px, py, playerColor(ownerId));

// City with faction color
Sprites::city(px, py, playerColor(ownerId));

// Mode icon (left panel)
Sprites::modeIcon(ControllerMode::TACTIC, ix, iy, 44, isActive);
```

**Rules:**
- Sprite functions must have no side effects and no internal state.
- Inputs are always value types (`int`, `Color`, enum). Never pass `World&` into a sprite function.
- All geometry is procedural (Raylib primitives). Texture loading is handled separately in `GridView` with `std::optional<Texture2D>` as an override.

### Adding a New Sprite

1. Add the declaration to `include/view/sprites.h`.
2. Implement in a dedicated `.cpp` in `src/view/` (e.g., `src/view/my_sprite.cpp`).
3. Build up the shape from Raylib primitives: `DrawPoly`, `DrawRectangle`, `DrawCircle`, `DrawTriangle`, `DrawLine`, etc.
4. Use `px, py` as the anchor (top-center of the isometric diamond — `y` offset downward for "height").
5. Add the new `.cpp` to `scripts/build.ps1`.

---

## Animation Conventions

Animations are driven by `GetTime()` (wall clock, seconds). No per-object frame counters.

```cpp
float t = (float)GetTime();
float pulse = 0.5f + 0.5f * sinf(t * freq * 2.0f * PI);  // 0.0 → 1.0
```

| Animation | Frequency | Notes |
|---|---|---|
| Selection highlight pulse | 4.0 Hz | Cyan alpha |
| Ready-unit glow | 3.0 Hz | Sine 0.5–1.0 |
| Ready-unit chevron bounce | 4.0 Hz | ±2 px vertical |
| Reachable ring fall | 1.4 Hz | Per-ring phase offset |
| Lethal swords cross | 1.7 Hz | 6.0 Hz pulse on top |
| Action button glow | 5.0 Hz | Green glow when active |

**Rule:** If you need an offset between multiple instances of the same animation (e.g., three falling rings), use a phase offset — `sinf(t * freq * TWO_PI + phaseOffset)`.

---

## Effect Systems

Effects (damage numbers, explosions) are managed by standalone systems that own their particle lists.

### Pattern

```cpp
struct MyParticle {
    float x, y;
    float age;       // seconds since spawn
    Color color;
    // ... other per-particle state
};

class MyEffectSystem {
    std::vector<MyParticle> particles;
public:
    void spawn(float x, float y, ...);
    void update(float dt);   // advance age, apply physics, cull dead
    void render() const;     // draw all live particles
};
```

- `update(dt)` removes particles whose `age >= duration`.
- `render()` maps `age / duration` to alpha and size.
- The system is updated and rendered by `GUI` each frame, after `GridView`.
- Systems are triggered from `GUI::onModelChanged()` — listen for `ModelEvent` variants and call `spawn()`.

### Trigger via Observer

```cpp
void GUI::onModelChanged(const ModelEvent& event) {
    std::visit([&](auto&& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, DamageDealtEvent>) {
            auto [px, py] = tileToPixel(e.pos);
            damageSystem.spawn(px, py - 20, "-" + std::to_string(e.amount), RED);
        }
        if constexpr (std::is_same_v<T, UnitDiedEvent>) {
            auto [px, py] = tileToPixel(e.pos);
            explosionSystem.spawn(px, py, {255, 140, 30, 255});
        }
    }, event);
}
```

---

## Color Conventions

```cpp
// Faction colors (4 players)
Color playerColor(int id);   // id 0-3

// Shade a color darker
Color Sprites::darken(Color c, float f);  // f: 0.0 (black) → 1.0 (unchanged)
```

| Usage | Color |
|---|---|
| Background clear | `{16, 16, 26, 255}` — dark navy |
| Hover highlight | Yellow |
| Selection highlight | Cyan (pulsing alpha) |
| Reachable overlay | Blue |
| Attackable overlay | Red outline |
| Lethal overlay | Red + animated swords |
| Active action button | Bright green `{80, 220, 80, 255}` |
| Error overlay | Dark red |
| Damage indicator | `{255, 80, 80, 255}` |
| Explosion base | `{255, 140, 30, 255}` |

Exhausted units get a gray tint. Enemy units get a dimmed tint. Apply tint via `playerColor()` passed into `Sprites::unit()`.

---

## UI Layout

Fixed constants in `include/view/layout.h`. Use them directly — never compute panel positions by hand.

| Region | Key constants |
|---|---|
| Screen | `SCREEN_W = 1920`, `SCREEN_H = 1080` |
| Grid origin | `GRID_ORIG_X = 960`, `GRID_ORIG_Y = 220` |
| Left panel | `ACTION_BTN_X = 30`, buttons `140×40`, 8px gap |
| Right panel | `INFO_PANEL_X = 1555`, `INFO_PANEL_W = 365` |
| Mode icons | `44×44 px`, `8px gap`, at `MODE_BTN_Y` |
| End turn btn | `164×48 px` |

Hit-testing uses `Rect::contains(px, py)` — a simple AABB check. Always define clickable areas as `Rect` structs stored in `GUI` or the relevant sub-view.

---

## Data Flow Rules

- `GUI::render()` receives `const World&` — it never mutates model state.
- `GUI` extracts movement/attack snapshots and passes them down to `GridView`. Sub-views never call `World` methods directly.
- Sub-views receive only what they need — no sub-view ever holds a reference to `World`.
- `ErrorView` state is set separately via `GUI::setError()` / `GUI::clearError()`, not through `render()`.

---

## Adding a New Sub-View (Checklist)

1. Create `include/view/my_view.h` — declare the class, `render()`, and any setters.
2. Create `src/view/my_view.cpp` — implement rendering with Raylib primitives.
3. Add `MyView* myView` to `GUI` members.
4. Instantiate in `GUI` constructor; delete in destructor.
5. Call `myView->render(...)` from `GUI::render()` at the correct z-order position.
6. Add `src/view/my_view.cpp` to `scripts/build.ps1`.

---

## Raylib Primitive Reference (Quick)

```cpp
DrawRectangle(x, y, w, h, color)
DrawRectangleLines(x, y, w, h, color)
DrawRectangleRec(rect, color)
DrawCircle(cx, cy, radius, color)
DrawCircleLines(cx, cy, radius, color)
DrawTriangle(v1, v2, v3, color)   // counter-clockwise winding
DrawPoly(center, sides, radius, rotation, color)
DrawLine(x1, y1, x2, y2, color)
DrawLineEx(start, end, thick, color)
DrawText(text, x, y, size, color)
DrawTextEx(font, text, pos, size, spacing, color)
GetTime()   // seconds since init (float)
GetFrameTime()  // dt for this frame
```

Textures:
```cpp
Texture2D tex = LoadTexture("path/to/file.png");
DrawTexture(tex, x, y, WHITE);       // WHITE tint = no tint
DrawTextureEx(tex, pos, rot, scale, tint);
UnloadTexture(tex);  // always unload in destructor
```
