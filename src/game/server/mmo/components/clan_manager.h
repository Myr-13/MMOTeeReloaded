#ifndef GAME_SERVER_MMO_COMPONENTS_CLAN_MANAGER_H
#define GAME_SERVER_MMO_COMPONENTS_CLAN_MANAGER_H

#include <game/server/mmo/component.h>

#include <engine/console.h>
#include <engine/server/databases/connection_pool.h>
#include <game/server/mmo/clan_data.h>

struct SClanResultBase;
struct SClanCreateResult;
struct SClanDeleteResult;
struct SClansLoadResult;

class CClanManager : public CServerComponent
{
	std::vector<std::shared_ptr<SClanCreateResult>> m_vpClanCreateResults;
	std::vector<std::shared_ptr<SClanDeleteResult>> m_vpClanDeleteResults;
	std::shared_ptr<SClansLoadResult> m_pClansLoadResult;

	std::vector<SClanData> m_vClansData;

	// Chat commands
	static void ChatCreateClan(IConsole::IResult *pResult, void *pUserData);
	static void ChatDeleteClan(IConsole::IResult *pResult, void *pUserData);

	// DB Threads
	static bool CreateClanThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool DeleteClanThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);
	static bool LoadClansThread(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize);

	CDbConnectionPool *DBPool();

	void LoadClans();

public:
	virtual void OnConsoleInit() override;
	virtual void OnTick() override;

	void CreateClan(int ClientID, const char *pClanName);
	void DeleteClan(int ClientID, const char *pClanName);
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

#endif // GAME_SERVER_MMO_COMPONENTS_CLAN_MANAGER_H
