/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/server/mmo/entities/ball_effect.h>
#include <game/server/mmo/entities/money_bag.h>
#include <game/teamscore.h>

#include "gamecontext.h"
#include "gamecontroller.h"
#include "player.h"

#include "entities/character.h"
#include "entities/pickup.h"

#include "mmo/dummies/dummy_base.h"
#include "mmo/entities/pickup_job.h"

IGameController::IGameController(class CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pConfig = m_pGameServer->Config();
	m_pServer = m_pGameServer->Server();
	m_pGameType = "unknown";

	//
	DoWarmup(g_Config.m_SvWarmup);
	m_GameOverTick = -1;
	m_SuddenDeath = 0;
	m_RoundStartTick = Server()->Tick();
	m_RoundCount = 0;
	m_GameFlags = 0;
	m_aMapWish[0] = 0;

	m_UnbalancedTick = -1;
	m_ForceBalanced = false;

	m_CurrentRecord = 0;
}

IGameController::~IGameController() = default;

void IGameController::DoActivityCheck()
{
	if(g_Config.m_SvInactiveKickTime == 0)
		return;

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
#ifdef CONF_DEBUG
		if(g_Config.m_DbgDummies)
		{
			if(i >= MAX_CLIENTS - g_Config.m_DbgDummies)
				break;
		}
#endif
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && Server()->GetAuthedState(i) == AUTHED_NO)
		{
			if(Server()->Tick() > GameServer()->m_apPlayers[i]->m_LastActionTick + g_Config.m_SvInactiveKickTime * Server()->TickSpeed() * 60)
			{
				switch(g_Config.m_SvInactiveKick)
				{
				case 0:
				{
					// move player to spectator
					DoTeamChange(GameServer()->m_apPlayers[i], TEAM_SPECTATORS);
				}
				break;
				case 1:
				{
					// move player to spectator if the reserved slots aren't filled yet, kick him otherwise
					int Spectators = 0;
					for(auto &pPlayer : GameServer()->m_apPlayers)
						if(pPlayer && pPlayer->GetTeam() == TEAM_SPECTATORS)
							++Spectators;
					if(Spectators >= g_Config.m_SvSpectatorSlots)
						Server()->Kick(i, "Kicked for inactivity");
					else
						DoTeamChange(GameServer()->m_apPlayers[i], TEAM_SPECTATORS);
				}
				break;
				case 2:
				{
					// kick the player
					Server()->Kick(i, "Kicked for inactivity");
				}
				}
			}
		}
	}
}

float IGameController::EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos, int DDTeam)
{
	float Score = 0.0f;
	CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for(; pC; pC = (CCharacter *)pC->TypeNext())
	{
		// ignore players in other teams
		if(GameServer()->GetDDRaceTeam(pC->GetPlayer()->GetCID()) != DDTeam)
			continue;

		float d = distance(Pos, pC->m_Pos);
		Score += d == 0 ? 1000000000.0f : 1.0f / d;
	}

	return Score;
}

