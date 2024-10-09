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

#include "CCreatureHandler.h"
#include "battle/IBattleInfoCallback.h"
#include "bonuses/BonusEnum.h"
#include "constants/EntityIdentifiers.h"

#include "BAI/v8/stack.h"
#include "schema/v8/constants.h"
#include "schema/v8/types.h"
#include <cmath>

namespace MMAI::BAI::V8 {
    using A = Schema::V8::StackAttribute;
    using F = Schema::V8::StackFlag;

    auto ValueCache = std::map<const CCreature*, float> {};

    // static
    int Stack::CalcValue(const CCreature* cr) {
        auto it = ValueCache.find(cr);
        if (it != ValueCache.end())
            return it->second;

        // Formula:
        // 10 * (A + B) * C * D1 * D2 * ... * Dn
        //

        // A = <offensive factor>
        // B = <defensive factor>
        // C = <speed factor>
        // D* = <bonus factor>

        auto att = cr->getBaseAttack();
        auto def = cr->getBaseAttack();
        auto dmg = (cr->getBaseDamageMax() + cr->getBaseDamageMin()) / 2.0;
        auto hp = cr->getBaseHitPoints();
        auto spd = cr->getBaseSpeed();
        auto shooter = cr->hasBonusOfType(BonusType::SHOOTER);
        auto bonuses = cr->getAllBonuses(Selector::all, nullptr);

        auto a = 3*dmg * (1 + std::min(4.0, 0.05*att));
        auto b = hp / (1 - std::min(0.7, 0.025*def));
        auto c = spd ? std::log(spd*2) : 0;
        auto d = shooter ? 1.5 : 1.0;

        for (auto &bonus : *bonuses) {
            switch (bonus->type) {
            break; case BonusType::ADDITIONAL_ATTACK:           d += (shooter ? 0.5 : 0.3);
            break; case BonusType::ADDITIONAL_RETALIATION:      d += (bonus->val * 0.1);
            break; case BonusType::ATTACKS_ALL_ADJACENT:        d += 0.2;
            break; case BonusType::BLOCKS_RETALIATION:          d += 0.3;
            break; case BonusType::DEATH_STARE:                 d += (bonus->val * 0.02);   // 10% = 0.2
            break; case BonusType::DOUBLE_DAMAGE_CHANCE:        d += (bonus->val * 0.005);  // 20% = 0.1
            break; case BonusType::FLYING:                      d += 0.1;
            break; case BonusType::NO_MELEE_PENALTY:            d += 0.1;
            break; case BonusType::THREE_HEADED_ATTACK:         d += 0.05;
            break; case BonusType::TWO_HEX_ATTACK_BREATH:       d += 0.1;
            break; case BonusType::UNLIMITED_RETALIATIONS:      d += 0.2;
            break; case BonusType::ENEMY_DEFENCE_REDUCTION:     d += (bonus->val * 0.0025); // 40% = 0.1
            break; case BonusType::FIRE_SHIELD:                 d += (bonus->val * 0.003);  // 20% = 0.1
            break; case BonusType::LIFE_DRAIN:                  d += (bonus->val * 0.003);  // 100% = 0.3
            break; case BonusType::NO_DISTANCE_PENALTY:         d += 0.5;
            break; case BonusType::SPELL_LIKE_ATTACK:
                switch(bonus->subtype.as<SpellID>()) {
                break; case SpellID::DEATH_CLOUD:               d += 0.2;
                // break; case SpellID::FIREBALL:
                }
            break; case BonusType::SPELL_AFTER_ATTACK:
                switch(bonus->subtype.as<SpellID>()) {
                break; case SpellID::BLIND:
                       case SpellID::STONE_GAZE:
                       case SpellID::PARALYZE:                  d += (bonus->val * 0.01);   // 20% = 0.2
                break; case SpellID::BIND:                      d += (bonus->val * 0.001);  // 100% = 0.1
                break; case SpellID::WEAKNESS:                  d += (bonus->val * 0.001);  // 100% = 0.1
                break; case SpellID::AGE:                       d += (bonus->val * 0.005);  // 20% = 0.1
                break; case SpellID::CURSE:                     d += (bonus->val * 0.0025); // 20% = 0.05
                // break; case SpellID::DISEASE:
                // break; case SpellID::DISPEL:
                // break; case SpellID::POISON:
                // break; case SpellID::THUNDERBOLT:
                }
            // break; case BonusType::ACID_BREATH:
            // break; case BonusType::CHARGE_IMMUNITY:
            // break; case BonusType::CREATURE_ENCHANT_POWER:
            // break; case BonusType::CREATURE_SPELL_POWER:
            // break; case BonusType::DRAGON_NATURE:
            // break; case BonusType::ENCHANTER:
            // break; case BonusType::FEAR:
            // break; case BonusType::FEARLESS:
            // break; case BonusType::GARGOYLE:
            // break; case BonusType::HATE:
            // break; case BonusType::HEALER:
            // break; case BonusType::HP_REGENERATION:
            // break; case BonusType::KING:
            // break; case BonusType::LEVEL_SPELL_IMMUNITY:
            // break; case BonusType::LUCK:
            // break; case BonusType::MAGIC_MIRROR:
            // break; case BonusType::MAGIC_RESISTANCE:
            // break; case BonusType::MANA_CHANNELING:
            // break; case BonusType::MANA_DRAIN:
            // break; case BonusType::MIND_IMMUNITY:
            // break; case BonusType::MORALE:
            // break; case BonusType::MORE_DAMAGE_FROM_SPELL:
            // break; case BonusType::NO_LUCK:
            // break; case BonusType::NO_TERRAIN_PENALTY:
            // break; case BonusType::NO_WALL_PENALTY:
            // break; case BonusType::NON_LIVING:
            // break; case BonusType::NOT_ACTIVE:
            // break; case BonusType::RANDOM_SPELLCASTER:
            // break; case BonusType::REBIRTH:
            // break; case BonusType::RETURN_AFTER_STRIKE:
            // break; case BonusType::SHOOTER:
            // break; case BonusType::SIEGE_WEAPON:
            // break; case BonusType::SPECIAL_CRYSTAL_GENERATION:
            // break; case BonusType::SPECIFIC_SPELL_POWER:
            // break; case BonusType::SPELL_DAMAGE_REDUCTION:
            // break; case BonusType::SPELL_IMMUNITY:
            // break; case BonusType::SPELL_RESISTANCE_AURA:
            // break; case BonusType::SPELL_SCHOOL_IMMUNITY:
            // break; case BonusType::SPELLCASTER:
            // break; case BonusType::UNDEAD:
            // break; case BonusType::VISIONS:
            }
        }

        // Multiply by 10 to reduce the integer rounding for weak units
        // (e.g. peasant 7.48 => 8 is a lot, 74.8 => 75 is OK)
        ValueCache[cr] = static_cast<int>(std::round(10 * (a + b) * c * d));
        // std::cout << "\n" << ValueCache[cr] << " " << cr->getNameSingularTextID() << "(a=" << a << ", b=" << b << ", c=" << c << ", d=" << d << ")\n";
        return ValueCache[cr];
    }

