#include "model/magic.h"
#include "model/world.h"
#include "model/unit.h"
#include <algorithm>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Spell definitions — one entry per SpellId (order must match the enum).
// ---------------------------------------------------------------------------

static const SpellDef SPELL_DEFS[] = {
    // name            description                                          cost  requiredBlessing
    { "Sear",         "Apply a burning debuff to the target unit",           8,  BlessingEffect::FLAME_SEAR         },
    { "Blaze Aura",   "Nearby allies deal bonus fire damage this turn",     10,  BlessingEffect::FLAME_BLAZE_AURA   },
    { "Ignite",       "Gain extra movement speed after the next kill",       6,  BlessingEffect::FLAME_IGNITE       },

    { "Veil",         "You and adjacent allies gain 30% dodge until your next turn", 12, BlessingEffect::SHADOW_VEIL },
    { "Shadow Step",  "Reposition to a new tile after defeating an enemy",   9,  BlessingEffect::SHADOW_STEP        },
    { "Dark Strike",  "Next attack bypasses a portion of enemy defense",     8,  BlessingEffect::SHADOW_DARK_STRIKE },

    { "Swiftness",    "Gain +1 movement range this turn",                    6,  BlessingEffect::GALE_SWIFTNESS     },
    { "Far Reach",    "Gain +1 attack range this turn",                      6,  BlessingEffect::GALE_FAR_REACH     },
    { "Tumble",       "May move again after attacking this turn",            10,  BlessingEffect::GALE_TUMBLE        },

    { "Iron Skin",    "Reduce incoming physical damage this turn",           7,  BlessingEffect::MARTIAL_IRON_SKIN  },
    { "Battle Cry",   "Adjacent allies gain a +attack bonus this turn",      9,  BlessingEffect::MARTIAL_BATTLE_CRY },
    { "Endure",       "Survive a lethal blow at 1 HP once per engagement",  12,  BlessingEffect::MARTIAL_ENDURE     },
    { "Shadow Pounce", "Scout vanishes for one turn; next attack deals double damage", 8, BlessingEffect::SHADOW_POUNCE },
    { "Flame Charge",  "Charge in a straight line (4 tiles). Enemies hit for 25 dmg. Trail burns for 3 turns.", 5, BlessingEffect::FLAME_IGNITE },
};

const SpellDef& MagicSystem::getSpellDef(SpellId spell) {
    return SPELL_DEFS[static_cast<int>(spell)];
}

// ---------------------------------------------------------------------------
// advanceMagic — regenerate magic and tick burn for a player's units
// ---------------------------------------------------------------------------

