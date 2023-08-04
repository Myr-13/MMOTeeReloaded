#ifndef GAME_SERVER_MMO_DUMMIES_PETS_PET_H
#define GAME_SERVER_MMO_DUMMIES_PETS_PET_H

#include <game/server/mmo/dummies/dummy_controller.h>

class CPet : public CDummyController
{
	int m_Dir;

	int m_OldDir;
	vec2 m_OldPos;

	void Move();
	void TargetMobs();

public:
	CPet();

	int m_Owner;
	int m_ItemID;

	void Tick() override;
	void Init() override;
};

#endif // GAME_SERVER_MMO_DUMMIES_PETS_PET_H
