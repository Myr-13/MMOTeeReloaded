#ifndef GAME_SERVER_MMO_COMPONENTS_CLAN_MANAGER_H
#define GAME_SERVER_MMO_COMPONENTS_CLAN_MANAGER_H

#include <game/server/mmo/component.h>

#include <engine/console.h>
#include <engine/server/databases/connection_pool.h>
#include <game/server/mmo/clan_data.h>
#include <game/server/mmo/account_data.h>

struct SClanResultBase;
struct SClanCreateResult;
struct SClanDeleteResult;
struct SClansLoadResult;
struct SClanGetMembersResult;

enum
{
	CLAN_UPGRADE_MAX_NUMBER,
	CLAN_UPGRADE_ADD_MONEY,
	CLAN_UPGRADE_ADD_EXP,
	CLAN_UPGRADE_SPAWN_HOUSE,
	CLAN_UPGRADE_CHAIRS
};

class CClanManager : public CServerComponent
{
	std::vector<std::shared_ptr<SClanCreateResult>> m_vpClanCreateResults;
	std::vector<std::shared_ptr<SClanDeleteResult>> m_vpClanDeleteResults;
	std::vector<std::shared_ptr<SClanGetMembersResult>> m_vpClanGetMembersResults;
	std::shared_ptr<SClansLoadResult> m_pClansLoadResult;

	std::vector<SClanData> m_vClansData;

	// Chat commands
	static void ChatCreateClan(IConsole::IResult *pResult, void *pUserData);
	static void ChatDeleteClan(IConsole::IResult *pResult, void *pUserData);
	static void ChatLeaveClan(IConsole::IResult *pResult, void *pUserData);
	static void ChatInviteClan(IConsole::IResult *pResult, void *pUserData);

	// TODO: Create safe shutdown (saving all clans and shutdown server after)

	// Console commands
	static void ConSaveClans(IConsole::IResult *pResult, void *pUserData);

	// DB Threads
	static bool CreateClanThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool DeleteClanThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool LoadClansThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool SaveClansThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool GetClanMembersThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);

	CDbConnectionPool *DBPool();

	void LoadClans();
	void SaveClans();

	void InternalSendClanInvite(int ClanID, int MembersCount, int From, int To);

public:
	virtual void OnConsoleInit() override;
	virtual void OnTick() override;
	virtual void OnShutdown() override;
	virtual void OnMessage(int ClientID, int MsgID, void *pRawMsg, bool InGame) override;

	void CreateClan(int ClientID, const char *pClanName);
	void DeleteClan(int ClientID, const char *pClanName);
	void LeaveClan(int ClientID);

	SClanData *GetClan(const char *pName);
	SClanData *GetClan(int ID);

	int GetMoneyForUpgrade(int UpgradeID, int UpgradeCount);

	void SendClanInvite(int From, int ClientID);
};

struct SClanResultBase : ISqlResult
{
	SClanResultBase()
	{
		m_aMessage[0] = '\0';
		m_State = STATE_FAILED;
		m_ClientID = -1;
	}

	enum
	{
		STATE_FAILED = -1,
		STATE_SUCCESSFUL
	};

	int m_State;
	char m_aMessage[512];
	int m_ClientID;
};

struct SClanCreateResult : SClanResultBase
{
	SClanCreateResult()
	{
		m_aClanName[0] = '\0';
		m_ClanID = 0;
	}

	char m_aClanName[32];
	int m_ClanID;
};

struct SClanCreateRequest : ISqlData
{
	SClanCreateRequest(std::shared_ptr<SClanCreateResult> pResult) :
		ISqlData(std::move(pResult))
	{
		m_aClanName[0] = '\0';
		m_LeaderID = 0;
	}

	char m_aClanName[32];
	int m_LeaderID;
};

struct SClanDeleteResult : SClanResultBase
{
	SClanDeleteResult()
	{
		m_aClanName[0] = '\0';
	}

	char m_aClanName[32];
};

struct SClanDeleteRequest : ISqlData
{
	SClanDeleteRequest(std::shared_ptr<SClanDeleteResult> pResult) :
		ISqlData(std::move(pResult))
	{
		m_aClanName[0] = '\0';
	}

	char m_aClanName[32];
};

struct SClansLoadResult : ISqlResult
{
	SClansLoadResult() = default;

	std::vector<SClanData> m_vClansData;
};

struct SClansLoadRequest : ISqlData
{
	SClansLoadRequest(std::shared_ptr<SClansLoadResult> pResult) :
		ISqlData(std::move(pResult))
	{
	}
};

struct SClansSaveRequest : ISqlData
{
	SClansSaveRequest() :
		ISqlData(0x0)
	{
	}

	std::vector<SClanData> m_vClans;
};

struct SClanGetMembersResult : ISqlResult
{
	enum
	{
		GET_MEMBERS_RESULT_VOTES,
		GET_MEMBERS_RESULT_INVITE
	};

	SClanGetMembersResult()
	{
		m_Type = GET_MEMBERS_RESULT_INVITE;
		m_ClientID = -1;
		m_ClientID2 = -1;
		m_ClanID = 0;
	}

	std::vector<SAccountData> m_vMembers;
	int m_Type;
	int m_ClientID;
	int m_ClientID2;
	int m_ClanID;
};

struct SClanGetMembersRequest : ISqlData
{
	SClanGetMembersRequest(std::shared_ptr<SClanGetMembersResult> pResult) :
		ISqlData(std::move(pResult))
	{
		m_ClanID = 0;
	}

	int m_ClanID;
};

#endif // GAME_SERVER_MMO_COMPONENTS_CLAN_MANAGER_H
