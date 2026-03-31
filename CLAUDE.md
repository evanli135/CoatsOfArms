# RevRoyale — Architecture Overview

RevRoyale is a 2D turn-based strategy game written in C++17 using **Raylib** for graphics. Two players control units on a 16x16 grid, with mechanics for movement, combat, cities, and (in-progress) economy/construction.

See `claude-docs/` for deeper per-system notes.

---

## Project Layout

```
RevRoyale/
├── include/
│   ├── controller/         # Input handling interfaces
│   ├── model/              # Game logic interfaces
│   └── view/               # Rendering interfaces
├── src/
│   ├── main.cpp            # Entry point & game loop
│   ├── controller/         # Input implementations
│   ├── model/world/        # World, movement, battle systems
│   └── view/tui/           # TUI renderer
├── scripts/
│   ├── build.ps1           # g++ build script (no CMake)
│   └── play.ps1            # Run script
├── claude-docs/            # Claude's personal docs
└── bin/                    # Compiled executable
```

---

## Architecture: MVC

```
     KeyboardController ──► World ──► TUI
          (input)           (model)   (render)
              └──── ModelObserver ◄───┘
                    (turn events)
```

### Model (`include/model/`, `src/model/`)

| Class | Responsibility |
|---|---|
| `World` | Central game state: grid, players, units. Owns all systems. |
| `Tile` | One grid cell: terrain type, optional unit, optional city, buildings. |
| `Unit` | A game piece: HP, DMG, MOV, RNG, moved-flag. Created by `UnitFactory`. |
| `Player` | Thin wrapper around a player ID. |
| `City` | Owned by a player; holds building levels and upgrade state. |
| `MovementSystem` | Dijkstra pathfinding; returns reachable positions and path costs. |
| `BattleSystem` | Combat resolution; range check + damage application. |
| `WorldFactory` | Builds world layouts (currently only `BASIC`: 1v1 warrior face-off). |

### Controller (`include/controller/`, `src/controller/`)

| Class | Responsibility |
|---|---|
| `KeyboardController` | Maps raw keys → `KeyboardAction`; validates moves; writes to `World`. |
| `Controller` (mouse) | Skeletal mouse controller; partially wired. |
| `ModelObserver` | Interface; `onEvent(ModelEvent)` called on `TURN_CHANGE`. |

### View (`include/view/`, `src/view/`)

| Class | Responsibility |
|---|---|
| `TUI` | Raylib-based renderer: grid, terrain, units, info panel, controls legend. |
| `GUI` | Planned advanced renderer with sub-views; currently a skeleton. |

---

## Key Data Types

**Enums**
- `Terrain`: GRASS, FOREST, RIVER, OCEAN, MOUNTAIN
- `UnitType`: WARRIOR, SCOUT, RANGER, CAVALRY, MAGE
- `GamePhase`: PREGAME, MIDGAME, ENDGAME
- `ControllerMode`: TACTIC, TRAINING, BUILDING
- `ResourceType`: FOOD, WOOD, STONE, METAL
- `BuildingType`: FOUNDRY, BARRACK, EXTRACTOR, SHRINE, UTILITY
- `PlayerError` / `InternalError`: error codes passed through the stack

**Structs**
- `Position` (`util.h`): row/col coordinate with bounds checking; used as map key via `std::hash`.
- `UnitId`: strong-type wrapper for unit identity.

---

## Unit Stats

| Unit | HP | DMG | MOV | RNG |
|---|---|---|---|---|
| Warrior | 100 | 20 | 2 | 1 |
| Ranger | 75 | 15 | 2 | 2 |
| Scout | 50 | 10 | 3 | 1 |
| Cavalry | 80 | 25 | 4 | 1 |
| Mage | 60 | 30 | 2 | 2 |

---

## Core Algorithms

**Movement — Dijkstra (`MovementSystem`)**
- Terrain cost multipliers: Grass 1.0×, Forest 1.5×, River 2.5×, Mountain 2.0×; Ocean impassable.
- `shortestPath()`: returns total path cost.
- `getMovementSnapshot()`: returns all reachable positions within unit's MOV budget.

**Combat (`BattleSystem`)**
- Range check: Chebyshev distance ≤ attacker's RNG.
- Damage: defender HP -= attacker DMG; on 0 HP, unit removed from grid.
- Both move and attack set the unit's `moved` flag (one action per turn).

**Turn System**
- Two players alternate; `World::nextTurn()` rotates active player and resets `moved` flags.
- Broadcasts `ModelEvent::TURN_CHANGE` to all `ModelObserver`s.

---

## Game Loop (`src/main.cpp`)

```
Init Raylib (1920×1080, 60 FPS)
Create players, world (BASIC), controllers, TUI
Register controllers as observers
world.startGame()

Loop:
  poll keyboard → controller.applyAction()
  tui.render(hoverPos, selection)
  DrawFPS
  Q → break
```

---

## Build

```powershell
# scripts/build.ps1
g++ -std=c++17 src/main.cpp src/model/world/world.cpp ...
    -IC:/raylib/include -LC:/raylib/lib
    -lraylib -lopengl32 -lgdi32 -lwinmm
    -o bin/revroyale.exe
```

---

## Implementation Status

| System | Status |
|---|---|
| Grid + tiles | Done |
| Unit movement (Dijkstra) | Done |
| Combat | Done |
| Turn management | Done |
| Keyboard input + TUI | Done |
| Observer / event system | Done |
| City / building data model | Partial |
| Economy / resources | Enums only |
| Spirits / blessings | Enums only |
| Mouse controller | Skeletal |
| Advanced GUI | Skeletal |
| Construction queue | Stubbed |
| Action menu (NUM keys) | Partially wired |
