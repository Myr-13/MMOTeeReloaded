#ifndef GAME_SERVER_MMO_WEAPONS_WEAPON_BASE_H
#define GAME_SERVER_MMO_WEAPONS_WEAPON_BASE_H

class CGameContext;
class CCharacter;

enum
{
	MMO_WEAPON_HAMMER
};

class CWeaponBase
{
protected:
	friend class CCharacter;

	CGameContext *m_pGameServer;
	CCharacter *m_pOwner;

	CGameContext *GameServer() const;
	CCharacter *Owner() const;
	class CGameWorld *GameWorld() const;
	class CConfig *Config() const;
	class IServer *Server() const;
	class CCollision *Collision() const;
	class CMMOCore *MMOCore() const;

public:
	virtual ~CWeaponBase() = default;
	virtual int SnappingWeapon() = 0;
	virtual const char *WeaponName() = 0;
	virtual bool AutoFire() = 0;
	virtual bool WillFire() = 0;
	virtual float FireDelay() = 0;
	virtual int WeaponID() = 0;

	/**
	 * Called on weapon initiation(weapon was given to player).
	 */
	virtual void OnInit() {}
	/**
	 * Called when player fireing with weapon.
	 */
	virtual void OnPrimaryAttack() {}
	/**
	 * Called when player using `/secondary_attack` command in chat.
	 */
	virtual void OnSecondaryAttack() {}
	/**
	 * Called every snapshot when weapon is picked up.
	 * @param SnappingClient ClientID who snapping.
	 */
	virtual void OnSnap(int SnappingClient) {}
	/**
	 * Called every tick when weapon is picked up.
	 * By default, 50 ticks per seconds
	 */
	virtual void OnTick() {}
};

#endif // GAME_SERVER_MMO_WEAPONS_WEAPON_BASE_H
