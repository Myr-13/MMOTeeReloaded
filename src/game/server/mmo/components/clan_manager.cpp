#include "clan_manager.h"

#include <engine/server/server.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <engine/server/databases/connection.h>

CDbConnectionPool *CClanManager::DBPool() { return ((CServer *)Server())->DbPool(); }

bool CClanManager::CreateClanThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const SClanCreateRequest *pData = dynamic_cast<const SClanCreateRequest *>(pGameData);
	SClanCreateResult *pResult = dynamic_cast<SClanCreateResult *>(pGameData->m_pResult.get());

	char aBuf[1024];
	int NumInserted;
	bool End;

	// Check for existed clan or already clan
	str_copy(aBuf, "SELECT COUNT(*) AS clans_count FROM clans WHERE name = ? OR leader_id = ?");
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		return true;
	pSqlServer->BindString(1, pData->m_aClanName);
	pSqlServer->BindInt(2, pData->m_OwnerAccID);

	if(pSqlServer->Step(&End, pError, ErrorSize))
		return true;
	if(pSqlServer->GetInt(1) != 0)
	{
		str_format(pResult->m_aMessage, sizeof(pResult->m_aMessage), "This name is already taken or you already have clan.");
		return false;
	}

	// Create new clan
	str_copy(aBuf, "INSERT INTO clans(name, leader_id) VALUES(?, ?)");

	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		return true;
	pSqlServer->BindString(1, pData->m_aClanName);
	pSqlServer->BindInt(2, pData->m_OwnerAccID);

	if(pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
		return true;

	// Get clan id by name
	str_copy(aBuf, "SELECT id FROM clans WHERE name = ?");
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		return true;
	pSqlServer->BindString(1, pData->m_aClanName);

	if(pSqlServer->Step(&End, pError, ErrorSize))
		return true;

	pResult->m_ClanID = pSqlServer->GetInt(1);
	str_copy(pResult->m_aMessage, "Your clan successfully created!");
	return false;
}

void CClanManager::CreateClan(int ClientID, const char *pClanName)
{
	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];
	if (!pPly || !pPly->m_LoggedIn)
		return;

	auto pResult = std::make_shared<SClanCreateResult>();
	pResult->m_ClientID = ClientID;
	m_vpClanCreateResults.push_back(pResult);

	auto pRequest = std::make_unique<SClanCreateRequest>(pResult);
	str_copy(pRequest->m_aClanName, pClanName);
	pRequest->m_OwnerAccID = pPly->m_AccData.m_ID;
	DBPool()->Execute(CreateClanThread, std::move(pRequest), "Create clan");
}

bool CClanManager::DeleteClanThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	return false;
}

void CClanManager::DeleteClan(int ClientID)
{

}

void CClanManager::ChatCreateClan(IConsole::IResult *pResult, void *pUserData)
{
	((CClanManager *)pUserData)->CreateClan(pResult->m_ClientID, pResult->GetString(0));
}

void CClanManager::ChatDeleteClan(IConsole::IResult *pResult, void *pUserData)
{
	((CClanManager *)pUserData)->DeleteClan(pResult->m_ClientID);
}

void CClanManager::OnConsoleInit()
{

}

void CClanManager::OnTick()
{
	for (unsigned long i = 0; i < m_vpClanCreateResults.size(); i++)
	{
		auto &pResult = m_vpClanCreateResults[i];

		if (!pResult->m_Completed)
			continue;

		CPlayer *pPly = GameServer()->m_apPlayers[pResult->m_ClientID];
		if (!pPly || !pPly->m_LoggedIn)
			continue;

		if (pResult->m_aMessage[0] != '\0')
			GameServer()->SendChatTarget(pResult->m_ClientID, pResult->m_aMessage);

		if (pResult->m_Success == SClanResultBase::STATE_SUCCESSFUL)
		{
			pPly->m_AccData.m_ClanID = pResult->m_ClanID;
		}

		m_vpClanCreateResults.erase(m_vpClanCreateResults.begin() + i);
	}
}
