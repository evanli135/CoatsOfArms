#include "model/magic.h"
#include "model/world.h"
#include "model/unit.h"
#include <algorithm>

// ---------------------------------------------------------------------------
// Spell definitions — one entry per SpellId (order must match the enum).
// ---------------------------------------------------------------------------

static const SpellDef SPELL_DEFS[] = {
    // name            description                                          cost  requiredBlessing
    { "Sear",         "Apply a burning debuff to the target unit",           8,  BlessingEffect::FLAME_SEAR         },
    { "Blaze Aura",   "Nearby allies deal bonus fire damage this turn",     10,  BlessingEffect::FLAME_BLAZE_AURA   },
    { "Ignite",       "Gain extra movement speed after the next kill",       6,  BlessingEffect::FLAME_IGNITE       },

    { "Veil",         "Enter magical concealment until end of turn",         7,  BlessingEffect::SHADOW_VEIL        },
    { "Shadow Step",  "Reposition to a new tile after defeating an enemy",   9,  BlessingEffect::SHADOW_STEP        },
    { "Dark Strike",  "Next attack bypasses a portion of enemy defense",     8,  BlessingEffect::SHADOW_DARK_STRIKE },

    { "Swiftness",    "Gain +1 movement range this turn",                    6,  BlessingEffect::GALE_SWIFTNESS     },
    { "Far Reach",    "Gain +1 attack range this turn",                      6,  BlessingEffect::GALE_FAR_REACH     },
    { "Tumble",       "May move again after attacking this turn",            10,  BlessingEffect::GALE_TUMBLE        },

    { "Iron Skin",    "Reduce incoming physical damage this turn",           7,  BlessingEffect::MARTIAL_IRON_SKIN  },
    { "Battle Cry",   "Adjacent allies gain a +attack bonus this turn",      9,  BlessingEffect::MARTIAL_BATTLE_CRY },
    { "Endure",       "Survive a lethal blow at 1 HP once per engagement",  12,  BlessingEffect::MARTIAL_ENDURE     },
};

const SpellDef& MagicSystem::getSpellDef(SpellId spell) {
    return SPELL_DEFS[static_cast<int>(spell)];
}

// ---------------------------------------------------------------------------
// advanceMagic — regenerate magic and tick burn for a player's units
// ---------------------------------------------------------------------------

void MagicSystem::advanceMagic(int playerId) {
    // Regen magic for all units owned by this player.
    for (auto& [id, unit] : world.units) {
        if (unit->getOwner().getId() == playerId)
            unit->regenMagic();
    }

    // Tick burn DoT: collect positions first to avoid iterator invalidation.
    std::vector<Position> burningPositions;
    for (int r = 0; r < Game::HEIGHT; ++r) {
        for (int c = 0; c < Game::WIDTH; ++c) {
            Position pos(r, c);
            if (!world.getTileAt(pos).hasUnit()) continue;
            Unit* unit = world.getUnitAt(pos);
            if (!unit || unit->getOwner().getId() != playerId) continue;
            if (!unit->hasBurn()) continue;
            burningPositions.push_back(pos);
        }
    }

    for (const Position& pos : burningPositions) {
        Unit* unit = world.getUnitAt(pos);
        if (!unit || !unit->hasBurn()) continue;

        int dmg = unit->tickBurn();
        if (dmg <= 0) continue;

        unit->lowerHP(dmg);
        world.notifyObservers(DamageDealtEvent{pos, dmg, false});

        if (!unit->isAlive()) {
            UnitId uid = world.getTileAt(pos).getUnit().value();
            world.notifyObservers(UnitDiedEvent{uid, pos});
            world.removeUnit(pos);
        }
    }
}

// ---------------------------------------------------------------------------
// canCast / castSpell
// ---------------------------------------------------------------------------

bool MagicSystem::canCast(
    Position casterPos, SpellId spell, const Player& player) const
{
    const Unit* unit = world.getUnitAt(casterPos);
    if (!unit) return false;
    if (!unit->sameOwner(player)) return false;
    if (unit->isExhausted()) return false;
    return unit->canCast(getSpellDef(spell).cost);
}

std::optional<PlayerError> MagicSystem::castSpell(
    Position casterPos, Position targetPos, SpellId spell, const Player& player)
{
    Unit* caster = world.getUnitAt(casterPos);
    if (!caster)                    return PlayerError::INVALIDTARGET;
    if (!caster->sameOwner(player)) return PlayerError::INVALIDTARGET;
    if (caster->isExhausted())      return PlayerError::UNITCANTMOVE;

    const SpellDef& def = getSpellDef(spell);
    if (!caster->canCast(def.cost)) return PlayerError::INSUFFICIENTRESOURCES;

    if (spell == SpellId::SEAR) {
        Unit* target = world.getUnitAt(targetPos);
        if (!target)                    return PlayerError::INVALIDTARGET;
        if (target->sameOwner(player))  return PlayerError::INVALIDTARGET;

        int dist = std::max(std::abs(targetPos.row() - casterPos.row()),
                            std::abs(targetPos.col() - casterPos.col()));
        if (dist > caster->getRange()) return PlayerError::OUTOFREACH;

        caster->spendMagic(def.cost);
        target->applyBurn(3, 5);   // 3 turns, 5 damage per turn = 15 total
        caster->setAttacked(true); // casting uses the unit's action for this turn
    }

    return std::nullopt;
}

// ---------------------------------------------------------------------------
// getCastablePositions — returns enemy positions reachable by the given spell
// ---------------------------------------------------------------------------

std::vector<Position> MagicSystem::getCastablePositions(
    Position casterPos, const Player& player, SpellId spell) const
{
    const Unit* caster = world.getUnitAt(casterPos);
    if (!caster || !caster->sameOwner(player)) return {};
    if (caster->isExhausted()) return {};
    if (!caster->canCast(getSpellDef(spell).cost)) return {};

    std::vector<Position> result;
    int range = caster->getRange();

    for (int dr = -range; dr <= range; ++dr) {
        for (int dc = -range; dc <= range; ++dc) {
            if (std::max(std::abs(dr), std::abs(dc)) > range) continue;
            int nr = casterPos.row() + dr;
            int nc = casterPos.col() + dc;
            if (nr < 0 || nr >= Game::HEIGHT || nc < 0 || nc >= Game::WIDTH) continue;
            Position targetPos(nr, nc);
            const Unit* target = world.getUnitAt(targetPos);
            if (!target || target->sameOwner(player)) continue;
            result.push_back(targetPos);
        }
    }
    return result;
}
