#ifndef GAME_SERVER_MMO_MMO_CORE_STRUCTS_H
#define GAME_SERVER_MMO_MMO_CORE_STRUCTS_H

#include <vector>
#include <game/server/teeinfo.h>

struct SShopEntry
{
	int m_ID;
	int m_Cost;
	int m_Level;
};

struct SBotLootData
{
	int m_ID;
	int m_MinCount;
	int m_MaxCount;
	int m_Chance;
};

struct SBotData
{
	int m_ID;
	char m_aName[MAX_NAME_LENGTH];
	CTeeInfo m_TeeInfo;
	char m_aSpawnPointName[16];
	int m_Level;
	int m_HP;
	int m_Armor;
	int m_Damage;
	int m_AIType;

	std::vector<SBotLootData> m_vLoot;
};

struct SArmorData
{
	int m_BodyID;
	int m_FeetID;
	int m_ColorBody;
	int m_ColorFeet;
	int m_Health;
	int m_Armor;
};

struct SCraftIngredient
{
	int m_ID;
	int m_Count;
};

struct SCraftData
{
	int m_ID;
	int m_Type;
	std::vector<SCraftIngredient> m_vIngredients;
};

struct SPetData
{
	int m_ID;
	CTeeInfo m_TeeInfo;
	int m_Damage;
	int m_PlusHealth;
	int m_PlusArmor;
};

#endif // GAME_SERVER_MMO_MMO_CORE_STRUCTS_H
