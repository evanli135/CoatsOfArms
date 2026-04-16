#include "controller/modes/tactic_mode.h"
#include "model/world.h"
#include "model/player.h"
#include "model/unit.h"
#include "model/magic.h"

TacticMode::TacticMode(World& world, const Player& player)
    : world(world), player(player) {}

// ---------------------------------------------------------------------------
// computeAvailableSpells — spells the selected unit can currently cast
// ---------------------------------------------------------------------------

std::vector<SpellId> TacticMode::computeAvailableSpells() const {
    if (!selection.has_value()) return {};
    const Unit* u = world.getUnitAt(*selection);
    if (!u) return {};

    // Only spells unlocked through shrine blessings are available.
    // A player earns a spell by receiving an isMagic blessing targeting this unit type.
    std::vector<SpellId> result;
    for (const Blessing& b : world.getPlayerBlessings(player.getId())) {
        if (!b.isMagic) continue;
        if (b.targetUnit != u->getType()) continue;

        // Map BlessingEffect index directly to SpellId (1-to-1 by enum order).
        SpellId spell = static_cast<SpellId>(static_cast<int>(b.effect));

        // Only include if the unit has enough magic to cast it right now.
        if (world.canCast(*selection, spell, player))
            result.push_back(spell);
    }
    return result;
}

// ---------------------------------------------------------------------------
// onTileSelect
// ---------------------------------------------------------------------------

std::optional<PlayerError> TacticMode::onTileSelect(Position pos) {
    if (!selection.has_value()) return selectOrigin(pos);

    // During spell selection, tile clicks are silently ignored — must pick a spell first
    if (subState == TacticSubState::SPELL_SELECT) return std::nullopt;

    return selectDestination(pos);
}

// Phase 1: choose a friendly unit to act with.
std::optional<PlayerError> TacticMode::selectOrigin(Position pos) {
    if (!world.hasUnitAt(pos))                  return PlayerError::INVALIDTARGET;
    const Unit* unit = world.getUnitAt(pos);
    if (!unit->sameOwner(player))               return PlayerError::INVALIDTARGET;
    if (!unit->canMove() && !unit->canAttack()) return PlayerError::UNITCANTMOVE;

    selection = pos;
    subState  = TacticSubState::NORMAL;
    return std::nullopt;
}

// Phase 2: choose a destination tile.
// Auto-infers MOV or ATT when no action has been explicitly chosen.
std::optional<PlayerError> TacticMode::selectDestination(Position pos) {
    bool autoInferred = !pendingAction.has_value();

    if (autoInferred) {
        if (world.hasUnitAt(pos)) {
            const Unit* target = world.getUnitAt(pos);
            if (target && !target->sameOwner(player)) {
                pendingAction = ControllerAction::ATT;
            } else {
                // Friendly unit — deselect current, re-select if different
                bool sameUnit = selection.has_value() && (pos == *selection);
                selection.reset();
                pendingAction.reset();
                subState      = TacticSubState::NORMAL;
                selectedSpell = std::nullopt;
                if (!sameUnit) return selectOrigin(pos);
                return std::nullopt;
            }
        } else {
            pendingAction = ControllerAction::MOV;
        }
    }

    // For CAST, bundle the selected spell into the request
    std::optional<SpellId> reqSpell;
    if (pendingAction == ControllerAction::CAST)
        reqSpell = selectedSpell;

    auto result = world.applyControllerRequest(
        ControllerRequest(pendingAction.value(), selection.value(), pos, player, reqSpell));

    if (!result.has_value()) {
        bool wasMov = (pendingAction == ControllerAction::MOV);
        pendingAction.reset();
        subState      = TacticSubState::NORMAL;
        selectedSpell = std::nullopt;

        Position unitPos = (wasMov && selection.has_value()) ? pos : selection.value_or(pos);
        const Unit* unit = world.getUnitAt(unitPos);
        if (!unit || unit->isExhausted()) {
            selection.reset();
        } else {
            selection = unitPos;
        }
    } else if (autoInferred) {
        pendingAction.reset();
    }

    return result;
}

// ---------------------------------------------------------------------------
// onActionButton
// ---------------------------------------------------------------------------

std::optional<PlayerError> TacticMode::onActionButton(int index) {
    if (!selection.has_value()) return PlayerError::INVALIDTARGET;

    // ── SPELL_SELECT sub-state: buttons are spell names ──────────────────────
    if (subState == TacticSubState::SPELL_SELECT) {
        if (index < 0 || index >= (int)availableSpells.size())
            return PlayerError::NOTSUPPORTED;
        selectedSpell = availableSpells[index];
        subState      = TacticSubState::SPELL_TARGET;
        pendingAction = ControllerAction::CAST;
        return std::nullopt;
    }

    // ── NORMAL or SPELL_TARGET: standard action buttons ──────────────────────
    switch (index) {
        case 0:
            subState      = TacticSubState::NORMAL;
            selectedSpell = std::nullopt;
            pendingAction = ControllerAction::MOV;
            return std::nullopt;

        case 1:
            subState      = TacticSubState::NORMAL;
            selectedSpell = std::nullopt;
            pendingAction = ControllerAction::ATT;
            return std::nullopt;

        case 2: {
            // CAST pressed: enter spell selection (or back to it from SPELL_TARGET)
            availableSpells = computeAvailableSpells();
            if (availableSpells.empty()) return PlayerError::NOTSUPPORTED;
            subState      = TacticSubState::SPELL_SELECT;
            selectedSpell = std::nullopt;
            pendingAction = std::nullopt;
            return std::nullopt;
        }

        default:
            return PlayerError::NOTSUPPORTED;
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void TacticMode::onDeselect() {
    selection.reset();
    pendingAction.reset();
    selectedSpell = std::nullopt;
    subState      = TacticSubState::NORMAL;
    availableSpells.clear();
}

void TacticMode::onExit() {
    onDeselect();
}

// ---------------------------------------------------------------------------
// Getters exposed to the view
// ---------------------------------------------------------------------------

std::optional<int> TacticMode::getPendingButtonIndex() const {
    if (subState == TacticSubState::SPELL_TARGET) return 2;   // CAST button highlighted
    return std::nullopt;
}

std::optional<SpellId> TacticMode::getSelectedSpell() const {
    if (subState == TacticSubState::SPELL_TARGET) return selectedSpell;
    return std::nullopt;
}

std::vector<std::string> TacticMode::getActionLabels() const {
    if (subState == TacticSubState::SPELL_SELECT) {
        // Show the name of each castable spell
        std::vector<std::string> labels;
        for (SpellId s : availableSpells)
            labels.push_back(World::getSpellDef(s).name);
        return labels;
    }
    return {"MOVE", "ATTACK", "CAST"};
}

std::vector<bool> TacticMode::getEnabledActions() const {
    if (subState == TacticSubState::SPELL_SELECT) {
        // All displayed spells are castable (we pre-filtered); all enabled
        return {};
    }
    if (!selection.has_value()) return {};
    const Unit* u = world.getUnitAt(*selection);
    if (!u) return {};

    bool hasCast = !computeAvailableSpells().empty();
    return { u->canMove(), u->canAttack(), hasCast };
}
