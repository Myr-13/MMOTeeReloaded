#include "clan_manager.h"

#include <engine/server/server.h>
#include <engine/shared/config.h>
#include <engine/server/databases/connection.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>

CDbConnectionPool *CClanManager::DBPool() { return ((CServer *)Server())->DbPool(); }

bool CClanManager::CreateClanThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const SClanCreateRequest *pData = dynamic_cast<const SClanCreateRequest *>(pGameData);
	SClanCreateResult *pResult = dynamic_cast<SClanCreateResult *>(pGameData->m_pResult.get());

	char aBuf[1024];
	int NumInserted;
	bool End;

	// Check for existed clan name
	str_copy(aBuf, "SELECT COUNT(*) FROM clans WHERE name = ?");
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		return true;
	pSqlServer->BindString(1, pData->m_aClanName);

	if(pSqlServer->Step(&End, pError, ErrorSize))
		return true;
	if(pSqlServer->GetInt(1) != 0)
	{
		str_format(pResult->m_aMessage, sizeof(pResult->m_aMessage), "This clan name is already taken.");
		return false;
	}

	// Create new clan
	str_copy(aBuf, "INSERT INTO clans(name, leader_id) VALUES(?, ?)");

	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		return true;
	pSqlServer->BindString(1, pData->m_aClanName);
	pSqlServer->BindInt(2, pData->m_LeaderID);

	if(pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
		return true;

	// Get ID by name
	str_copy(aBuf, "SELECT id FROM clans WHERE name = ?");
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		return true;
	pSqlServer->BindString(1, pData->m_aClanName);

	if(pSqlServer->Step(&End, pError, ErrorSize))
		return true;
	int ClanID = pSqlServer->GetInt(1);

	// Update clan_id for leader
	str_copy(aBuf, "UPDATE users SET clan_id = ? WHERE id = ?");

	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		return true;
	pSqlServer->BindInt(1, ClanID);
	pSqlServer->BindInt(2, pData->m_LeaderID);

	if(pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
		return true;

	str_format(pResult->m_aMessage, sizeof(pResult->m_aMessage), "Clan created successfully.");
	pResult->m_State = SClanResultBase::STATE_SUCCESSFUL;
	pResult->m_ClanID = ClanID;

	return false;
}

void CClanManager::CreateClan(int ClientID, const char *pClanName)
{
	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];

	if(!pPly->m_LoggedIn)
	{
		GameServer()->SendChatTarget(ClientID, "Login first!");
		return;
	}

	if(pPly->m_AccData.m_ClanID != 0)
	{
		GameServer()->SendChatTarget(ClientID, "You already in clan. Use /clan_leave for leave from clan.");
		return;
	}

	auto pResult = std::make_shared<SClanCreateResult>();
	pResult->m_ClientID = ClientID;
	str_copy(pResult->m_aClanName, pClanName);
	m_vpClanCreateResults.push_back(pResult);

	auto Request = std::make_unique<SClanCreateRequest>(pResult);
	str_copy(Request->m_aClanName, pClanName);
	Request->m_LeaderID = GameServer()->m_apPlayers[ClientID]->m_AccData.m_ID;
	DBPool()->Execute(CreateClanThread, std::move(Request), "Create clan");
}

bool CClanManager::DeleteClanThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	return false;
}

void CClanManager::DeleteClan(int ClientID, const char *pClanName)
{
	auto pResult = std::make_shared<SClanDeleteResult>();
	pResult->m_ClientID = ClientID;
	m_vpClanDeleteResults.push_back(pResult);

	auto Request = std::make_unique<SClanDeleteRequest>(pResult);
	str_copy(Request->m_aClanName, pClanName);
	DBPool()->Execute(DeleteClanThread, std::move(Request), "Delete clan");
}

bool CClanManager::LoadClansThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const SClansLoadRequest *pData = dynamic_cast<const SClansLoadRequest *>(pGameData);
	SClansLoadResult *pResult = dynamic_cast<SClansLoadResult *>(pGameData->m_pResult.get());

	char aBuf[64];
	
	// Load all clans
	str_copy(aBuf, "SELECT * FROM clans");
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		return true;

	bool End = false;
	while(!pSqlServer->Step(&End, pError, ErrorSize) && !End)
	{
		SClanData Clan;

		Clan.m_ID = pSqlServer->GetInt(1);
		pSqlServer->GetString(2, Clan.m_aClanName, MAX_CLAN_LENGTH);
		Clan.m_LeaderID = pSqlServer->GetInt(3);
		Clan.m_Level = pSqlServer->GetInt(4);
		Clan.m_Exp = pSqlServer->GetInt(5);
		Clan.m_MaxNum = pSqlServer->GetInt(6);
		Clan.m_Money = pSqlServer->GetInt(7);
		Clan.m_MoneyAdd = pSqlServer->GetInt(8);
		Clan.m_ExpAdd = pSqlServer->GetInt(9);
		Clan.m_SpawnHouse = pSqlServer->GetInt(10);
		Clan.m_ChairHouse = pSqlServer->GetInt(11);
		Clan.m_HouseID = pSqlServer->GetInt(12);

		pResult->m_vClansData.push_back(Clan);
	}

	return false;
}

