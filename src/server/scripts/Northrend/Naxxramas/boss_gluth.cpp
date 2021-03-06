/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptPCH.h"
#include "naxxramas.h"

enum Spells
{
    SPELL_MORTAL_WOUND      = 25646,
    SPELL_ENRAGE_10         = 28371,
    SPELL_ENRAGE_25         = 54427,
    SPELL_DECIMATE_10       = 28374,
    SPELL_DECIMATE_25       = 54426,
    SPELL_BERSERK           = 26662,
    SPELL_INFECTED_WOUND    = 29306,
};

const Position PosSummon[3] =
{
    {3267.9f, -3172.1f, 297.42f, 0.94f},
    {3253.2f, -3132.3f, 297.42f, 0},
    {3308.3f, -3185.8f, 297.42f, 1.58f},
};

enum ScriptTexts
{
    EMOTE_ENRAGE,
    EMOTE_DECIMATE,
    EMOTE_NEARBY,
};

enum Events
{
    EVENT_NONE,
    EVENT_WOUND,
    EVENT_ENRAGE,
    EVENT_DECIMATE,
    EVENT_BERSERK,
    EVENT_SUMMON,
};

class boss_gluth : public CreatureScript
{
public:
    boss_gluth() : CreatureScript("boss_gluth") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_gluthAI (creature);
    }

    struct boss_gluthAI : public BossAI
    {
        boss_gluthAI(Creature* creature) : BossAI(creature, BOSS_GLUTH)
        {
            // Do not let Gluth be affected by zombies' debuff
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_INFECTED_WOUND, true);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who->GetEntry() == NPC_ZOMBIE && me->IsWithinDistInMap(who, 7))
            {
                SetGazeOn(who);
                Talk(EMOTE_NEARBY);
            }
            else
                BossAI::MoveInLineOfSight(who);
        }

        void EnterCombat(Unit* /*who*/)
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_WOUND, 10000);
            events.ScheduleEvent(EVENT_ENRAGE, 15000);
            events.ScheduleEvent(EVENT_DECIMATE, 105000);
            events.ScheduleEvent(EVENT_BERSERK, 480000);
            events.ScheduleEvent(EVENT_SUMMON, 15000);
        }

        void JustSummoned(Creature* summon)
        {
            if (summon->GetEntry() == NPC_ZOMBIE)
                summon->AI()->AttackStart(me);
            summons.Summon(summon);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictimWithGaze() || !CheckInRoom())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_WOUND:
                        DoCast(me->getVictim(), SPELL_MORTAL_WOUND);
                        events.ScheduleEvent(EVENT_WOUND, 10000);
                        break;
                    case EVENT_ENRAGE:
                        Talk(EMOTE_ENRAGE);
                        DoCast(me, RAID_MODE(SPELL_ENRAGE_10, SPELL_ENRAGE_25));
                        events.ScheduleEvent(EVENT_ENRAGE, 15000);
                        break;
                    case EVENT_DECIMATE:
                        Talk(EMOTE_DECIMATE);
                        DoCastAOE(RAID_MODE(SPELL_DECIMATE_10, SPELL_DECIMATE_25));
                        events.ScheduleEvent(EVENT_DECIMATE, 105000);
                        break;
                    case EVENT_BERSERK:
                        DoCast(me, SPELL_BERSERK);
                        events.ScheduleEvent(EVENT_BERSERK, 300000);
                        break;
                    case EVENT_SUMMON:
                        for (int32 i = 0; i < RAID_MODE(1, 2); ++i)
                            DoSummon(NPC_ZOMBIE, PosSummon[rand() % RAID_MODE(1, 3)]);
                        events.ScheduleEvent(EVENT_SUMMON, 10000);
                        break;
                }
            }

            if (me->getVictim() && me->getVictim()->GetEntry() == NPC_ZOMBIE)
            {
                if (me->IsWithinMeleeRange(me->getVictim()))
                {
                    me->Kill(me->getVictim());
                    me->ModifyHealth(int32(me->CountPctFromMaxHealth(5)));
                }
            }
            else
                DoMeleeAttackIfReady();
        }
    };

};

class npc_gluth_zombie : public CreatureScript
{
    public:

		npc_gluth_zombie() : CreatureScript("npc_gluth_zombie"){ }

        CreatureAI* GetAI(Creature* creature) const
        {
           return new npc_gluth_zombieAI(creature);
        }

		struct npc_gluth_zombieAI : public ScriptedAI
        {
            npc_gluth_zombieAI(Creature* creature) : ScriptedAI(creature) {}

			void SpellHit(Unit* /*caster*/, const SpellInfo* spell)
			{
				if (spell->Id == SPELL_DECIMATE_10)
					me->SetHealth(RAID_MODE(25200, 50400));
			}
		};
};

void AddSC_boss_gluth()
{
    new boss_gluth();
	new npc_gluth_zombie();
}
