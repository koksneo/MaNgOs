/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundSA.h"
#include "Language.h"

BattleGroundSA::BattleGroundSA()
{
    //TODO FIX ME!
    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = 0;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_WS_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_WS_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_WS_HAS_BEGUN;
    TimerEnabled = false;
}

BattleGroundSA::~BattleGroundSA()
{

}

bool BattleGroundSA::TriggerTeamStartLoc(uint32 TeamID, float &X, float &Y, float &Z, float &O) const
{
    if(!ShipStarted)
    {
        if(TeamID == attackers)
        {
            if (urand(0,1))
            {
                X = 2682.936f;
                Y = -830.368f;
                Z = 50.0f;
                O = 2.895f;
            }
            else
            {
                X = 2577.003f;
                Y = 980.261f;
                Z = 50.0f;
                O = 0.807f;
            }
        }else{
            X = 1209.7f;
            Y = -65.16f;
            Z = 70.1f;
            O = 0.0f;
        }
    // ----- ship started
    }else{
        if(TeamID == attackers)
        {
            // IMPORTANT: NEED IMPLEMENT URAND
            X = 1600.381f;
            Y = -106.263f;
            Z = 8.8745f;
            O = 3.78f;
        }else{
            X = 1209.7f;
            Y = -65.16f;
            Z = 70.1f;
            O = 0.0f;
        }
    }

    return true;
}

void BattleGroundSA::Reset()
{
    BattleGround::Reset();

    // --- set team attackers and defender
    attackers = ((urand(0,1)) ? BG_TEAM_ALLIANCE : BG_TEAM_HORDE);
    ShipStarted = false;
}

void BattleGroundSA::Update(uint32 diff)
{
    BattleGround::Update(diff);
}

void BattleGroundSA::StartingEventCloseDoors()
{
}

void BattleGroundSA::StartingEventOpenDoors()
{
}

void BattleGroundSA::AddPlayer(Player *plr)
{
    BattleGround::AddPlayer(plr);
    //create score and add it to map, default values are set in constructor
    BattleGroundSAScore* sc = new BattleGroundSAScore;

    if(plr->GetTeam() == attackers)
        plr->CastSpell(plr,12438,true);

    m_PlayerScores[plr->GetGUID()] = sc;
}

void BattleGroundSA::RemovePlayer(Player* /*plr*/,uint64 /*guid*/)
{

}

void BattleGroundSA::HandleAreaTrigger(Player * /*Source*/, uint32 /*Trigger*/)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;
}

void BattleGroundSA::UpdatePlayerScore(Player* Source, uint32 type, uint32 value)
{

    BattleGroundScoreMap::iterator itr = m_PlayerScores.find(Source->GetGUID());
    if(itr == m_PlayerScores.end())                         // player not found...
        return;

    BattleGround::UpdatePlayerScore(Source,type,value);
}
