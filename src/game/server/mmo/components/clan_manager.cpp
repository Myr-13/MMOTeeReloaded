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
		GameServer()->SendChatLocalize(ClientID, "You already in clan. Use /clan_leave for leave from clan.");
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

bool CClanManager::SaveClansThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const SClansSaveRequest *pData = dynamic_cast<const SClansSaveRequest *>(pGameData);

	char aBuf[256];
	int NumInserted;

	str_copy(aBuf, "UPDATE clans SET leader_id = ?, level = ?, exp = ?, max_num = ?, money = ?, money_add = ?, exp_add = ?, spawn_house = ?, chair_house = ?, house_id = ? WHERE id = ?");
	for(const SClanData &Clan : pData->m_vClans)
	{
		if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
			return true;

		pSqlServer->BindInt(1, Clan.m_LeaderID);
		pSqlServer->BindInt(2, Clan.m_Level);
		pSqlServer->BindInt(3, Clan.m_Exp);
		pSqlServer->BindInt(4, Clan.m_MaxNum);
		pSqlServer->BindInt(5, Clan.m_Money);
		pSqlServer->BindInt(6, Clan.m_MoneyAdd);
		pSqlServer->BindInt(7, Clan.m_ExpAdd);
		pSqlServer->BindInt(8, Clan.m_SpawnHouse);
		pSqlServer->BindInt(9, Clan.m_ChairHouse);
		pSqlServer->BindInt(10, Clan.m_HouseID);
		pSqlServer->BindInt(11, Clan.m_ID);

		if(pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
			return true;
	}

	return false;
}

void CClanManager::SaveClans()
{
	dbg_msg("clans", "saving %ld clans...", m_vClansData.size());

	auto Request = std::make_unique<SClansSaveRequest>();
	Request->m_vClans = m_vClansData;
	DBPool()->Execute(SaveClansThread, std::move(Request), "Save clans");
}

