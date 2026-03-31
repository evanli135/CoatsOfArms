#pragma once

#include <variant>
#include "model/util.h"
#include "model/unit.h"

// ---------------------------------------------------------------------------
// Typed model events
//
// Each event carries the data observers actually need, so they don't have to
// call back into the model to find out what changed.
// ---------------------------------------------------------------------------

/** The active player has changed. Fired by startGame() and nextTurn(). */
struct TurnChangeEvent {
    int turn;
    int newPlayerId;
};

/** A unit successfully moved from one tile to another. */
struct UnitMovedEvent {
    UnitId   unitId;
    Position from;
    Position to;
};

/** A unit's HP reached zero and it was removed from the grid. */
struct UnitDiedEvent {
    UnitId   unitId;
    Position pos;
};

/**
 * Variant over all possible model events.
 * Use std::visit (or std::get_if) in onModelChanged() to handle only the
 * event types you care about and ignore the rest.
 */
using ModelEvent = std::variant<TurnChangeEvent, UnitMovedEvent, UnitDiedEvent>;


// ---------------------------------------------------------------------------
// Observer interface
// ---------------------------------------------------------------------------

class ModelObserver {
public:
    virtual ~ModelObserver() = default;

    /**
     * Called by World::notifyObservers() whenever a model event fires.
     * Implementations should std::visit the variant and respond only to
     * relevant event types.
     */
    virtual void onModelChanged(const ModelEvent& event) = 0;
};
