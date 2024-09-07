// =============================================================================
// Copyright 2024 Simeon Manolov <s.manolloff@gmail.com>.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================

#include "Global.h"

#include "./stack.h"
#include "./hex.h"
#include "./util.h"
#include "battle/IBattleInfoCallback.h"
#include "bonuses/BonusCustomTypes.h"
#include "constants/EntityIdentifiers.h"
#include "spells/CSpellHandler.h"
#include "schema/v4/constants.h"
#include "schema/v4/types.h"

namespace MMAI::BAI::V4 {
    using A = Schema::V4::StackAttribute;

    Stack::Stack(const CStack* cstack_, int id, Queue &q, bool blocked, bool blocking, DamageEstimation estdmg)
    : cstack(cstack_)
    {
        // XXX: NULL attrs are used only for non-existing stacks
        // => don't fill with null here (as opposed to attrs in Hex)

        auto id2 = id % MAX_STACKS_PER_SIDE;
        alias = id2 + (id2 < 7 ? '0' : 'A'-7);

        // queue pos needs to be set first to determine if stack is active
        auto qit = std::find(q.begin(), q.end(), cstack->unitId());
        auto qpos = (qit == q.end()) ? 100 : qit - q.begin();

        auto [x, y] = Hex::CalcXY(cstack->getPosition());
        auto bonuses = cstack->getAllBonuses(Selector::all, nullptr);
        auto noretal = false;

        // XXX: config/creatures/<faction>.json is misleading
        //      (for example, no creature has NO_MELEE_PENALTY bonus there)
        //      The source of truth is the original H3 data files
        //      (referred to as CRTRAITS.TXT file, see CCreatureHandler::loadLegacyData)
        for (auto &bonus : *bonuses) {
            switch (bonus->type) {
            /* 100% */
            // break; case BonusType::LUCK: {
            //     if (bonus->valType == BonusValueType::INDEPENDENT_MAX)
            //         minluck = bonus->val;
            //     else if (bonus->valType == BonusValueType::INDEPENDENT_MIN)
            //         maxluck = bonus->val;  // e.g. hourglass of the evil hour
            //     else
            //         addattr(A::LUCK, bonus->val);
            // }

            /* 86% (127 creatures) */
            // break; case BonusType::MORALE: {
            //     if (bonus->valType == BonusValueType::INDEPENDENT_MAX)
            //         minmorale = bonus->val;  // e.g. minotaur
            //     else if (bonus->valType == BonusValueType::INDEPENDENT_MIN)
            //         maxmorale = bonus->val;  // e.g. spirit of oppression
            //     else
            //         addattr(A::MORALE, bonus->val);
            // }

            // break; case BonusType::NO_LUCK: minluck = maxluck = 0;
            // break; case BonusType::NO_MORALE: minmorale = maxmorale = 0;

            /* 28% (42 creatures) */
            break; case BonusType::FLYING: setattr(A::FLYING, 1);

            /* 20% (29 creatures), but overlaps with SHOTS */
            // break; case BonusType::SHOOTER: setattr(A::SHOOTER, 1);

            /* 10% (15 creatures) undead */
            /* +4% (6 creatures) unliving */
            /* +all war machines */
            // break; case BonusType::UNDEAD: setattr(A::NON_LIVING, 2); minmorale = maxmorale = 0;
            // break; case BonusType::NON_LIVING: setattr(A::NON_LIVING, 1); minmorale = maxmorale = 0;
            // break; case BonusType::SIEGE_WEAPON:

            /* 8.8% (13 creatures) */
            break; case BonusType::BLOCKS_RETALIATION: setattr(A::BLOCKS_RETALIATION, 1);

            /* 5.4% (8 creatures) */
            break; case BonusType::NO_MELEE_PENALTY: setattr(A::NO_MELEE_PENALTY, 1);

            /* 5.4% (8 creatures) */
            break; case BonusType::TWO_HEX_ATTACK_BREATH: setattr(A::TWO_HEX_ATTACK_BREATH, 1);

            /* 2.7% (4 creatures) */
            break; case BonusType::ADDITIONAL_ATTACK: setattr(A::ADDITIONAL_ATTACK, 1);

            break; case BonusType::SPELL_AFTER_ATTACK:
                switch(bonus->subtype.as<SpellID>()) {
                /* 1.4% (2 creatures) blind */
                /* +2.7% (4 creatures) petrify */
                /* +0.7% (1 creature) paralyze */
                break; case SpellID::BLIND:
                       case SpellID::STONE_GAZE:
                       case SpellID::PARALYZE:
                    addattr(A::BLIND_LIKE_ATTACK, bonus->val);

                /* 1.4% (2 creatures) */
                // break; case SpellID::WEAKNESS:

                /* 1.4% (2 creatures) */
                // break; case SpellID::DISEASE:

                /* 1.4% (2 creatures) */
                // break; case SpellID::DISPEL:

                /* 0.7% (1 creature) */
                // break; case SpellID::POISON:

                /* 2% (3 creatures) */
                // break; case SpellID::CURSE:

                /* 0.7% (1 creature) */
                // break; case SpellID::AGE:

                /* 0.7% (1 creature) */
                // break; case SpellID::THUNDERBOLT:
                }

            /* 4% (6 creatures); but requires extra "cast" action */
            // break; case BonusType::SPELLCASTER:

            /* 2% (3 creatures) */
            // break; case BonusType::SPELL_LIKE_ATTACK:
            //     switch(bonus->subtype.as<SpellID>()) {
            //     break; case SpellID::FIREBALL: addattr(A::AREA_ATTACK, 1);
            //     break; case SpellID::DEATH_CLOUD: setattr(A::AREA_ATTACK, 2);
            //     }

            /* 2% (3 creatures) */
            // XXX: being near a unicorn does NOT not give MAGIC_RESISTSNCE
            //      bonus -- must check neighbouring hexes manually and add
            //      the percentage here -- not worth the effort
            // break; case BonusType::MAGIC_RESISTANCE:

            /* 1.4% (2 creatures) all-around attack */
            /* +0.7% (1 creature) 3-headed attack */
            // break; case BonusType::THREE_HEADED_ATTACK: setattr(A::ATTACKS_ADJACENT, 1);
            //        case BonusType::ATTACKS_ALL_ADJACENT: setattr(A::ATTACKS_ADJACENT, 2);

            /* 1.4% (2 creatures) */
            // break; case BonusType::CHARGE_IMMUNITY:

            /* 1.4% (2 creatures) */
            // break; case BonusType::JOUSTING:

            /* 1.4% (2 creatures) */
            // break; case BonusType::NO_WALL_PENALTY:

            /* 1.4% (2 creatures) */
            // break; case BonusType::RETURN_AFTER_STRIKE:

            /* 0.02% (2 creatures per hate (1.4%) but only vs. 2 specific creatures (1.4%)) */
            // break; case BonusType::HATE:

            /* 0.7% (1 creature), or via artifact */
            // break; case BonusType::FREE_SHOOTING:

            /* 0.3% (4 creatures (2.7%) but only vs. 18 caster creatures (12%)) */
            // break; case BonusType::SPELL_RESISTANCE_AURA:

            /* 0.5% (6 creatures (4%) but only vs. 18 caster creatures (12%)) */
            // break; case BonusType::LEVEL_SPELL_IMMUNITY:

            /* <0.1% (4 creatures (2.7%) but only vs. 2 dmg-casting creatures (1.4%)) */
            // break; case BonusType::SPELL_DAMAGE_REDUCTION:

            /* <0.1% (2 creatures (1.4%) but only vs 2 mind-casting creatures (1.4%)) */
            // break; case BonusType::MIND_IMMUNITY:

            /* 1.4% (2 creatures) */
            // break; case BonusType::ENEMY_DEFENCE_REDUCTION:

            /* 0.7% (1 creature) */
            // break; case BonusType::FIRE_SHIELD:

            /* 0.7% (1 creature) */
            // break; case BonusType::LIFE_DRAIN:

            /* 0.7% (1 creature) */
            // break; case BonusType::DOUBLE_DAMAGE_CHANCE:

            /* 0.7% (1 creature); but requires extra "cast" action */
            // break; case BonusType::RANDOM_SPELLCASTER:

            /* 0.7% (1 creature) */
            // break; case BonusType::NO_DISTANCE_PENALTY:

            /* 0.1% (7 creatures can cast this spell with a 20% chance). Overlaps with QUEUE_POS */
            break; case BonusType::NOT_ACTIVE:
                if (cstack->unitType()->getId() != CreatureID::AMMO_CART) {
                    auto dur = bonus->duration;
                    auto turns = bonus->turnsRemain;
                    addattr(A::SLEEPING, (dur & BonusDuration::PERMANENT) ? 100 : turns);
                }

            /* <1% (petrified units, but only 2 other creatures may cast petrify wuth a small chance)) */
            /* + <0.1% (shielded units, but only 1 other creature may cast shield with a small chance)) */
            /* ? commanding hero with armorer skill */
            // break; case BonusType::GENERAL_DAMAGE_REDUCTION: {
            //     auto st = bonus->subtype.as<BonusCustomSubtype>();
            //     if (st == BonusCustomSubtype::damageTypeAll) {
            //         addattr(A::MELEE_DAMAGE_REDUCTION, bonus->val);
            //         addattr(A::RANGED_DAMAGE_REDUCTION, bonus->val);
            //     } else if (st == BonusCustomSubtype::damageTypeRanged) {
            //         addattr(A::RANGED_DAMAGE_REDUCTION, bonus->val);
            //     } else if (st == BonusCustomSubtype::damageTypeMelee) {
            //         addattr(A::MELEE_DAMAGE_REDUCTION, bonus->val);
            //     }
            // }

            /* <1% (blinded/paralyzed units, but only 3 other creature may cast petrify with a small chance)) */
            /* <0.1% (shielded units, but only 1 other creature may cast (air) shield with a small chance)) */
            // break; case BonusType::GENERAL_ATTACK_REDUCTION: {
            //     auto st = bonus->subtype.as<BonusCustomSubtype>();
            //     if (st == BonusCustomSubtype::damageTypeAll) {
            //         addattr(A::MELEE_ATTACK_REDUCTION, bonus->val);
            //         addattr(A::RANGED_ATTACK_REDUCTION, bonus->val);
            //     } else if (st == BonusCustomSubtype::damageTypeRanged) {
            //         addattr(A::RANGED_ATTACK_REDUCTION, bonus->val);
            //     } else if (st == BonusCustomSubtype::damageTypeMelee) {
            //         addattr(A::MELEE_ATTACK_REDUCTION, bonus->val);
            //     }
            // }

            /* ? any defending unit, but it's mostly overlapping with defense skill */
            // break; case BonusType::DEFENSIVE_STANCE:

            /* no creature casts this spell */
            // break; case BonusType::HYPNOTIZED:

            /* <1% (blinded/paralyzed units, but only 3 other creatures may cast petrify with a small chance)) */
            // break; case BonusType::NO_RETALIATION:

            /* <1% only 1 creature may cast this spell with a small chance */
            // break; case BonusType::MAGIC_MIRROR:

            /* no creature casts this spell (berserk) */
            // break; case BonusType::ATTACKS_NEAREST_CREATURE:

            /* no creature casts this spell */
            // break; case BonusType::FORGETFULL:

            /* 0.7% (1 creature) */
            // break; case BonusType::DEATH_STARE:

            /* <0.1% only 1 creature may cast this spell with a small chance */
            // break; case BonusType::POISON:

            /* 0.7% (1 creature) */
            // break; case BonusType::ACID_BREATH:

            /* 0.7% (1 creature) */
            // break; case BonusType::REBIRTH:

            /* 0.1% (<5% per school, but only vs. <5 caster creatures per scoll) */
            // break; case BonusType::SPELL_SCHOOL_IMMUNITY: {
            //     auto st = bonus->subtype.as<SpellSchool>();
            //     if (st == SpellSchool::ANY) {
            //         addattr(A::AIR_DAMAGE_REDUCTION, 100);
            //         addattr(A::WATER_DAMAGE_REDUCTION, 100);
            //         addattr(A::FIRE_DAMAGE_REDUCTION, 100);
            //         addattr(A::EARTH_DAMAGE_REDUCTION, 100);
            //     }
            //     else if (st == SpellSchool::AIR) addattr(A::AIR_DAMAGE_REDUCTION, 100);
            //     else if (st == SpellSchool::WATER) addattr(A::WATER_DAMAGE_REDUCTION, 100);
            //     else if (st == SpellSchool::FIRE) addattr(A::FIRE_DAMAGE_REDUCTION, 100);
            //     else if (st == SpellSchool::EARTH) addattr(A::EARTH_DAMAGE_REDUCTION, 100);
            // }
            break;
          }
        }

        shots = cstack->shots.available();
        // XXX: disabled DMG estimation
        // double avgdmg = 0.5*(estdmg.damage.max + estdmg.damage.min);
        // auto dmgPercentHP = std::clamp<int>(static_cast<int>(100 * avgdmg / cstack->getAvailableHealth()), 0, 100);

        auto sas = StackActState::DONE;

        if (cstack->willMove()) {
            sas = cstack->waitedThisTurn ? StackActState::WAITING : StackActState::READY;
        }


        setattr(A::ID, id);
        setattr(A::Y_COORD, y);
        setattr(A::X_COORD, x);
        setattr(A::SIDE, EI(cstack->unitSide()));
        setattr(A::QUANTITY, Util::Damp(cstack->getCount(), STACK_QTY_MAX));
        setattr(A::ATTACK, cstack->getAttack(shots > 0));
        setattr(A::DEFENSE, cstack->getDefense(false));
        setattr(A::SHOTS, shots);
        setattr(A::DMG_MIN, cstack->getMinDamage(shots > 0));
        setattr(A::DMG_MAX, cstack->getMaxDamage(shots > 0));
        setattr(A::HP, cstack->getMaxHealth());
        setattr(A::HP_LEFT, cstack->getFirstHPleft());
        setattr(A::SPEED, cstack->getMovementRange());
        setattr(A::QUEUE_POS, qpos);
        setattr(A::ACTSTATE, EI(sas));
        setattr(A::BLOCKED, blocked);
        setattr(A::BLOCKING, blocking);
        // XXX: disabled DMG estimation
        // setattr(A::ESTIMATED_DMG, dmgPercentHP);
        setattr(A::RETALIATIONS_LEFT, noretal ? 0 : cstack->counterAttacks.available());
        setattr(A::IS_WIDE, cstack->occupiedHex().isAvailable());
        setattr(A::AI_VALUE, Util::Damp(cstack->unitType()->getAIValue(), STACK_VALUE_MAX));

        finalize();
    }

    const StackAttrs& Stack::getAttrs() const {
        return attrs;
    }

    char Stack::getAlias() const {
        return alias;
    }

    int Stack::getAttr(StackAttribute a) const {
        return attr(a);
    }

    std::string Stack::name() const {
        // return boost::str(boost::format("(%d,%d)") % attr(A::Y_COORD) % attr(A::X_COORD));
        return "(" + std::to_string(attr(A::Y_COORD)) + "," + std::to_string(attr(A::X_COORD)) + ")";
    }

    int Stack::attr(StackAttribute a) const {
        return attrs.at(EI(a));
    };

    void Stack::setattr(StackAttribute a, int value) {
        attrs.at(EI(a)) = value;
    };

    void Stack::addattr(StackAttribute a, int value) {
        attrs.at(EI(a)) += value;
    };

    void Stack::finalize() {
        for (int i=0; i<EI(A::_count); ++i) {
            attrs.at(i) = std::min(attrs.at(i), std::get<3>(STACK_ENCODING.at(i)));
        }
    }
}
