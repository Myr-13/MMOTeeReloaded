#include "vote_menu.h"

#include <cstdlib>

#include <cstdarg>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#ifdef __GNUC__
#define str_scan(Str, Format, ...) sscanf(Str, Format, __VA_ARGS__)
#else
#define str_scan(Str, Format, ...) sscanf_s(Str, Format, __VA_ARGS__)
#endif

#define CLICK "☞ "
#define VALUE "► "

CVoteMenu::CVoteMenu()
{
	for (int &i : m_aPlayersMenu)
		i = MENU_NO_AUTH;
}

void CVoteMenu::OnMessage(int ClientID, int MsgID, void *pRawMsg, bool InGame)
{
	if (MsgID != NETMSGTYPE_CL_CALLVOTE)
		return;

	CNetMsg_Cl_CallVote *pMsg = (CNetMsg_Cl_CallVote *)pRawMsg;

	char aDesc[VOTE_DESC_LENGTH] = {0};
	char aCmd[VOTE_CMD_LENGTH] = {0};

	if(!str_comp_nocase(pMsg->m_pType, "option"))
	{
		for (auto & i : m_aPlayersVotes[ClientID])
		{
			if(str_comp_nocase(pMsg->m_pValue, i.m_aDescription) == 0)
			{
				str_copy(aDesc, i.m_aDescription);
				str_format(aCmd, sizeof(aCmd), "%s", i.m_aCommand);
			}
		}
	}

	if(!str_comp(aCmd, "null"))
		return;

	// Handle cmds
	int Value1;

	if (str_scan(aCmd, "set%d", &Value1))
	{
		m_aPlayersMenu[ClientID] = Value1;
		RebuildMenu(ClientID);

		CCharacter *pChr = GameServer()->m_apPlayers[ClientID]->GetCharacter();
		if (pChr)
			GameServer()->CreateSound(pChr->m_Pos, SOUND_PICKUP_ARMOR);
	}
	else if (str_scan(aCmd, "inv_list%d", &Value1))
	{
		RebuildMenu(ClientID);
		AddMenuVote(ClientID, "null", "");
		ListInventory(ClientID, Value1);
	}
	else if (str_scan(aCmd, "inv_item%d", &Value1))
	{
		ItemInfo(ClientID, Value1);
	}
	else if (str_scan(aCmd, "inv_item_use%d", &Value1))
	{
		int Count = 1;
		try
		{
			Count = std::stoi(pMsg->m_pReason);
		} catch(std::exception &e) {}
		
		if (Count <= 0)
			return;

		m_aPlayersMenu[ClientID] = MENU_INVENTORY;
		RebuildMenu(ClientID);
		MMOCore()->UseItem(ClientID, Value1, Count);
	}
	else if (str_scan(aCmd, "inv_item_eqp%d", &Value1))
	{
		MMOCore()->SetEquippedItem(ClientID, Value1, MMOCore()->GetEquippedItem(ClientID, MMOCore()->GetItemType(Value1)) == -1);
		RebuildMenu(ClientID);
	}
	else if (str_scan(aCmd, "inv_item_drop%d", &Value1))
	{
		int Count = 1;
		try
		{
			Count = std::stoi(pMsg->m_pReason);
		} catch(std::exception &e) {}
		
		if (Count <= 0)
			return;

		m_aPlayersMenu[ClientID] = MENU_INVENTORY;
		RebuildMenu(ClientID);
		MMOCore()->DropItem(ClientID, Value1, Count);
	}
	else if (str_scan(aCmd, "upgr%d", &Value1))
	{
		int Count = 1;
		try
		{
			Count = std::stoi(pMsg->m_pReason);
		} catch(std::exception &e) {}
		
		if (Count <= 0)
			return;

		CPlayer *pPly = GameServer()->m_apPlayers[ClientID];

		int Cost = MMOCore()->GetUpgradeCost(Value1) * Count;
		if (Cost > pPly->m_AccUp.m_UpgradePoints)
		{
			GameServer()->SendChatLocalize(ClientID, "You don't have needed count of upgrade points.");
			return;
		}

		pPly->m_AccUp.m_UpgradePoints -= Cost;
		pPly->m_AccUp[Value1] += Count;

		RebuildMenu(ClientID);
	}
	else if (str_scan(aCmd, "shop%d", &Value1))
	{
		MMOCore()->BuyItem(ClientID, Value1);
		RebuildMenu(ClientID);
	}
	else if (str_scan(aCmd, "craft_list%d", &Value1))
	{
		ListCrafts(ClientID, Value1);
	}
	else if (str_scan(aCmd, "craft%d", &Value1))
	{
		int Count = 1;
		try
		{
			Count = std::stoi(pMsg->m_pReason);
		} catch(std::exception &e) {}
		
		if (Count <= 0)
			return;

		MMOCore()->CraftItem(ClientID, Value1, Count);
		RebuildMenu(ClientID);
	}
	else if(str_scan(aCmd, "cln_upgr_%d", &Value1))
	{
		// WARNING: BE CAREFULLY, WHEN ADDING NEW CLAN UPGRADES

		// Get player
		CPlayer *pPly = GameServer()->m_apPlayers[ClientID];
		int ClanID = pPly->m_AccData.m_ClanID;

		if(ClanID == 0)
			return;

		// Get clan
		SClanData *pClan = GameServer()->m_ClanManager.GetClan(ClanID);

		if(!pClan)
			return;

		// Check for leader
		if(pClan->m_LeaderID != pPly->m_AccData.m_ID)
		{
			GameServer()->SendChatLocalize(ClientID, "Only leader can buy upgrades for clan.");
			return;
		}

		int *pClanUpgrade = &pClan->m_MaxNum;
		int Cost = GameServer()->m_ClanManager.GetMoneyForUpgrade(Value1, pClanUpgrade[Value1]);

		if(pClan->m_Money < Cost)
		{
			GameServer()->SendChatLocalize(ClientID, "Your clan don't have enough money in bank.");
			return;
		}

		pClan->m_Money -= Cost;
		pClanUpgrade[Value1]++;

		GameServer()->SendChatLocalize(ClientID, "Upgrade successful!");

		RebuildMenu(ClientID);
	}
	else if(!str_comp(aCmd, "cln_add_money"))
	{
		// Get gold count
		int Count = 1;
		try
		{
			Count = std::stoi(pMsg->m_pReason);
		} catch(std::exception &e) {}

		// Get player
		CPlayer *pPly = GameServer()->m_apPlayers[ClientID];
		int ClanID = pPly->m_AccData.m_ClanID;

		if(ClanID == 0)
			return;

		// Get clan
		SClanData *pClan = GameServer()->m_ClanManager.GetClan(ClanID);

		if(!pClan)
			return;

		// Process adding
		Count = fmin(pPly->m_AccData.m_Money, Count);

		pPly->m_AccData.m_Money -= Count;
		pClan->m_Money += Count;

		GameServer()->SendChatLocalize(ClientID, "Money added to clan bank.");

		RebuildMenu(ClientID);
	}
}

