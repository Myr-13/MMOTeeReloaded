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

	return false;
}

void CClanManager::CreateClan(int ClientID, const char *pClanName)
{
	if(!GameServer()->m_apPlayers[ClientID]->m_LoggedIn)
	{
		GameServer()->SendChatTarget(ClientID, "Login first!");
		return;
	}

	auto pResult = std::make_shared<SClanCreateResult>();
	pResult->m_ClientID = ClientID;
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

void CClanManager::OnConsoleInit()
{
	Console()->Register("clan_create", "s[name]", CFGFLAG_SERVER | CFGFLAG_CHAT, ChatCreateClan, this, "Create clan");
	Console()->Register("clan_delete", "s[name]", CFGFLAG_SERVER | CFGFLAG_CHAT, ChatDeleteClan, this, "Delete your clan");

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
		}

		m_vpClanCreateResults.erase(m_vpClanCreateResults.begin() + i);
	}
}
