#pragma once

#include <memory>
#include <optional>
#include "controller/error.h"
#include "model/util.h"
#include "model/unit.h"
#include "model/player.h"
#include "model/economy.h"
#include "model/magic.h"

class World;

// ---------------------------------------------------------------------------
// GameCommand — Command pattern base
//
// Each concrete command encapsulates one reversible game action.
// World::applyControllerRequest() creates the appropriate command, calls
// execute(), and pushes successful commands onto commandHistory.
// World::undoLastCommand() pops the most recent command and calls undo().
// The history is cleared at the start of each new turn.
// ---------------------------------------------------------------------------

class GameCommand {
public:
    virtual ~GameCommand() = default;

    /**
     * Performs the action against the world.
     * @return  nullopt on success, PlayerError describing the failure.
     */
    virtual std::optional<PlayerError> execute(World& world) = 0;

    /**
     * Reverses the action. Only called on commands that were successfully
     * executed (i.e., execute() returned nullopt).
     */
    virtual void undo(World& world) = 0;
};


// ---------------------------------------------------------------------------
// MoveCommand
// Moves a unit from `from` to `to`. Undo moves it back and clears moved flag.
// ---------------------------------------------------------------------------

class MoveCommand : public GameCommand {
public:
    MoveCommand(Position from, Position to, Player player)
        : from(from), to(to), player(player) {}

    std::optional<PlayerError> execute(World& world) override;
    void undo(World& world) override;

private:
    Position from, to;
    Player   player;
};


// ---------------------------------------------------------------------------
// AttackCommand
// Attacks the unit at defenderPos from attackerPos.
//
// The caller MUST snapshot the defender before calling execute() (since
// execute may destroy it). Pass the defender's current HP and a full copy
// of the Unit so undo can restore it if the defender was killed.
// ---------------------------------------------------------------------------

class AttackCommand : public GameCommand {
public:
    AttackCommand(Position attackerPos, Position defenderPos, Player player,
                  int defenderHpBefore, Unit defenderSnapshot,
                  int attackerHpBefore, Unit attackerSnapshot)
        : attackerPos(attackerPos), defenderPos(defenderPos), player(player),
          defenderHpBefore(defenderHpBefore),
          defenderSnapshot(std::move(defenderSnapshot)),
          attackerHpBefore(attackerHpBefore),
          attackerSnapshot(std::move(attackerSnapshot)) {}

    std::optional<PlayerError> execute(World& world) override;
    void undo(World& world) override;

private:
    Position attackerPos, defenderPos;
    Player   player;

    // Saved state for undo
    int  defenderHpBefore;
    Unit defenderSnapshot;   // full copy captured before execute()
    int  attackerHpBefore;   // needed to undo retaliation damage
    Unit attackerSnapshot;   // full copy in case attacker dies from retaliation
};


// ---------------------------------------------------------------------------
// TrainCommand
// Trains a new unit at a city. Undo removes the trained unit and resets the
// city's trainedThisTurn flag.
// ---------------------------------------------------------------------------

class TrainCommand : public GameCommand {
public:
    TrainCommand(Position cityPos, UnitType unitType, Player player)
        : cityPos(cityPos), unitType(unitType), player(player) {}

    std::optional<PlayerError> execute(World& world) override;
    void undo(World& world) override;

private:
    Position cityPos;
    UnitType unitType;
    Player   player;
};


// ---------------------------------------------------------------------------
// ConstructCommand
// Queues a building for construction at a city. Undo removes it from the
// queue and refunds resources.
// ---------------------------------------------------------------------------

class ConstructCommand : public GameCommand {
public:
    ConstructCommand(Position tilePos, BuildingType buildingType, Player player)
        : tilePos(tilePos), buildingType(buildingType), player(player) {}

    std::optional<PlayerError> execute(World& world) override;
    void undo(World& world) override;

private:
    Position     tilePos;   // the border tile where the building will be placed
    BuildingType buildingType;
    Player       player;
};


// ---------------------------------------------------------------------------
// CastCommand
// Casts a spell from casterPos onto targetPos.
// Undo restores the caster's magic, un-exhausts the caster, and clears the
// burn if the target wasn't already burning before the cast.
// ---------------------------------------------------------------------------

class CastCommand : public GameCommand {
public:
    CastCommand(Position casterPos, Position targetPos,
                SpellId spell, Player player,
                int casterMagicBefore, bool targetWasBurning)
        : casterPos(casterPos), targetPos(targetPos),
          spell(spell), player(player),
          casterMagicBefore(casterMagicBefore),
          targetWasBurning(targetWasBurning) {}

    std::optional<PlayerError> execute(World& world) override;
    void undo(World& world) override;

private:
    Position casterPos, targetPos;
    SpellId  spell;
    Player   player;

    int  casterMagicBefore;   // saved to restore on undo
    bool targetWasBurning;    // if true, undo does not clear the burn
};