void IGameController::EvaluateSpawnType(CSpawnEval *pEval, int Type, int DDTeam)
{
	// j == 0: Find an empty slot, j == 1: Take any slot if no empty one found
	for(int j = 0; j < 2 && !pEval->m_Got; j++)
	{
		// get spawn point
		for(const vec2 &SpawnPoint : m_avSpawnPoints[Type])
		{
			vec2 P = SpawnPoint;
			if(j == 0)
			{
				// check if the position is occupado
				CEntity *apEnts[MAX_CLIENTS];
				int Num = GameServer()->m_World.FindEntities(SpawnPoint, 64, apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
				vec2 aPositions[5] = {vec2(0.0f, 0.0f), vec2(-32.0f, 0.0f), vec2(0.0f, -32.0f), vec2(32.0f, 0.0f), vec2(0.0f, 32.0f)}; // start, left, up, right, down
				int Result = -1;
				for(int Index = 0; Index < 5 && Result == -1; ++Index)
				{
					Result = Index;
					if(!GameServer()->m_World.m_Core.m_aTuning[0].m_PlayerCollision)
						break;
					for(int c = 0; c < Num; ++c)
					{
						CCharacter *pChr = static_cast<CCharacter *>(apEnts[c]);
						if(GameServer()->Collision()->CheckPoint(SpawnPoint + aPositions[Index]) ||
							distance(pChr->m_Pos, SpawnPoint + aPositions[Index]) <= pChr->GetProximityRadius())
						{
							Result = -1;
							break;
						}
					}
				}
				if(Result == -1)
					continue; // try next spawn point

				P += aPositions[Result];
			}

			float S = EvaluateSpawnPos(pEval, P, DDTeam);
			if(!pEval->m_Got || (j == 0 && pEval->m_Score > S))
			{
				pEval->m_Got = true;
				pEval->m_Score = S;
				pEval->m_Pos = P;
			}
		}
	}
}

bool IGameController::CanSpawn(int Team, vec2 *pOutPos, int DDTeam)
{
	// spectators can't spawn
	if(Team == TEAM_SPECTATORS)
		return false;

	CSpawnEval Eval;
	EvaluateSpawnType(&Eval, 0, DDTeam);
	EvaluateSpawnType(&Eval, 1, DDTeam);
	EvaluateSpawnType(&Eval, 2, DDTeam);

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

bool IGameController::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	dbg_assert(Index >= 0, "Invalid entity index");

	const vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);

	int aSides[8];
	aSides[0] = GameServer()->Collision()->Entity(x, y + 1, Layer);
	aSides[1] = GameServer()->Collision()->Entity(x + 1, y + 1, Layer);
	aSides[2] = GameServer()->Collision()->Entity(x + 1, y, Layer);
	aSides[3] = GameServer()->Collision()->Entity(x + 1, y - 1, Layer);
	aSides[4] = GameServer()->Collision()->Entity(x, y - 1, Layer);
	aSides[5] = GameServer()->Collision()->Entity(x - 1, y - 1, Layer);
	aSides[6] = GameServer()->Collision()->Entity(x - 1, y, Layer);
	aSides[7] = GameServer()->Collision()->Entity(x - 1, y + 1, Layer);

	if(Index == ENTITY_SPAWN && Initial)
	{
		const int Type = Index - ENTITY_SPAWN;
		m_avSpawnPoints[Type].push_back(Pos);
	}

	int Type = -1;
	int SubType = 0;

	if(Index == ENTITY_ARMOR_1)
		Type = POWERUP_ARMOR;
	else if(Index == ENTITY_HEALTH_1)
		Type = POWERUP_HEALTH;

	if(Type != -1)
	{
		CPickup *pPickup = new CPickup(&GameServer()->m_World, Type, SubType, Layer, Number);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

bool IGameController::OnQuadEntity(const char *pLayerName, vec2 Pivot, vec2 *pPoints)
{
	vec2 Pos;
	for (int i = 0; i < 4; i++)
		Pos += pPoints[i];
	Pos /= 4;

	// Handle mobs spawn points
	if (str_startswith_nocase(pLayerName, "Bot"))
		GameServer()->m_MMOCore.OnMapBotPoint(Pos, pLayerName);
	// Handle mobs
	else if (!str_comp(pLayerName, "Farm"))
		new CPickupJob(&GameServer()->m_World, Pos, PICKUP_JOB_TYPE_FARM);
	else if (!str_comp(pLayerName, "Mine"))
		new CPickupJob(&GameServer()->m_World, Pos, PICKUP_JOB_TYPE_MINE);
	else if (!str_comp(pLayerName, "Wood"))
		new CPickupJob(&GameServer()->m_World, Pos, PICKUP_JOB_TYPE_WOOD);
	else if (!str_comp(pLayerName, "MoneyBag"))
		new CMoneyBag(&GameServer()->m_World, Pos);
	else if (!str_comp(pLayerName, "BallEffect"))
		new CBallEffect(&GameServer()->m_World, Pos);
	else if(!str_comp(pLayerName, "Material"))
		new CPickupJob(&GameServer()->m_World, Pos, PICKUP_JOB_TYPE_MATERIAL);

	return false;
}

void IGameController::OnPlayerConnect(CPlayer *pPlayer)
{
	int ClientID = pPlayer->GetCID();
	pPlayer->Respawn();

	if(!Server()->ClientPrevIngame(ClientID))
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientID, Server()->ClientName(ClientID), pPlayer->GetTeam());
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}
}

