#include "weapon_base.h"

#include <game/server/gamecontext.h>

CGameContext *CWeaponBase::GameServer() const { return m_pGameServer; }
CCharacter *CWeaponBase::Owner() const { return m_pOwner; }
CGameWorld *CWeaponBase::GameWorld() const { return &m_pGameServer->m_World; }
CConfig *CWeaponBase::Config() const { return m_pGameServer->Config(); }
IServer *CWeaponBase::Server() const { return m_pGameServer->Server(); }
CCollision *CWeaponBase::Collision() const { return m_pGameServer->Collision(); }
CMMOCore *CWeaponBase::MMOCore() const { return &m_pGameServer->m_MMOCore; }