void MagicSystem::advanceMagic(int playerId) {
    // Regen magic and tick veil for all units owned by this player.
    for (auto& [id, unit] : world.units) {
        if (unit->getOwner().getId() == playerId) {
            unit->regenMagic();
            unit->tickVeil();   // veil expires at the start of the caster's next turn
        }
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
        target->applyBurn(3, 5);
        caster->setAttacked(true);
        return std::nullopt;
    }

    if (spell == SpellId::VEIL) {
        // Apply veil to the caster and every adjacent friendly unit.
        caster->applyVeil(1);
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                int nr = casterPos.row() + dr;
                int nc = casterPos.col() + dc;
                if (nr < 0 || nr >= Game::HEIGHT || nc < 0 || nc >= Game::WIDTH) continue;
                Position adj(nr, nc);
                Unit* ally = world.getUnitAt(adj);
                if (ally && ally->sameOwner(player)) ally->applyVeil(1);
            }
        }
        caster->spendMagic(def.cost);
        caster->setAttacked(true);
        return std::nullopt;
    }

    if (spell == SpellId::SHADOW_POUNCE) {
        if (caster->getType() != UnitType::SCOUT) return PlayerError::NOTSUPPORTED;
        if (targetPos != casterPos) return PlayerError::INVALIDTARGET;

        caster->spendMagic(def.cost);
        caster->applyShadowPounce();
        caster->setAttacked(true);
        return std::nullopt;
    }

    if (spell == SpellId::FLAME_CHARGE) {
        if (caster->getType() != UnitType::CAVALRY) return PlayerError::NOTSUPPORTED;

        int dr = targetPos.row() - casterPos.row();
        int dc = targetPos.col() - casterPos.col();

        // Must be a pure cardinal direction (not diagonal)
        if (dr != 0 && dc != 0) return PlayerError::INVALIDTARGET;

        int dist = std::max(std::abs(dr), std::abs(dc));
        if (dist < 1 || dist > 4)          return PlayerError::OUTOFREACH;

        // Destination must be empty — cavalry lands there
        if (world.hasUnitAt(targetPos))    return PlayerError::INVALIDTARGET;

        int stepR = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
        int stepC = (dc == 0) ? 0 : (dc > 0 ? 1 : -1);

        // Allied unit anywhere in the path blocks the charge
        for (int i = 1; i < dist; ++i) {
            Position p(casterPos.row() + stepR * i, casterPos.col() + stepC * i);
            if (world.hasUnitAt(p)) {
                const Unit* u = world.getUnitAt(p);
                if (u && u->sameOwner(player)) return PlayerError::INVALIDTARGET;
            }
        }

        caster->spendMagic(def.cost);

        // Damage every enemy along the path (intermediate tiles only)
        for (int i = 1; i < dist; ++i) {
            Position p(casterPos.row() + stepR * i, casterPos.col() + stepC * i);
            if (!world.hasUnitAt(p)) continue;
            Unit* enemy = world.getUnitAt(p);
            if (!enemy || enemy->sameOwner(player)) continue;

            int dmg = 25;
            if (world.blessingSystem.tryEndure(*enemy, dmg)) {
                enemy->setHealth(1);
            } else {
                enemy->lowerHP(dmg);
                if (!enemy->isAlive()) {
                    world.notifyObservers(DamageDealtEvent{p, dmg, false});
                    world.removeUnit(p);
                }
            }
        }

        // Set fire on all intermediate tiles (the cinder trail)
        for (int i = 1; i < dist; ++i) {
            Position p(casterPos.row() + stepR * i, casterPos.col() + stepC * i);
            world.getTileAt(p).setFire(3, player.getId());
        }

        // Teleport cavalry to destination
        UnitId uid = world.getTileAt(casterPos).getUnit().value();
        world.getTileAt(casterPos).removeUnit();
        world.getTileAt(targetPos).placeUnit(uid);

        // Charge exhausts the cavalry
        caster->setAttacked(true);
        caster->setMoved(true);

        return std::nullopt;
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

    // VEIL: self-cast — caster selects themselves; allies are automatically included
    if (spell == SpellId::VEIL) {
        return { casterPos };
    }

    // SHADOW_POUNCE: Scout targets only themselves (self-cast)
    if (spell == SpellId::SHADOW_POUNCE) {
        if (caster->getType() != UnitType::SCOUT) return {};
        return { casterPos };
    }

    // FLAME_CHARGE: show empty landing tiles along cardinal lines (up to 4 tiles)
    if (spell == SpellId::FLAME_CHARGE) {
        if (caster->getType() != UnitType::CAVALRY) return {};

        std::vector<Position> result;
        static const int DIRS[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};

        for (auto& dir : DIRS) {
            for (int dist = 1; dist <= 4; ++dist) {
                int nr = casterPos.row() + dir[0] * dist;
                int nc = casterPos.col() + dir[1] * dist;
                if (nr < 0 || nr >= Game::HEIGHT || nc < 0 || nc >= Game::WIDTH) break;
                Position p(nr, nc);

                if (world.hasUnitAt(p)) {
                    const Unit* u = world.getUnitAt(p);
                    if (u && u->sameOwner(player)) break;  // allied unit blocks path
                    continue;  // enemy in path: skip as landing spot, keep checking beyond
                }

                result.push_back(p);  // empty tile: valid landing spot
            }
        }
        return result;
    }

    // Default (SEAR): all enemies within Chebyshev range
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
            if (target->isShadowCloaked()) continue;   // invisible — untargetable
            result.push_back(targetPos);
        }
    }
    return result;
}
