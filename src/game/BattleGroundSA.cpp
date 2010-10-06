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
    
    // ----- ship not started
    if(!ShipStarted)
    {
        if(plr->GetTeam() == attackers)
        {
            plr->CastSpell(plr,12438,true);
            if (urand(0,1))
                plr->TeleportTo(607, 2682.936f, -830.368f, 50.0f, 2.895f, 0);
            else
                plr->TeleportTo(607, 2577.003f, 980.261f, 50.0f, 0.807f, 0);
        }else{
            plr->TeleportTo(607, 1209.7f, -65.16f, 70.1f, 0.0f, 0);
        }
    // ----- ship started
    }else{
        if(plr->GetTeam() == attackers)
        {
            // IMPORTANT: NEED IMPLEMENT URAND
            plr->TeleportTo(607, 1600.381f, -106.263f, 8.8745f, 3.78f, 0);
        }else{
            plr->TeleportTo(607, 1209.7f, -65.16f, 70.1f, 0.0f, 0);
        }
    }

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
