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
	m_LastJumpTick = 0;
}

void CPet::Init()
{
	m_pDummyBase->Core()->m_HookHitDisabled = true;
	m_pDummyBase->Core()->m_CollisionDisabled = true;
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
	if(YDist < -64 && TicksPassed(15))
		Jump();

	// Teleport if pet too far
	float dist = distance(m_Pos, pChr->m_Pos);
	if(dist > 32.f * 32.f)
	{
		m_pDummyBase->Core()->m_Pos = pChr->m_Pos;
		m_pDummyBase->Core()->m_Vel = vec2(0, 0);
	}

	// Jump if pet stuck
	if(m_OldDir == m_Dir && m_OldPos == m_Pos && m_Dir != 0)
		Jump();

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
	CDummyBase *pEnt = (CDummyBase *)GameWorld()->ClosestEntity(m_Pos, 640.f, CGameWorld::ENTTYPE_DUMMY, m_pDummyBase);
	if(!pEnt)
		return;
	if(!pEnt->IsAlive())
		return;
	if(pEnt->GetDummyType() == DUMMY_TYPE_PET)
		return;
	if(Collision()->IntersectLine(m_Pos, pEnt->m_Pos, 0x0, 0x0))
		return;

	vec2 PredPos = pEnt->Core()->m_Pos + pEnt->Core()->m_Vel * 3.6f;

	// Fire
	SetWeapon(WEAPON_GUN);
	SetAim(PredPos - m_Pos);

	if(TicksPassed(8))
		Fire();
}

void CPet::Tick()
{
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
