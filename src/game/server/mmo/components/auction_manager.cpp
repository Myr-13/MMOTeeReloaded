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
}
