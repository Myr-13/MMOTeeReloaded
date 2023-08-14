#ifndef GAME_SERVER_MMO_COMPONENTS_AUCTION_MANAGER_H
#define GAME_SERVER_MMO_COMPONENTS_AUCTION_MANAGER_H

#include <game/server/mmo/account_data.h>
#include <game/server/mmo/component.h>

struct SAuctionItem
{
	int m_ID;
	int m_ItemID;
	int m_SellerID;
	char m_aSellerName[MAX_LOGIN_LENGTH];
	int m_Cost;
	int64_t m_EndDate;
	int m_Mode;
	int m_BuyerID;
	char m_aBuyerName[MAX_LOGIN_LENGTH];
};

enum
{
	AUC_MODE_BID,
	AUC_MODE_AUCTION
};

class CAuctionManager : public CServerComponent
{
public:
	void OnInit() override;
	void OnShutdown() override;
	void OnTick() override;

	std::vector<SAuctionItem> m_vItems;

	void Create(int ClientID, SAuctionItem &Item);
	void Buy(int ClientID, int ID);

	int GetPlusCost(int64_t Time);

	SAuctionItem GetAuctionItem(int ID);
};

#endif // GAME_SERVER_MMO_COMPONENTS_AUCTION_MANAGER_H
