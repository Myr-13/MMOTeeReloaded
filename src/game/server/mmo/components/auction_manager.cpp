#include "auction_manager.h"

#include <game/server/gamecontext.h>
#include <game/server/player.h>

void CAuctionManager::OnInit()
{

}

void CAuctionManager::OnShutdown()
{

}

void CAuctionManager::OnTick()
{
	// Calculate every minute
	if(Server()->Tick() % (Server()->TickSpeed() * 60) != 0)
		return;


}

void CAuctionManager::Create(int ClientID, SAuctionItem &Item)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
		return;
	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];
	if(!pPly || !pPly->m_LoggedIn)
		return;

	int Cost = Item.m_Cost + GetPlusCost(Item.m_EndDate);

	if(pPly->m_AccData.m_Money < Cost)
	{
		GameServer()->SendChatLocalize(ClientID, "You don't have enough money");
		return;
	}

	m_vItems.push_back(Item);
}

void CAuctionManager::Buy(int ClientID, int ID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
		return;
	CPlayer *pPly = GameServer()->m_apPlayers[ClientID];
	if(!pPly || !pPly->m_LoggedIn)
		return;

	SAuctionItem Item = GetAuctionItem(ID);

	if(Item.m_ID == -1)
	{
		LOG_ERROR(ClientID, "Item already bought?")
		return;
	}

	if(pPly->m_AccData.m_Money < Item.m_Cost)
	{
		GameServer()->SendChatLocalize(ClientID, "You don't have enough money");
		return;
	}

	// Just buy
	if(Item.m_Mode == AUC_MODE_BID)
	{
		pPly->m_AccData.m_Money -= Item.m_Cost;
		MMOCore()->GiveItem(ClientID, Item.m_ItemID);
		RemoveAuctionItem(Item.m_ID);

		GameServer()->SendChatLocalize(ClientID, "Successful bought");
		return;
	}
}

int CAuctionManager::GetPlusCost(int64_t Time)
{
	return Time / 60;
}

SAuctionItem CAuctionManager::GetAuctionItem(int ID)
{
	for(auto &Item : m_vItems)
		if(Item.m_ID == ID)
			return Item;
	return {-1};
}

void CAuctionManager::RemoveAuctionItem(int ID)
{
	for(int i = 0; i < m_vItems.size(); i++)
	{
		if(m_vItems[i].m_ID == ID)
		{
			m_vItems.erase(m_vItems.begin() + i);
			return;
		}
	}
}