void IGameController::OnPlayerDisconnect(class CPlayer *pPlayer, const char *pReason)
{
	pPlayer->OnDisconnect();
	int ClientID = pPlayer->GetCID();
	if(Server()->ClientIngame(ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(ClientID), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, -1, CGameContext::CHAT_SIX);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", ClientID, Server()->ClientName(ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}
}

void IGameController::EndRound()
{
	if(m_Warmup) // game can't end when we are running warmup
		return;

	GameServer()->m_World.m_Paused = true;
	m_GameOverTick = Server()->Tick();
	m_SuddenDeath = 0;
}

void IGameController::ResetGame()
{
	GameServer()->m_World.m_ResetRequested = true;
}

const char *IGameController::GetTeamName(int Team)
{
	if(Team == 0)
		return "game";
	return "spectators";
}

void IGameController::StartRound()
{
	ResetGame();

	m_RoundStartTick = Server()->Tick();
	m_SuddenDeath = 0;
	m_GameOverTick = -1;
	GameServer()->m_World.m_Paused = false;
	m_ForceBalanced = false;
	Server()->DemoRecorder_HandleAutoStart();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "start round type='%s' teamplay='%d'", m_pGameType, m_GameFlags & GAMEFLAG_TEAMS);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
}

void IGameController::ChangeMap(const char *pToMap)
{
	Server()->ChangeMap(pToMap);
}

void IGameController::OnReset()
{
	for(auto &pPlayer : GameServer()->m_apPlayers)
		if(pPlayer)
			pPlayer->Respawn();
}

int IGameController::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	return 0;
}

void IGameController::OnCharacterSpawn(class CCharacter *pChr)
{
	pChr->GiveWeapon(WEAPON_HAMMER);

	CAccountInventory &Inv = pChr->GetPlayer()->m_AccInv;

	if (Inv.HaveItem(ITEM_GUN))
		pChr->GiveWeapon(WEAPON_GUN);
	if (Inv.HaveItem(ITEM_GRENADE))
		pChr->GiveWeapon(WEAPON_GRENADE);
	if (Inv.HaveItem(ITEM_SHOTGUN))
		pChr->GiveWeapon(WEAPON_SHOTGUN);
	if (Inv.HaveItem(ITEM_LASER))
		pChr->GiveWeapon(WEAPON_LASER);
}

void IGameController::HandleCharacterTiles(CCharacter *pChr, int MapIndex)
{
	// Do nothing by default
}

void IGameController::DoWarmup(int Seconds)
{
	if(Seconds < 0)
		m_Warmup = 0;
	else
		m_Warmup = Seconds * Server()->TickSpeed();
}

bool IGameController::IsForceBalanced()
{
	return false;
}

bool IGameController::CanBeMovedOnBalance(int ClientID)
{
	return true;
}

void IGameController::Tick()
{
	// do warmup
	if(m_Warmup)
	{
		m_Warmup--;
		if(!m_Warmup)
			StartRound();
	}

	if(m_GameOverTick != -1)
	{
		// game over.. wait for restart
		if(Server()->Tick() > m_GameOverTick + Server()->TickSpeed() * 10)
		{
			StartRound();
			m_RoundCount++;
		}
	}

	DoActivityCheck();
}

