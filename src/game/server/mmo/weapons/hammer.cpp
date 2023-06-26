#include "hammer.h"

#include <engine/server/server.h>

#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/mmo/entities/pickup_job.h>
#include <game/server/mmo/entities/pickup_phys.h>
#include <game/server/player.h>
#include <game/server/mmo/mmo_core.h>
#include <game/server/gamecontext.h>
#include <game/server/mmo/dummies/dummy_base.h>

int CHammer::SnappingWeapon() { return WEAPON_HAMMER; }
const char *CHammer::WeaponName() { return "Hammer"; }
float CHammer::FireDelay() { return 20.f; }
bool CHammer::WillFire() { return true; }
bool CHammer::AutoFire()
{
	CPlayer *pPly = m_pOwner->GetPlayer();
	return pPly->m_AccInv.HaveItem(ITEM_AUTO_HAMMER);
}
int CHammer::WeaponID() { return MMO_WEAPON_HAMMER; }

void CHammer::OnPrimaryAttack()
{
	// Check farming
	CPickupJob *pPickup = (CPickupJob *)GameWorld()->ClosestEntity(m_pOwner->m_Pos, 30.f, CGameWorld::ENTTYPE_PICKUP_JOB);
	if (pPickup && pPickup->m_State != 0)
	{
		pPickup->Damage(m_pOwner->GetPlayer()->GetCID());
		m_pOwner->m_ReloadTimer = Server()->TickSpeed();
		return;
	}

	// Check pickup items
	CPickupPhys *pPickupPhys = (CPickupPhys *)GameWorld()->ClosestEntity(m_pOwner->m_Pos, 30.f, CGameWorld::ENTTYPE_PICKUP_PHYS);
	if (pPickupPhys && pPickupPhys->m_Type == PICKUP_PHYS_TYPE_ITEM)
	{
		MMOCore()->GiveItem(m_pOwner->GetPlayer()->GetCID(), pPickupPhys->m_ItemID, pPickupPhys->m_Count);
		pPickupPhys->Destroy();

		m_pOwner->m_ReloadTimer = Server()->TickSpeed();
		return;
	}

	// reset objects Hit
	GameServer()->CreateSound(m_pOwner->m_Pos, SOUND_HAMMER_FIRE, m_pOwner->TeamMask());

	if(m_pOwner->Core()->m_HammerHitDisabled)
		return;

	vec2 ProjStartPos = m_pOwner->m_Pos + m_pOwner->GetDirection() * m_pOwner->GetProximityRadius() * 0.75f;

	CEntity *apEnts[MAX_CLIENTS];
	int Hits = 0;
	int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_pOwner->GetProximityRadius() * 0.5f, apEnts,
		MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

	for(int i = 0; i < Num; ++i)
	{
		auto *pTarget = static_cast<CCharacter *>(apEnts[i]);

		if(pTarget == m_pOwner || (pTarget->IsAlive() && !m_pOwner->CanCollide(pTarget->GetPlayer()->GetCID())))
			continue;

		// set his velocity to fast upward (for now)
		if(length(pTarget->m_Pos - ProjStartPos) > 0.0f)
			GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - ProjStartPos) * m_pOwner->GetProximityRadius() * 0.5f, m_pOwner->TeamMask());
		else
			GameServer()->CreateHammerHit(ProjStartPos, m_pOwner->TeamMask());

		vec2 Dir;
		if(length(pTarget->m_Pos - m_pOwner->m_Pos) > 0.0f)
			Dir = normalize(pTarget->m_Pos - m_pOwner->m_Pos);
		else
			Dir = vec2(0.f, -1.f);

		float Strength;
		if(!m_pOwner->m_TuneZone)
			Strength = GameServer()->Tuning()->m_HammerStrength;
		else
			Strength = GameServer()->TuningList()[m_pOwner->m_TuneZone].m_HammerStrength;

		vec2 Temp = pTarget->Core()->m_Vel + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;
		Temp = ClampVel(pTarget->m_MoveRestrictions, Temp);
		Temp -= pTarget->Core()->m_Vel;
		pTarget->TakeDamage((vec2(0.f, -1.0f) + Temp) * Strength, 3,
			m_pOwner->GetPlayer()->GetCID(), SnappingWeapon());

		if(m_pOwner->m_FreezeHammer)
			pTarget->Freeze();

		Hits++;
	}

	Num = GameServer()->m_World.FindEntities(ProjStartPos, m_pOwner->GetProximityRadius(), apEnts,
		MAX_CLIENTS, CGameWorld::ENTTYPE_DUMMY);

	for(int i = 0; i < Num; ++i)
	{
		auto *pTarget = static_cast<CDummyBase *>(apEnts[i]);

		if(!pTarget->IsAlive())
			continue;

		// set his velocity to fast upward (for now)
		if(length(pTarget->m_Pos - ProjStartPos) > 0.0f)
			GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - ProjStartPos) * m_pOwner->GetProximityRadius() * 0.5f, m_pOwner->TeamMask());
		else
			GameServer()->CreateHammerHit(ProjStartPos, m_pOwner->TeamMask());

		vec2 Dir;
		if(length(pTarget->m_Pos - m_pOwner->m_Pos) > 0.0f)
			Dir = normalize(pTarget->m_Pos - m_pOwner->m_Pos);
		else
			Dir = vec2(0.f, -1.f);

		float Strength;
		if(!m_pOwner->m_TuneZone)
			Strength = GameServer()->Tuning()->m_HammerStrength;
		else
			Strength = GameServer()->TuningList()[m_pOwner->m_TuneZone].m_HammerStrength;

		vec2 Temp = pTarget->Core()->m_Vel + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;
		//Temp = ClampVel(pTarget->m_MoveRestrictions, Temp);
		Temp -= pTarget->Core()->m_Vel;
		pTarget->TakeDamage((vec2(0.f, -1.0f) + Temp) * Strength, 3,
			m_pOwner->GetPlayer()->GetCID(), SnappingWeapon());

		Hits++;
	}

	// if we Hit anything, we have to wait for to reload
	if(Hits)
	{
		float FireDelay;
		if(!m_pOwner->m_TuneZone)
			FireDelay = GameServer()->Tuning()->m_HammerHitFireDelay;
		else
			FireDelay = GameServer()->TuningList()[m_pOwner->m_TuneZone].m_HammerHitFireDelay;
		m_pOwner->m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
	}
}
