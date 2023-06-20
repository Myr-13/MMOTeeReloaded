#ifndef GAME_SERVER_MMO_CLAN_DATA_H
#define GAME_SERVER_MMO_CLAN_DATA_H

#include <engine/shared/protocol.h>

struct SClanData
{
	char m_aClanName[MAX_CLAN_LENGTH];
	int m_ID;
	int m_LeaderID;
	int m_Level;
	int m_Exp;
	int m_MaxNum;
	int m_Money;
	int m_MoneyAdd;
	int m_ExpAdd;
	int m_SpawnHouse;
	int m_ChairHouse;
	int m_HouseID;
};

#endif // GAME_SERVER_MMO_CLAN_DATA_H
