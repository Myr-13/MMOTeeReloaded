#ifndef GAME_SERVER_MMO_WEAPONS_HAMMER_H
#define GAME_SERVER_MMO_WEAPONS_HAMMER_H

#include "weapon_base.h"

class CHammer : public CWeaponBase
{
public:
	virtual int SnappingWeapon();
	virtual const char *WeaponName();
	virtual bool AutoFire();
	virtual bool WillFire();
	virtual float FireDelay();
	virtual int WeaponID();

	virtual void OnPrimaryAttack() override;
};

#endif // GAME_SERVER_MMO_WEAPONS_HAMMER_H