    Stack::Stack(
        const CStack* cstack_,
        Queue &q,
        const GlobalStats *lgstats,
        const GlobalStats *rgstats,
        const Stats stats,
        bool blocked,
        bool blocking,
        DamageEstimation estdmg
    ) : cstack(cstack_)
    {
        // XXX: NULL attrs are used only for non-existing stacks
        // => don't fill with null here (as opposed to attrs in Hex)

        auto slot = cstack->unitSlot();
        if (slot >= 0) {
            alias = slot + '0';
        } else if (slot == SlotID::SUMMONED_SLOT_PLACEHOLDER) {
            alias = 'S';
        } else if (slot == SlotID::WAR_MACHINES_SLOT) {
            alias = 'M';
        }

        // queue pos needs to be set first to determine if stack is active
        auto maxpos = std::get<3>(HEX_ENCODING.at(EI(HA::STACK_QUEUE_POS))) - 1;
        auto qit = std::find(q.begin(), q.end(), cstack->unitId());
        auto qpos = (qit == q.end()) ? maxpos : qit - q.begin();
        auto bonuses = cstack->getAllBonuses(Selector::all, nullptr);

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
            break; case BonusType::FLYING: setflag(F::FLYING);

            /* 20% (29 creatures), but overlaps with SHOTS */
            // break; case BonusType::SHOOTER: setattr(A::SHOOTER, 1);

            /* 10% (15 creatures) undead */
            /* +4% (6 creatures) unliving */
            /* +all war machines */
            // break; case BonusType::UNDEAD: setattr(A::NON_LIVING, 2); minmorale = maxmorale = 0;
            // break; case BonusType::NON_LIVING: setattr(A::NON_LIVING, 1); minmorale = maxmorale = 0;
            // break; case BonusType::SIEGE_WEAPON:

            /* 8.8% (13 creatures) */
            break; case BonusType::BLOCKS_RETALIATION: setflag(F::BLOCKS_RETALIATION);

            /* 5.4% (8 creatures) */
            break; case BonusType::NO_MELEE_PENALTY: setflag(F::NO_MELEE_PENALTY);

            /* 5.4% (8 creatures) */
            break; case BonusType::TWO_HEX_ATTACK_BREATH: setflag(F::TWO_HEX_ATTACK_BREATH);

            /* 2.7% (4 creatures) */
            break; case BonusType::ADDITIONAL_ATTACK: setflag(F::ADDITIONAL_ATTACK);

            break; case BonusType::SPELL_AFTER_ATTACK:
                switch(bonus->subtype.as<SpellID>()) {
                /* 1.4% (2 creatures) blind */
                /* +2.7% (4 creatures) petrify */
                /* +0.7% (1 creature) paralyze */
                break; case SpellID::BLIND:
                       case SpellID::STONE_GAZE:
                       case SpellID::PARALYZE:
                    setflag(F::BLIND_LIKE_ATTACK);

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
                if (cstack->unitType()->getId() != CreatureID::AMMO_CART)
                    setflag(F::SLEEPING);


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

        // double avgdmg = 0.5*(estdmg.damage.max + estdmg.damage.min);
        // auto dmgPercentHP = std::clamp<int>(std::round(100 * avgdmg / cstack->getAvailableHealth()), 0, 100);

        if (cstack->willMove()) {
            setflag(F::WILL_ACT);
            // XXX: do NOT use cstack->waited()
            //      (it returns cstack->waiting, which becomes false after acting)
            if (!cstack->waitedThisTurn)
                setflag(F::CAN_WAIT);
        }

        if (cstack->ableToRetaliate())
            setflag(F::CAN_RETALIATE);

        if (blocked)
            setflag(F::BLOCKED);

        if (blocking)
            setflag(F::BLOCKING);

        if (cstack->occupiedHex().isAvailable())
            setflag(F::IS_WIDE);

        if (qpos == 0)
            setflag(F::IS_ACTIVE);

        shots = cstack->shots.available();

        int cid = cstack->creatureId().num;
        if (cid > Schema::V8::CREATURE_ID_MAX) {
            logAi->error("MMAI error: unknown creature with: %d (%s)", cid, cstack->getDescription());
            #ifdef ENABLE_ML
                throw std::runtime_error("unknown creature id: " + std::to_string(cid));
            #endif
            cid = 122; // this is a "NOT USED (1)" creature
        }

        auto valueOne = CalcValue(cstack->unitType());

        setattr(A::SIDE, EI(cstack->unitSide()));
        // setattr(A::CREATURE_ID, cid);
        setattr(A::QUANTITY, cstack->getCount());
        setattr(A::ATTACK, cstack->getAttack(shots > 0));
        setattr(A::DEFENSE, cstack->getDefense(false));
        setattr(A::SHOTS, shots);
        setattr(A::DMG_MIN, cstack->getMinDamage(shots > 0));
        setattr(A::DMG_MAX, cstack->getMaxDamage(shots > 0));
        setattr(A::HP, cstack->getMaxHealth());
        setattr(A::HP_LEFT, cstack->getFirstHPleft());
        setattr(A::SPEED, cstack->getMovementRange());
        setattr(A::QUEUE_POS, qpos);
        // setattr(A::ESTIMATED_DMG, dmgPercentHP);

        // std::cout << "[" << cstack->unitType()->getNameSingularTextID() << "] lgstats->valueNow:" << lgstats->valueNow << ", rgstats->valueNow: " << rgstats->valueNow << "\n";
        auto bf_valueNow = lgstats->valueNow + rgstats->valueNow;
        auto bf_valuePrev = lgstats->valuePrev + rgstats->valuePrev;
        auto bf_valueStart = lgstats->valueStart + rgstats->valueStart;
        auto bf_hpPrev = lgstats->hpPrev + rgstats->hpPrev;
        auto bf_hpStart = lgstats->hpStart + rgstats->hpStart;
        auto value = valueOne * cstack->getCount();

        // std::cout << "[" << cstack->unitType()->getNameSingularTextID() << "] VALUE_ONE:" << valueOne << ", VALUE: " << value << ", bf_valueNow: " << bf_valueNow << "\n";
        auto percent = [](int v1, int v2) {
            return 100 * v1 / v2;
        };

        setattr(A::VALUE_ONE, valueOne);
        setattr(A::VALUE_REL,             percent(value, bf_valueNow));
        setattr(A::VALUE_REL0,            percent(value, bf_valueStart));
        setattr(A::VALUE_KILLED_REL,      percent(stats.valueKilledNow, bf_valuePrev));
        setattr(A::VALUE_KILLED_ACC_REL0, percent(stats.valueKilledTotal, bf_valueStart));
        setattr(A::VALUE_LOST_REL,        percent(stats.valueLostNow, bf_valuePrev));
        setattr(A::VALUE_LOST_ACC_REL0,   percent(stats.valueLostTotal, bf_valueStart));
        setattr(A::DMG_DEALT_REL,         percent(stats.dmgDealtNow, bf_hpPrev));
        setattr(A::DMG_DEALT_ACC_REL0,    percent(stats.dmgDealtTotal, bf_hpStart));
        setattr(A::DMG_RECEIVED_REL,      percent(stats.dmgReceivedNow, bf_hpPrev));
        setattr(A::DMG_RECEIVED_ACC_REL0, percent(stats.dmgReceivedTotal, bf_hpStart));

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

    int Stack::getFlag(StackFlag sf) const {
        return flag(sf);
    }

    bool Stack::flag(StackFlag f) const {
        return flags.test(EI(f));
    };

    int Stack::attr(StackAttribute a) const {
        return attrs.at(EI(a));
    };

    void Stack::setflag(StackFlag f) {
        flags.set(EI(f));
    };

    void Stack::setattr(StackAttribute a, int value) {
        attrs.at(EI(a)) = value;
    };

    void Stack::addattr(StackAttribute a, int value) {
        attrs.at(EI(a)) += value;
    };

    std::map<const CCreature*, int> warned {};

    void Stack::finalize() {
        setattr(A::FLAGS, flags.to_ulong());
        // for (int i=0; i<EI(A::_count); ++i) {
        //     auto vmax = std::get<3>(HEX_ENCODING.at(STACK_ATTR_OFFSET + i));
        //     auto v = attrs.at(i);
        //     if (v > vmax) {
        //         auto cr = cstack->unitType();
        //         if (warned[cr] % 100 == 0) {
        //             warned[cr] = 0;
        //             std::cout << boost::str(
        //                 boost::format("WARNING: %s > %s (SA(%d), %s, QTY=%d)\n")
        //                 % v % vmax % i % cr->getNameSingularTextID() % cstack->getCount()
        //             );
        //         }
        //         v = vmax;
        //         warned[cr]++;
        //     }

        //     attrs.at(i) = std::min(attrs.at(i), vmax);
        // }
    }
}