void CClanManager::LoadClans()
{
	m_pClansLoadResult = std::make_shared<SClansLoadResult>();

	auto Request = std::make_unique<SClansLoadRequest>(m_pClansLoadResult);
	DBPool()->Execute(LoadClansThread, std::move(Request), "Load clans");
}

void CClanManager::ChatCreateClan(IConsole::IResult *pResult, void *pUserData)
{
	CClanManager *pThis = (CClanManager *)pUserData;

	const char *pName = pResult->GetString(0);

	if(str_length(pName) + 1 > MAX_CLAN_LENGTH)
	{
		pThis->GameServer()->SendChatTarget(pResult->m_ClientID, "Clan name is to long");
		return;
	}

	pThis->CreateClan(pResult->m_ClientID, pName);
}

void CClanManager::ChatDeleteClan(IConsole::IResult *pResult, void *pUserData)
{
	CClanManager *pThis = (CClanManager *)pUserData;

	const char *pName = pResult->GetString(0);

	if(str_length(pName) + 1 > MAX_CLAN_LENGTH)
	{
		pThis->GameServer()->SendChatTarget(pResult->m_ClientID, "Clan name is to long");
		return;
	}

	pThis->DeleteClan(pResult->m_ClientID, pName);
}

void CClanManager::LeaveClan(int ClientID)
{
	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];

	if(!pPly || !pPly->m_LoggedIn)
		return;

	pPly->m_AccData.m_ClanID = 0;
	GameServer()->SendChatTarget(ClientID, "You have left from clan.");
}

void CClanManager::ChatLeaveClan(IConsole::IResult *pResult, void *pUserData)
{
	((CClanManager *)pUserData)->LeaveClan(pResult->m_ClientID);
}

void CClanManager::OnConsoleInit()
{
	Console()->Register("clan_create", "s[name]", CFGFLAG_SERVER | CFGFLAG_CHAT, ChatCreateClan, this, "Create clan");
	Console()->Register("clan_delete", "s[name]", CFGFLAG_SERVER | CFGFLAG_CHAT, ChatDeleteClan, this, "Delete your clan");
	Console()->Register("clan_leave", "", CFGFLAG_SERVER | CFGFLAG_CHAT, ChatLeaveClan, this, "Leave from clan");

	LoadClans();
}

void CClanManager::OnTick()
{
	for (unsigned long i = 0; i < m_vpClanCreateResults.size(); i++)
	{
		auto &pResult = m_vpClanCreateResults[i];

		if (!pResult->m_Completed)
			continue;
		CPlayer *pPly = GameServer()->m_apPlayers[pResult->m_ClientID];
		if (!pPly)
			continue;

		if (pResult->m_aMessage[0] != '\0')
			GameServer()->SendChatTarget(pResult->m_ClientID, pResult->m_aMessage);

		if (pResult->m_State == SAccountResultBase::STATE_SUCCESSFUL)
		{
			pPly->m_AccData.m_ClanID = pResult->m_ClanID;

			// Create clan in cache
			SClanData Clan;
			Clan.m_ID = pResult->m_ClanID;
			str_copy(Clan.m_aClanName, pResult->m_aClanName);
			Clan.m_LeaderID = pPly->m_AccData.m_ID;
			Clan.m_Level = 1;
			Clan.m_Exp = 0;
			Clan.m_MaxNum = 2;
			Clan.m_Money = 0;
			Clan.m_MoneyAdd = 0;
			Clan.m_ExpAdd = 0;
			Clan.m_SpawnHouse = 0;
			Clan.m_ChairHouse = 0;
			Clan.m_HouseID = -1;

			m_vClansData.push_back(Clan);
		}

		m_vpClanCreateResults.erase(m_vpClanCreateResults.begin() + i);
	}

	// Check if load clans completed
	{
		if (!m_pClansLoadResult || !m_pClansLoadResult->m_Completed)
			return;
		
		m_vClansData = m_pClansLoadResult->m_vClansData;

		dbg_msg("clans", "loaded %ld clans", m_vClansData.size());

		m_pClansLoadResult = nullptr;
	}
}

SClanData *CClanManager::GetClan(const char *pName)
{
	for(SClanData &Clan : m_vClansData)
		if(str_comp(Clan.m_aClanName, pName) == 0)
			return &Clan;

	return 0x0;
}

SClanData *CClanManager::GetClan(int ID)
{
	for(SClanData &Clan : m_vClansData)
		if(Clan.m_ID == ID)
			return &Clan;

	return 0x0;
}

int CClanManager::GetMoneyForUpgrade(int UpgradeID, int UpgradeCount)
{
	UpgradeCount += 1;

	switch(UpgradeID)
	{
	case CLAN_UPGRADE_MAX_NUMBER: return 1250 * UpgradeCount;
	case CLAN_UPGRADE_ADD_MONEY:
	case CLAN_UPGRADE_ADD_EXP: return 1000 * UpgradeCount;
	case CLAN_UPGRADE_SPAWN_HOUSE: return 15000;
	case CLAN_UPGRADE_CHAIRS: return 1500 * UpgradeCount;
	}
}
