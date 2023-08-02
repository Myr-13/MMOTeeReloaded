#ifndef GAME_SERVER_MMO_MMO_CORE_H
#define GAME_SERVER_MMO_MMO_CORE_H

#include <base/vmath.h>
#include <engine/shared/protocol.h>
#include <game/server/teeinfo.h>

#include "mmo_core_structs.h"

#include "components/account_manager.h"

enum
{
	BOT_IDS_OFFSET = 24
};

class CGameContext;

class CMMOCore
{
	CGameContext *m_pGameServer;

	int m_aBotSnapIDs[MAX_CLIENTS];

	std::vector<SInvItem> m_vItems;
	std::vector<SShopEntry> m_vShopItems;
	std::vector<SBotData> m_vBotsData;
	std::vector<SArmorData> m_vArmorsData;
	std::vector<SCraftData> m_vCrafts;
	std::vector<SPetData> m_vPetsData;

	SInvItem *GetItem(int ItemID);

public:
	CGameContext *GameServer() { return m_pGameServer; }
	class IServer *Server();
	class CGameWorld *GameWorld();

	void Init(CGameContext *pGameServer);

	// Functions for load data from xml
	void InitItems();
	void InitMobs();
	void InitArmor();
	void InitPets();
	void InitCrafts();
	void InitShop();

	void GetProgressBar(char *pStr, int StrSize, char Filler, char Empty, int Num, int MaxNum);

	// Dummies / Bots
	int GetNextBotSnapID(int ClientID);
	void CreateDummy(vec2 Pos, int DummyType, int DummyAIType);
	void CreateDummy(vec2 Pos, SBotData Data);
	void ClearBotSnapIDs();
	void OnMapBotPoint(vec2 Pos, const char *pPointName);

	// Items: get info
	const char *GetItemName(int ItemID);
	int GetItemType(int ItemID);
	int GetItemRarity(int ItemID);
	bool IsItemNotDroppable(int ItemID);
	int GetItemMaxCount(int ItemID);
	const char *GetQualityString(int Quality);
	const char *GetRarityString(int Rarity);

	// Items: set info
	bool GiveItem(int ClientID, int ItemID, int Count = 1, int Quality = QUALITY_0, int Data = 0);
	void UseItem(int ClientID, int ItemID, int Count);
	void BuyItem(int ClientID, int ItemID);
	std::vector<SShopEntry> &GetShopItems() { return m_vShopItems; }
	int GetEquippedItem(int ClientID, int ItemType);
	void SetEquippedItem(int ClientID, int ItemID, bool Equipped);
	void DropItem(int ClientID, int ItemID, int Count);

	// Upgrades
	const char *GetUpgradeName(int UpgradeID);
	int GetUpgradeCost(int UpgradeID);

	// Works
	int _GetMinerLevelID(int Level);
	int GetRandomMinerItemByLevel(int Level);
	int GetExpForLevelUp(int Level);
	int GetExpForLevelUpWork(int WorkID, int Level);
	const char *GetWorkName(int WorkID);

	// Damage
	int GetPlusDamage(int ClientID);

	// Armor
	int ArmorColor(int ItemID);
	int ArmorHealth(int ItemID);
	int ArmorDefense(int ItemID);
	void ResetTeeInfo(int ClientID);

	// Craft
	void CraftItem(int ClientID, int ItemID, int Count);
	std::vector<SCraftData> &GetCrafts() { return m_vCrafts; };

	// Pet
	CTeeInfo PetTeeInfo(int ItemID);
};

#endif // GAME_SERVER_MMO_MMO_CORE_H