void IGameController::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = Server()->SnapNewItem<CNetObj_GameInfo>(0);
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if(m_GameOverTick != -1)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
	if(m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;
	if(GameServer()->m_World.m_Paused)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;
	pGameInfoObj->m_WarmupTimer = m_Warmup;

	pGameInfoObj->m_RoundNum = 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount + 1;

	CCharacter *pChr;
	CPlayer *pPlayer = SnappingClient != SERVER_DEMO_CLIENT ? GameServer()->m_apPlayers[SnappingClient] : 0;
	CPlayer *pPlayer2;

	if(pPlayer && (pPlayer->m_TimerType == CPlayer::TIMERTYPE_GAMETIMER || pPlayer->m_TimerType == CPlayer::TIMERTYPE_GAMETIMER_AND_BROADCAST) && pPlayer->GetClientVersion() >= VERSION_DDNET_GAMETICK)
	{
		if((pPlayer->GetTeam() == TEAM_SPECTATORS || pPlayer->IsPaused()) && pPlayer->m_SpectatorID != SPEC_FREEVIEW && (pPlayer2 = GameServer()->m_apPlayers[pPlayer->m_SpectatorID]))
		{
			if((pChr = pPlayer2->GetCharacter()) && pChr->m_DDRaceState == DDRACE_STARTED)
			{
				pGameInfoObj->m_WarmupTimer = -pChr->m_StartTime;
				pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_RACETIME;
			}
		}
		else if((pChr = pPlayer->GetCharacter()) && pChr->m_DDRaceState == DDRACE_STARTED)
		{
			pGameInfoObj->m_WarmupTimer = -pChr->m_StartTime;
			pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_RACETIME;
		}
	}

	CNetObj_GameInfoEx *pGameInfoEx = Server()->SnapNewItem<CNetObj_GameInfoEx>(0);
	if(!pGameInfoEx)
		return;

	pGameInfoEx->m_Flags =
		GAMEINFOFLAG_ALLOW_HOOK_COLL |
		GAMEINFOFLAG_BUG_DDRACE_INPUT |
		GAMEINFOFLAG_PREDICT_DDRACE |
		GAMEINFOFLAG_PREDICT_DDRACE_TILES |
		GAMEINFOFLAG_ALLOW_ZOOM |
		GAMEINFOFLAG_ENTITIES_DDNET |
		GAMEINFOFLAG_ENTITIES_DDRACE;
	pGameInfoEx->m_Flags2 =
		GAMEINFOFLAG2_HUD_DDRACE |
		GAMEINFOFLAG2_NO_WEAK_HOOK;
	pGameInfoEx->m_Version = GAMEINFO_CURVERSION;

	if(Server()->IsSixup(SnappingClient))
	{
		protocol7::CNetObj_GameData *pGameData = Server()->SnapNewItem<protocol7::CNetObj_GameData>(0);
		if(!pGameData)
			return;

		pGameData->m_GameStartTick = m_RoundStartTick;
		pGameData->m_GameStateFlags = 0;
		if(m_GameOverTick != -1)
			pGameData->m_GameStateFlags |= protocol7::GAMESTATEFLAG_GAMEOVER;
		if(m_SuddenDeath)
			pGameData->m_GameStateFlags |= protocol7::GAMESTATEFLAG_SUDDENDEATH;
		if(GameServer()->m_World.m_Paused)
			pGameData->m_GameStateFlags |= protocol7::GAMESTATEFLAG_PAUSED;

		pGameData->m_GameStateEndTick = 0;

		protocol7::CNetObj_GameDataRace *pRaceData = Server()->SnapNewItem<protocol7::CNetObj_GameDataRace>(0);
		if(!pRaceData)
			return;

		pRaceData->m_BestTime = round_to_int(m_CurrentRecord * 1000);
		pRaceData->m_Precision = 0;
		pRaceData->m_RaceFlags = protocol7::RACEFLAG_HIDE_KILLMSG | protocol7::RACEFLAG_KEEP_WANTED_WEAPON;
	}

	if(!GameServer()->Switchers().empty())
	{
		int Team = pPlayer && pPlayer->GetCharacter() ? pPlayer->GetCharacter()->Team() : 0;

		if(pPlayer && (pPlayer->GetTeam() == TEAM_SPECTATORS || pPlayer->IsPaused()) && pPlayer->m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[pPlayer->m_SpectatorID] && GameServer()->m_apPlayers[pPlayer->m_SpectatorID]->GetCharacter())
			Team = GameServer()->m_apPlayers[pPlayer->m_SpectatorID]->GetCharacter()->Team();

		if(Team == TEAM_SUPER)
			return;

		CNetObj_SwitchState *pSwitchState = Server()->SnapNewItem<CNetObj_SwitchState>(Team);
		if(!pSwitchState)
			return;

		pSwitchState->m_HighestSwitchNumber = clamp((int)GameServer()->Switchers().size() - 1, 0, 255);
		mem_zero(pSwitchState->m_aStatus, sizeof(pSwitchState->m_aStatus));

		std::vector<std::pair<int, int>> vEndTicks; // <EndTick, SwitchNumber>

		for(int i = 0; i <= pSwitchState->m_HighestSwitchNumber; i++)
		{
			int Status = (int)GameServer()->Switchers()[i].m_aStatus[Team];
			pSwitchState->m_aStatus[i / 32] |= (Status << (i % 32));

			int EndTick = GameServer()->Switchers()[i].m_aEndTick[Team];
			if(EndTick > 0 && EndTick < Server()->Tick() + 3 * Server()->TickSpeed() && GameServer()->Switchers()[i].m_aLastUpdateTick[Team] < Server()->Tick())
			{
				// only keep track of EndTicks that have less than three second left and are not currently being updated by a player being present on a switch tile, to limit how often these are sent
				vEndTicks.emplace_back(GameServer()->Switchers()[i].m_aEndTick[Team], i);
			}
		}

		// send the endtick of switchers that are about to toggle back (up to four, prioritizing those with the earliest endticks)
		mem_zero(pSwitchState->m_aSwitchNumbers, sizeof(pSwitchState->m_aSwitchNumbers));
		mem_zero(pSwitchState->m_aEndTicks, sizeof(pSwitchState->m_aEndTicks));

		std::sort(vEndTicks.begin(), vEndTicks.end());
		const int NumTimedSwitchers = minimum((int)vEndTicks.size(), (int)std::size(pSwitchState->m_aEndTicks));

		for(int i = 0; i < NumTimedSwitchers; i++)
		{
			pSwitchState->m_aSwitchNumbers[i] = vEndTicks[i].second;
			pSwitchState->m_aEndTicks[i] = vEndTicks[i].first;
		}
	}
}

