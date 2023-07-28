#include "pet.h"

#include <game/server/mmo/dummies/dummy_base.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <game/server/entities/character.h>

CPet::CPet()
{
	m_Owner = -1;
	m_Target = -1;
	m_Dir = 0;
	m_OldDir = 0;
}

void CPet::Move()
{
	CPlayer *pPly = GameServer()->m_apPlayers[m_Owner];
	CCharacter *pChr = pPly->GetCharacter();

	m_Dir = 0;

	if(!pChr)
		return;

	// Move X
	if(abs(m_Pos.x - pChr->m_Pos.x) > 128.f)
		m_Dir = (m_Pos.x > pChr->m_Pos.x) ? -1 : 1;

	// Jump
	float YDist = pChr->m_Pos.y - m_Pos.y;
	if(YDist < -64 && YDist > -96)
		Jump();

	// Teleport if pet away
	float dist = distance(m_Pos, pChr->m_Pos);
	if(dist > 32.f * 32.f)
	{
		m_pDummyBase->Core()->m_Pos = pChr->m_Pos;
		m_pDummyBase->Core()->m_Vel = vec2(0, 0);
	}

	// Jump if pet stuck
	if(m_OldDir == m_Dir && m_OldPos == m_Pos && m_Dir != 0)
		m_AntiStuckTicks++;
	else
		m_AntiStuckTicks = 0;

	if(m_AntiStuckTicks)
	{
		Jump();
		m_AntiStuckTicks = 0;
	}

	// Change view
	if(m_Dir == -1)
		SetAim(-10, 0);
	if(m_Dir == 1)
		SetAim(10, 0);

	// Write old vars
	m_OldPos = m_Pos;
	m_OldDir = m_Dir;
}

void CPet::TargetMobs()
{
	CEntity *pEnt = GameWorld()->ClosestEntity(m_Pos, 320.f, CGameWorld::ENTTYPE_DUMMY, m_pDummyBase);
	if(!pEnt)
		return;
	if(!((CDummyBase *)pEnt)->IsAlive())
		return;

	// Fire
	SetWeapon(WEAPON_GUN);
	SetAim(pEnt->m_Pos - m_Pos);

	if(Server()->Tick() % 8 == 0)
		Fire();
}

void CPet::Tick()
{
	// Meh
	if(!m_pDummyBase->Core()->m_CollisionDisabled)
	{
		m_pDummyBase->Core()->m_HookHitDisabled = true;
		m_pDummyBase->Core()->m_CollisionDisabled = true;
	}

	// Some health :D
	m_pDummyBase->m_Health = 10000000;
	m_pDummyBase->m_NoDamage = true;

	// Check for valid player
	if(m_Owner != -1)
	{
		CPlayer *pPly = GameServer()->m_apPlayers[m_Owner];
		if(!pPly)
		{
			m_pDummyBase->m_MarkedForDestroy = true;
			m_Owner = -1;
			return;
		}
	}

	// Logic
	if(m_Owner != -1)
		Move();
	TargetMobs();

	SetMove(m_Dir);
}
