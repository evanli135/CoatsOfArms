#pragma once

#include <optional>
#include "controller/mode_handler.h"

class World;
class Player;

/**
 * ModeHandler for TACTIC mode — handles unit movement and combat.
 *
 * Three-step interaction:
 *   Step 1 — onTileSelect() with no selection: calls selectOrigin().
 *             Requires a friendly unit that has not yet acted this turn.
 *   Step 2 — onActionButton(): choose MOV or ATT. Requires step 1 to be complete.
 *             Step 3 will be rejected until an action is chosen here.
 *   Step 3 — onTileSelect() with a selection: calls selectDestination().
 *             Dispatches the ControllerRequest to the model.
 *             Rejected with NOTSUPPORTED if no action was set in step 2.
 *
 * On a successful request, selection and pendingAction are cleared.
 * On failure, selection is preserved so the player can retry step 3.
 */
class TacticMode : public ModeHandler {
public:
    TacticMode(World& world, const Player& player);

    std::optional<PlayerError> onTileSelect(Position pos) override;
    std::optional<PlayerError> onActionButton(int index) override;
    void onDeselect() override;
    void onExit()     override;

    std::optional<Position>         getSelection()    const override { return selection; }
    std::optional<ControllerAction> getPendingAction() const override { return pendingAction; }

    /** Highlights CAST button (index 2) when a spell has been chosen and we're targeting. */
    std::optional<int> getPendingButtonIndex() const override;

    /** Returns the spell chosen in SPELL_TARGET sub-state, otherwise nullopt. */
    std::optional<SpellId> getSelectedSpell() const override;

    /**
     * In SPELL_SELECT sub-state: returns the names of available spells.
     * Otherwise returns {"MOVE", "ATTACK", "CAST"}.
     */
    std::vector<std::string> getActionLabels() const override;
    std::vector<bool>        getEnabledActions() const override;

private:
    /** Sub-states for the CAST interaction flow. */
    enum class TacticSubState {
        NORMAL,        ///< Default: MOVE / ATTACK / CAST buttons
        SPELL_SELECT,  ///< After clicking CAST: shows available spell names as buttons
        SPELL_TARGET   ///< After picking a spell: shows cast targets; CAST button highlighted
    };

    World& world;
    const Player& player;

    TacticSubState                  subState      = TacticSubState::NORMAL;
    std::optional<Position>         selection;
    std::optional<ControllerAction> pendingAction;
    std::optional<SpellId>          selectedSpell;
    std::vector<SpellId>            availableSpells;   // cached when entering SPELL_SELECT

    std::optional<PlayerError> selectOrigin(Position pos);
    std::optional<PlayerError> selectDestination(Position pos);

    /** Returns all spells the selected unit can currently cast. */
    std::vector<SpellId> computeAvailableSpells() const;
};