void CVoteMenu::OnPlayerLeft(int ClientID)
{
	ClearVotes(ClientID);
	m_aPlayersMenu[ClientID] = MENU_NO_AUTH;
}

void CVoteMenu::AddMenuVote(int ClientID, const char *pCmd, const char *pDesc)
{
	int Len = str_length(pCmd);

	CVoteOptionServer Vote;
	str_copy(Vote.m_aDescription, pDesc, sizeof(Vote.m_aDescription));
	mem_copy(Vote.m_aCommand, pCmd, Len + 1);
	m_aPlayersVotes[ClientID].push_back(Vote);

	CNetMsg_Sv_VoteOptionAdd OptionMsg;
	OptionMsg.m_pDescription = Vote.m_aDescription;
	Server()->SendPackMsg(&OptionMsg, MSGFLAG_VITAL, ClientID);
}

void CVoteMenu::ClearVotes(int ClientID)
{
	m_aPlayersVotes[ClientID].clear();

	CNetMsg_Sv_VoteClearOptions ClearMsg;
	Server()->SendPackMsg(&ClearMsg, MSGFLAG_VITAL, ClientID);
}

void CVoteMenu::AddMenuChangeVote(int ClientID, int Menu, const char *pDesc)
{
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "set%d", Menu);
	AddMenuVote(ClientID, aBuf, pDesc);
}

