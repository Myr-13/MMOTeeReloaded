#include "vote_menu.h"

#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

CVoteMenu::CVoteMenu()
{
	for (int &i : m_aPlayersMenu)
		i = MENU_MAIN;
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

	if (sscanf_s(aCmd, "set%d", &Value1))
	{
		m_aPlayersMenu[ClientID] = Value1;
		RebuildMenu(ClientID);

		CCharacter *pChr = GameServer()->m_apPlayers[ClientID]->GetCharacter();
		if (pChr)
			GameServer()->CreateSound(pChr->m_Pos, SOUND_PICKUP_ARMOR);
	}
	else if (sscanf_s(aCmd, "inv_list%d", &Value1))
	{
		RebuildMenu(ClientID);
		AddMenuVote(ClientID, "null", "");
		ListInventory(ClientID, Value1);
	}
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

void CVoteMenu::RebuildMenu(int ClientID)
{
	int Menu = m_aPlayersMenu[ClientID];

	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];
	if (!pPly)
		return;

	ClearVotes(ClientID);

	if (Menu == MENU_MAIN)
	{
		AddMenuVote(ClientID, "null", "------------ Your stats");
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "☪ Account: %s", pPly->m_AccData.m_aAccountName);
		AddMenuVote(ClientID, "null", aBuf);
		str_format(aBuf, sizeof(aBuf), "ღ Level: %d", pPly->m_AccData.m_Level);
		AddMenuVote(ClientID, "null", aBuf);
		str_format(aBuf, sizeof(aBuf), "ღ EXP: %d", pPly->m_AccData.m_EXP);
		AddMenuVote(ClientID, "null", aBuf);
		str_format(aBuf, sizeof(aBuf), "ღ Money: %d", pPly->m_AccData.m_Money);
		AddMenuVote(ClientID, "null", aBuf);
		AddMenuVote(ClientID, "null", "------------ Server");
		AddMenuChangeVote(ClientID, MENU_INFO, "☞ Info");
		AddMenuVote(ClientID, "null", "------------ Account menu");
		AddMenuChangeVote(ClientID, MENU_EQUIP, "☞ Equipment");
		AddMenuChangeVote(ClientID, MENU_INVENTORY, "☞ Inventory");
		AddMenuChangeVote(ClientID, MENU_UPGRADE, "☞ Upgrade");
	}
	else if (Menu == MENU_INFO)
	{
		AddMenuVote(ClientID, "null", "------------ Info about server");
		AddMenuVote(ClientID, "null", "Code with ♥ by Myr, based on DDNet by DDNet staff");
		AddMenuVote(ClientID, "null", "Idea and MMOTee azataz by Kurosio");
		AddMenuVote(ClientID, "null", "Discord: https://discord.gg/3KrNyerWtx");

		AddBack(ClientID, MENU_MAIN);
	}
	else if (Menu == MENU_EQUIP)
	{
		AddMenuVote(ClientID, "null", "------------ Your equipment");

		AddBack(ClientID, MENU_MAIN);
	}
	else if (Menu == MENU_INVENTORY)
	{
		AddMenuVote(ClientID, "null", "------------ Your inventory");
		AddMenuVote(ClientID, "inv_list0", "☞ Profession");
		AddMenuVote(ClientID, "inv_list1", "☞ Craft");
		AddMenuVote(ClientID, "inv_list2", "☞ Use");
		AddMenuVote(ClientID, "inv_list3", "☞ Artifact");
		AddMenuVote(ClientID, "inv_list4", "☞ Weapon");
		AddMenuVote(ClientID, "inv_list5", "☞ Other");

		AddBack(ClientID, MENU_MAIN);
	}
	else if (Menu == MENU_UPGRADE)
	{
		AddMenuVote(ClientID, "null", "------------ Upgrades");

		AddBack(ClientID, MENU_MAIN);
	}
}

void CVoteMenu::ListInventory(int ClientID, int Type)
{
	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];
	CMMOCore *pMMOCore = &GameServer()->m_MMOCore;

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
			pMMOCore->GetItemName(Item.m_ID),
			Item.m_Count,
			pMMOCore->GetRarityString(Item.m_Rarity),
			pMMOCore->GetQualityString(Item.m_Quality));
		str_format(aCmd, sizeof(aCmd), "itm%d", Item.m_ID);

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