int IGameController::GetAutoTeam(int NotThisID)
{
	// this will force the auto balancer to work overtime as well
#ifdef CONF_DEBUG
	if(g_Config.m_DbgStress)
		return 0;
#endif

	int aNumplayers[2] = {0, 0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	int Team = 0;

	if(CanJoinTeam(Team, NotThisID))
		return Team;
	return -1;
}

bool IGameController::CanJoinTeam(int Team, int NotThisID)
{
	if(Team == TEAM_SPECTATORS || (GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() != TEAM_SPECTATORS))
		return true;

	int aNumplayers[2] = {0, 0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	return (aNumplayers[0] + aNumplayers[1]) < Server()->MaxClients() - g_Config.m_SvSpectatorSlots;
}

int IGameController::ClampTeam(int Team)
{
	if(Team < 0)
		return TEAM_SPECTATORS;
	return 0;
}

CClientMask IGameController::GetMaskForPlayerWorldEvent(int Asker, int ExceptID)
{
	// Send all world events to everyone by default
	return CClientMask().set().reset(ExceptID);
}

void IGameController::DoTeamChange(CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	Team = ClampTeam(Team);
	if(Team == pPlayer->GetTeam())
		return;

	pPlayer->SetTeam(Team);
	int ClientID = pPlayer->GetCID();

	char aBuf[128];
	DoChatMsg = false;
	if(DoChatMsg)
	{
		str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(ClientID), GameServer()->m_pController->GetTeamName(Team));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}

	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", ClientID, Server()->ClientName(ClientID), Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// OnPlayerInfoChange(pPlayer);
}