void CVoteMenu::AddMenuChangeVoteLocalize(int ClientID, int Menu, const char *pDesc)
{
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "set%d", Menu);
	AddMenuVote(ClientID, aBuf, GameServer()->Localize(ClientID, pDesc));
}

void CVoteMenu::RebuildMenu(int ClientID)
{
	int Menu = m_aPlayersMenu[ClientID];

	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];
	if (!pPly)
		return;
	CCharacter *pChr = pPly->GetCharacter();

	ClearVotes(ClientID);

	if(Menu == MENU_NO_AUTH)
	{
		AddMenuVote(ClientID, "null", "Register and login if you want play");
		AddMenuVote(ClientID, "null", "After login press 'Join game' button");
	}
	else if(Menu == MENU_MAIN)
	{
		AddMenuVoteLocalize(ClientID, "null", "------------ Your stats");
		AddMenuVoteLocalize(ClientID, "null", "☪ Account: %s", pPly->m_AccData.m_aAccountName);
		AddMenuVoteLocalize(ClientID, "null", "ღ Level: %d", pPly->m_AccData.m_Level);
		AddMenuVoteLocalize(ClientID, "null", "ღ EXP: %d", pPly->m_AccData.m_EXP);
		AddMenuVoteLocalize(ClientID, "null", "ღ Money: %d", pPly->m_AccData.m_Money);
		AddMenuVoteLocalize(ClientID, "null", "------------ Server");
		AddMenuChangeVoteLocalize(ClientID, MENU_INFO, "☞ Info");
		AddMenuVoteLocalize(ClientID, "null", "------------ Account menu");
		AddMenuChangeVoteLocalize(ClientID, MENU_EQUIP, "☞ Equipment");
		AddMenuChangeVoteLocalize(ClientID, MENU_INVENTORY, "☞ Inventory");
		AddMenuChangeVoteLocalize(ClientID, MENU_UPGRADE, "☞ Upgrade");

		AddMenuVoteLocalize(ClientID, "null", "------------ Clan menu");
		if(pPly->m_AccData.m_ClanID != 0)
		{
			AddMenuChangeVoteLocalize(ClientID, MENU_CLAN_INFO, "☞ Info");
			AddMenuChangeVoteLocalize(ClientID, MENU_CLAN_UPGRADE, "☞ Upgrade");
		}
		else
			AddMenuVoteLocalize(ClientID, "null", "You not in clan");

		if (pChr && pChr->m_InShop)
		{
			char aCmd[256];

			AddMenuVoteLocalize(ClientID, "null", "------------ Shop");

			for (SShopEntry Entry : MMOCore()->GetShopItems())
			{
				str_format(aCmd, sizeof(aCmd), "shop%d", Entry.m_ID);
				AddMenuVoteLocalize(ClientID, aCmd, "☞ %s", MMOCore()->GetItemName(Entry.m_ID));
				AddMenuVoteLocalize(ClientID, "null", "Cost: %d | Level: %d", Entry.m_Cost, Entry.m_Level);
			}
		}

		if (pChr && pChr->m_InCraft)
		{
			char aCmd[256];

			AddMenuVote(ClientID, "null", "------------ Craft");

			for (SCraftData &Entry : MMOCore()->GetCrafts())
			{
				str_format(aCmd, sizeof(aCmd), "craft%d", Entry.m_ID);
				AddMenuVoteLocalize(ClientID, aCmd, "☞ %s", MMOCore()->GetItemName(Entry.m_ID));

				for (SCraftIngredient Ingredient : Entry.m_vIngredients)
					AddMenuVoteLocalize(ClientID, "null", "- %s x%d", MMOCore()->GetItemName(Ingredient.m_ID), Ingredient.m_Count);

				AddMenuVote(ClientID, "null", "");
			}
		}
	}
	else if(Menu == MENU_INFO)
	{
		AddMenuVoteLocalize(ClientID, "null", "------------ Info about server");
		AddMenuVoteLocalize(ClientID, "null", "Code with ♥ by Myr, based on DDNet by DDNet staff");
		AddMenuVoteLocalize(ClientID, "null", "Idea and MMOTee azataz by Kurosio");
		AddMenuVoteLocalize(ClientID, "null", "Hosted by Tee 3D");
		AddMenuVoteLocalize(ClientID, "null", "Discord: https://discord.gg/3KrNyerWtx");

		AddBack(ClientID, MENU_MAIN);
	}
	else if(Menu == MENU_EQUIP)
	{
		AddMenuVoteLocalize(ClientID, "null", "------------ Your equipment");
		AddMenuVoteLocalize(ClientID, "inv_list6", "☞ Armor body"); // ITEM_TYPE_ARMOR_BODY = 6
		AddMenuVoteLocalize(ClientID, "inv_list7", "☞ Armor feet"); // ITEM_TYPE_ARMOR_FEET = 7

		AddBack(ClientID, MENU_MAIN);
	}
	else if(Menu == MENU_INVENTORY)
	{
		AddMenuVoteLocalize(ClientID, "null", "------------ Your inventory");
		AddMenuVoteLocalize(ClientID, "inv_list0", "☞ Profession");
		AddMenuVoteLocalize(ClientID, "inv_list1", "☞ Craft");
		AddMenuVoteLocalize(ClientID, "inv_list2", "☞ Use");
		AddMenuVoteLocalize(ClientID, "inv_list3", "☞ Artifact");
		AddMenuVoteLocalize(ClientID, "inv_list4", "☞ Weapon");
		AddMenuVoteLocalize(ClientID, "inv_list5", "☞ Other");

		AddBack(ClientID, MENU_MAIN);
	}
	else if(Menu == MENU_UPGRADE)
	{
		AddMenuVote(ClientID, "null", "------------ Upgrades");
		char aBuf[128];
		char aCmd[64];

		AddMenuVoteLocalize(ClientID, "null", "► Upgrade points: %d", pPly->m_AccUp.m_UpgradePoints);

		for (int i = UPGRADE_DAMAGE; i < UPGRADES_NUM; i++)
		{
			str_format(aCmd, sizeof(aCmd), "upgr%d", i);
			AddMenuVoteLocalize(ClientID, aCmd, "☞ [%d] %s", pPly->m_AccUp[i], MMOCore()->GetUpgradeName(i));
		}

		AddBack(ClientID, MENU_MAIN);
	}
	else if(Menu == MENU_CLAN_INFO)
	{
		AddMenuVote(ClientID, "null", "------------ Clan info");

		char aBuf[128];

		SClanData *pClan = GameServer()->m_ClanManager.GetClan(pPly->m_AccData.m_ClanID);
		if(!pClan)
		{
			AddMenuVoteLocalize(ClientID, "null", "Something went wrong. Pointer to clan is null.");
			AddMenuVoteLocalize(ClientID, "null", "Contact administrator and send screenshot of this menu.");
			AddMenuVoteLocalize(ClientID, "null", "ClanID: %d", pPly->m_AccData.m_ClanID);
			AddMenuVoteLocalize(ClientID, "null", "AccID: %d", pPly->m_AccData.m_ID);
			AddMenuVoteLocalize(ClientID, "null", "Location: %s, %d", __FILE__, __LINE__);
			AddBack(ClientID, MENU_MAIN);

			return;
		}

		AddMenuVoteLocalize(ClientID, "null", "► Level: %d", pClan->m_Level);
		AddMenuVoteLocalize(ClientID, "null", "► Exp: %d", pClan->m_Exp);
		AddMenuVoteLocalize(ClientID, "null", "► Bank: %d", pClan->m_Money);

		AddMenuVote(ClientID, "null", "");
		AddMenuVoteLocalize(ClientID, "cln_add_money", "► Add money to bank");
		AddMenuVoteLocalize(ClientID, "null", "Reason = count");
		AddMenuVote(ClientID, "null", "");

		AddMenuVote(ClientID, "null", "TODO: Add members list");

		AddBack(ClientID, MENU_MAIN);
	}
	else if(Menu == MENU_CLAN_UPGRADE)
	{
		AddMenuVote(ClientID, "null", "------------ Clan upgrade");

		char aBuf[128];

		SClanData *pClan = GameServer()->m_ClanManager.GetClan(pPly->m_AccData.m_ClanID);
		if(!pClan)
		{
			AddMenuVoteLocalize(ClientID, "null", "Something went wrong. Pointer to clan is null.");
			AddMenuVoteLocalize(ClientID, "null", "Contact administrator and send screenshot of this menu.");
			AddMenuVoteLocalize(ClientID, "null", "ClanID: %d", pPly->m_AccData.m_ClanID);
			AddMenuVoteLocalize(ClientID, "null", "AccID: %d", pPly->m_AccData.m_ID);
			AddMenuVoteLocalize(ClientID, "null", "Location: %s, %d", __FILE__, __LINE__);
			AddBack(ClientID, MENU_MAIN);

			return;
		}

		AddMenuVoteLocalize(ClientID, "null", "► Bank: %d", pClan->m_Money);
		AddMenuVote(ClientID, "null", "");

		CClanManager *pClanMgr = &GameServer()->m_ClanManager;

		AddMenuVoteLocalize(ClientID, "cln_upgr_0", "☞ Max number of members: %d", pClan->m_MaxNum);
		AddMenuVoteLocalize(ClientID, "null", "► Cost: %d", pClanMgr->GetMoneyForUpgrade(CLAN_UPGRADE_MAX_NUMBER, pClan->m_MaxNum));

		AddMenuVote(ClientID, "null", "");

		AddMenuVoteLocalize(ClientID, "cln_upgr_1", "☞ Add money: %d", pClan->m_MoneyAdd);
		AddMenuVoteLocalize(ClientID, "null", "► Cost: %d", pClanMgr->GetMoneyForUpgrade(CLAN_UPGRADE_ADD_MONEY, pClan->m_MoneyAdd));
		AddMenuVoteLocalize(ClientID, "null", "Additional money for all members");

		AddMenuVote(ClientID, "null", "");

		AddMenuVoteLocalize(ClientID, "cln_upgr_2", "☞ Add exp: %d", pClan->m_ExpAdd);
		AddMenuVoteLocalize(ClientID, "null", "► Cost: %d", pClanMgr->GetMoneyForUpgrade(CLAN_UPGRADE_ADD_EXP, pClan->m_ExpAdd));
		AddMenuVoteLocalize(ClientID, "null", "Additional exp for all members");

		AddMenuVote(ClientID, "null", "");

		str_format(aBuf, sizeof(aBuf), CLICK "Spawn in house: %d", pClan->m_SpawnHouse);
		AddMenuVote(ClientID, "cln_upgr_3", aBuf);
		str_format(aBuf, sizeof(aBuf), VALUE "Cost: %d", pClanMgr->GetMoneyForUpgrade(CLAN_UPGRADE_SPAWN_HOUSE, pClan->m_SpawnHouse));
		AddMenuVote(ClientID, "null", aBuf);
		AddMenuVote(ClientID, "null", "Spawning in clan houses");
		AddMenuVote(ClientID, "null", "Available only in top 1 and top 2 clan houses");

		AddMenuVote(ClientID, "null", "");

		str_format(aBuf, sizeof(aBuf), CLICK "Chairs in house: %d", pClan->m_ChairHouse);
		AddMenuVote(ClientID, "cln_upgr_4", aBuf);
		str_format(aBuf, sizeof(aBuf), VALUE "Cost: %d", pClanMgr->GetMoneyForUpgrade(CLAN_UPGRADE_CHAIRS, pClan->m_ChairHouse));
		AddMenuVote(ClientID, "null", aBuf);
		AddMenuVote(ClientID, "null", "Give additional money and exp from chairs in clan house");

		AddBack(ClientID, MENU_MAIN);
	}
	else
	{
		AddMenuVote(ClientID, "null", "Woah, how you got here? O.o");
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Menu ID: %d", Menu);
		AddMenuVote(ClientID, "null", aBuf);

		AddBack(ClientID, MENU_MAIN);
	}
}