void CClanManager::ChatCreateClan(IConsole::IResult *pResult, void *pUserData)
{
	CClanManager *pThis = (CClanManager *)pUserData;

	const char *pName = pResult->GetString(0);

	if(str_length(pName) + 1 > MAX_CLAN_LENGTH)
	{
		pThis->GameServer()->SendChatLocalize(pResult->m_ClientID, "Clan name is to long.");
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
		pThis->GameServer()->SendChatLocalize(pResult->m_ClientID, "Clan name is to long.");
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
	GameServer()->SendChatLocalize(ClientID, "You have left from clan.");
}

void CClanManager::ChatLeaveClan(IConsole::IResult *pResult, void *pUserData)
{
	((CClanManager *)pUserData)->LeaveClan(pResult->m_ClientID);
}

bool CClanManager::GetClanMembersThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const SClanGetMembersRequest *pData = dynamic_cast<const SClanGetMembersRequest *>(pGameData);
	SClanGetMembersResult *pResult = dynamic_cast<SClanGetMembersResult *>(pGameData->m_pResult.get());

	char aBuf[128];
	
	// Load all clans
	str_copy(aBuf, "SELECT id, name, level FROM users WHERE clan_id = ?");
	if(pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		return true;

	pSqlServer->BindInt(1, pData->m_ClanID);

	bool End = false;
	while(!pSqlServer->Step(&End, pError, ErrorSize) && !End)
	{
		SAccountData Member;
		
		Member.m_ID = pSqlServer->GetInt(1);
		pSqlServer->GetString(2, Member.m_aAccountName, MAX_LOGIN_LENGTH);
		Member.m_Level = pSqlServer->GetInt(3);

		pResult->m_vMembers.push_back(Member);
	}

	return false;
}

void CClanManager::OnConsoleInit()
{
	Console()->Register("clan_create", "s[name]", CFGFLAG_SERVER | CFGFLAG_CHAT, ChatCreateClan, this, "Create clan");
	Console()->Register("clan_delete", "s[name]", CFGFLAG_SERVER | CFGFLAG_CHAT, ChatDeleteClan, this, "Delete your clan");
	Console()->Register("clan_leave", "", CFGFLAG_SERVER | CFGFLAG_CHAT, ChatLeaveClan, this, "Leave from clan");
	Console()->Register("clan_invite", "r[name]", CFGFLAG_SERVER | CFGFLAG_CHAT, ChatInviteClan, this, "Invite player to your clan");

	Console()->Register("save_clans", "", CFGFLAG_SERVER | CFGFLAG_GAME, ConSaveClans, this, "Save all clans (DO ALWAYS BEFORE SHUTDOWN)");

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

	for (unsigned long i = 0; i < m_vpClanGetMembersResults.size(); i++)
	{
		auto &pResult = m_vpClanGetMembersResults[i];

		if(!pResult->m_Completed)
			continue;
		if(!GameServer()->m_apPlayers[pResult->m_ClientID])
			continue;
		if(!GameServer()->m_apPlayers[pResult->m_ClientID2])
			continue;

		if(pResult->m_Type == SClanGetMembersResult::GET_MEMBERS_RESULT_INVITE)
		{
			InternalSendClanInvite(pResult->m_ClanID, pResult->m_vMembers.size(), pResult->m_ClientID, pResult->m_ClientID2);
		}
		else // GET_MEMBERS_RESULT_VOTES
		{

		}

		m_vpClanGetMembersResults.erase(m_vpClanGetMembersResults.begin() + i);
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

void CClanManager::OnMessage(int ClientID, int MsgID, void *pRawMsg, bool InGame)
{
	if(MsgID != NETMSGTYPE_CL_VOTE)
		return;

	CNetMsg_Cl_Vote *pMsg = (CNetMsg_Cl_Vote *)pRawMsg;
	int Vote = pMsg->m_Vote;

	if(Vote == 0)
		return;

	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];

	if(Server()->Tick() > pPly->m_ClanInviteEndTick)
		return;

	// Accept
	if(Vote == 1)
	{
		pPly->m_AccData.m_ClanID = pPly->m_ClanInviteID;

		// Save new clan id
		GameServer()->m_AccountManager.Save(ClientID);

		// Send the message
		GameServer()->SendChatLocalize(ClientID, "You accepted clan invite!");
	}
	else // Deceline
	{
		// 1 tick is a bypass
		pPly->m_ClanInviteEndTick = Server()->Tick() + 1;

		GameServer()->SendChatLocalize(ClientID, "You declined clan invite.");
	}
}

void CClanManager::OnShutdown()
{
	SaveClans();
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

	return INT_MAX;
}

void CClanManager::SendClanInvite(int From, int ClientID)
{
	CPlayer *pFrom = GameServer()->m_apPlayers[From];
	CPlayer *pTo = GameServer()->m_apPlayers[ClientID];

	if(!pFrom->m_LoggedIn || !pTo->m_LoggedIn)
		return;

	int ClanID = pFrom->m_AccData.m_ClanID;
	SClanData *pClan = GetClan(ClanID);

	if(!pClan)
		return;

	// Only leader can invite other players
	if(pFrom->m_AccData.m_ID != pClan->m_LeaderID)
	{
		GameServer()->SendChatLocalize(From, "Only leader can invite other players.");
		return;
	}

	// Check for player clan
	if(pTo->m_AccData.m_ClanID != 0)
	{
		GameServer()->SendChatLocalize(From, "This player is already in clan.");
		return;
	}

	// Create DB request
	auto pResult = std::make_shared<SClanGetMembersResult>();
	pResult->m_ClientID = From;
	pResult->m_ClientID2 = ClientID;
	pResult->m_ClanID = ClanID;
	pResult->m_Type = SClanGetMembersResult::GET_MEMBERS_RESULT_INVITE;
	m_vpClanGetMembersResults.push_back(pResult);

	auto pRequest = std::make_unique<SClanGetMembersRequest>(pResult);
	pRequest->m_ClanID = ClanID;
	DBPool()->Execute(GetClanMembersThread, std::move(pRequest), "Get clan members");
}

void CClanManager::InternalSendClanInvite(int ClanID, int MembersCount, int From, int To)
{
	SClanData *pClan = GetClan(ClanID);

	if(!pClan)
		return;

	if(MembersCount + 1 > pClan->m_MaxNum)
	{
		GameServer()->SendChatLocalize(From, "Your clan don't have empty slots.");
		return;
	}

	// Invite
	CPlayer *pPly = GameServer()->m_apPlayers[To];
	pPly->m_ClanInviteEndTick = Server()->Tick() + 50 * 10;
	pPly->m_ClanInviteID = ClanID;

	// Send vote
	CNetMsg_Sv_VoteSet Msg;

	Msg.m_Timeout = 10;
	Msg.m_pDescription = "Accept clan invite?";
	Msg.m_pReason = "";

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, To);
}

void CClanManager::ConSaveClans(IConsole::IResult *pResult, void *pUserData)
{
	((CClanManager *)pUserData)->SaveClans();
}

void CClanManager::ChatInviteClan(IConsole::IResult *pResult, void *pUserData)
{
	CClanManager *pThis = (CClanManager *)pUserData;
	int From = pResult->m_ClientID;
	int ClientID = -1;

	const char *pName = pResult->GetString(0);
	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		if(str_comp(pThis->Server()->ClientName(i), pName) == 0)
		{
			ClientID = i;
			break;
		}
	}

	if(ClientID == -1)
	{
		pThis->GameServer()->SendChatLocalize(From, "Can't find player with this name.");
		return;
	}

	pThis->SendClanInvite(From, ClientID);
}