void CVoteMenu::ListInventory(int ClientID, int Type)
{
	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];

	if (pPly->m_AccInv.m_vItems.empty())
	{
		AddMenuVote(ClientID, "null", "Your inventory is empty");
		return;
	}

	char aBuf[256];
	char aCmd[64];
	for (SInvItem Item : pPly->m_AccInv.m_vItems)
	{
		if (Item.m_Type != Type)
			continue;

		str_format(aBuf, sizeof(aBuf), "%s x%d [%s] | %s",
			MMOCore()->GetItemName(Item.m_ID),
			Item.m_Count,
			MMOCore()->GetRarityString(Item.m_Rarity),
			MMOCore()->GetQualityString(Item.m_Quality));
		str_format(aCmd, sizeof(aCmd), "inv_item%d", Item.m_ID);

		AddMenuVote(ClientID, aCmd, aBuf);
	}
}

void CVoteMenu::AddBack(int ClientID, int Menu)
{
	AddMenuVote(ClientID, "null", "");
	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "set%d", Menu);
	AddMenuVote(ClientID, aBuf, "◄ Back ◄");
}

void CVoteMenu::ItemInfo(int ClientID, int ItemID)
{
	ClearVotes(ClientID);

	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];
	SInvItem Item = pPly->m_AccInv.GetItem(ItemID);
	int ItemType = MMOCore()->GetItemType(ItemID);
	char aBuf[256];
	char aBuf2[256];

	AddMenuVote(ClientID, "null", "Count = Reason");
	AddMenuVote(ClientID, "null", "------------ Item menu");
	str_format(aBuf, sizeof(aBuf), "Item: %s", MMOCore()->GetItemName(ItemID));
	AddMenuVote(ClientID, "null", aBuf);
	str_format(aBuf, sizeof(aBuf), "Count: %d", Item.m_Count);
	AddMenuVote(ClientID, "null", aBuf);
	str_format(aBuf, sizeof(aBuf), "Rarity: %s", MMOCore()->GetRarityString(Item.m_Rarity));
	AddMenuVote(ClientID, "null", aBuf);
	str_format(aBuf, sizeof(aBuf), "Quality: %s", MMOCore()->GetQualityString(Item.m_Quality));
	AddMenuVote(ClientID, "null", aBuf);
	AddMenuVote(ClientID, "null", "");

	if (ItemType == ITEM_TYPE_USE)
	{
		str_format(aBuf, sizeof(aBuf), "inv_item_use%d", ItemID);
		AddMenuVote(ClientID, aBuf, "☞ Use");
	}
	else if (ItemType == ITEM_TYPE_ARMOR_BODY || ItemType == ITEM_TYPE_ARMOR_FEET)
	{
		bool IsEquippedItem = (MMOCore()->GetEquippedItem(ClientID, ItemType) == ItemID);

		str_format(aBuf, sizeof(aBuf), "inv_item_eqp%d", ItemID);
		str_format(aBuf2, sizeof(aBuf2), "☞ %s Put %s", IsEquippedItem ? "☑" : "☐", IsEquippedItem ? "off" : "on");
		AddMenuVote(ClientID, aBuf, aBuf2);
	}

	str_format(aBuf, sizeof(aBuf), "inv_item_drop%d", ItemID);
	AddMenuVote(ClientID, aBuf, "☞ Drop");
	str_format(aBuf, sizeof(aBuf), "inv_item_dest%d", ItemID);
	AddMenuVote(ClientID, aBuf, "☞ Destroy");

	AddBack(ClientID, MENU_INVENTORY);
}

void CVoteMenu::ListCrafts(int ClientID, int Type)
{

}

void CVoteMenu::AddMenuVoteLocalize(int ClientID, const char *pCmd, const char *pDesc, ...)
{
	char aBuf[512];
	const char *pFormat = GameServer()->Localize(ClientID, pDesc);

#if defined(CONF_FAMILY_WINDOWS)
	va_list ap;
	va_start(ap, pDesc);
	_vsprintf_p(aBuf, sizeof(aBuf), pFormat, ap);
	va_end(ap);

	aBuf[sizeof(aBuf) - 1] = 0; /* assure null termination */
#else
	va_list ap;
	va_start(ap, pDesc);
	vsnprintf(aBuf, sizeof(aBuf), pFormat, ap);
	va_end(ap);

	/* null termination is assured by definition of vsnprintf */
#endif

	str_utf8_fix_truncation(aBuf);

	AddMenuVote(ClientID, pCmd, aBuf);
}
