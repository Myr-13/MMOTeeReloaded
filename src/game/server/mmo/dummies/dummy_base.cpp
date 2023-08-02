#include "dummy_base.h"

#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <game/server/entities/laser.h>
#include <game/server/entities/projectile.h>

#include <game/server/mmo/entities/pickup_phys.h>

#include <game/mapitems.h>

// Mobs
#include "mobs/slime.h"

// Pets
#include "pets/pet.h"

CDummyBase::CDummyBase(CGameWorld *pWorld, vec2 Pos, int DummyType, int DummyAIType) :
	CEntity(pWorld, CGameWorld::ENTTYPE_DUMMY, Pos, 28.f)
{
	GameWorld()->InsertEntity(this);
	GameWorld()->m_Core.m_vDummies.push_back(&m_Core);
	m_Core.Init(&GameWorld()->m_Core, Collision());

	m_SpawnPos = Pos;
	m_NoDamage = false;
	m_DummyType = DummyType;
	m_DefaultEmote = EMOTE_NORMAL;
	m_DummyAIType = DummyAIType;
	m_Level = 0;
	m_MaxHealth = 10;
	m_MaxArmor = 10;
	m_Damage = 0;
	m_AttackTick = 0;

	str_copy(m_aName, "[NULL BOT]");
	str_copy(m_aClan, "");
	m_aFormatedName[0] = '\0';

	// Create dummy controller
	switch(m_DummyAIType)
	{
	case DUMMY_AI_TYPE_NONE: m_pDummyController = 0x0; break;
	case DUMMY_AI_TYPE_ATTACK: m_pDummyController = new CSlimeController(); break;
	case DUMMY_AI_TYPE_PET: m_pDummyController = new CPet(); break;
	default:
		m_pDummyController = 0x0;
		dbg_msg("dummy", "invalid dummy ai type: %d", m_DummyType);
	}

	if (m_pDummyController)
	{
		m_pDummyController->m_pDummyBase = this;
		m_pDummyController->Init();
	}

	Spawn();
}

CDummyBase::~CDummyBase()
{
	switch(m_DummyType)
	{
	case DUMMY_TYPE_SLIME: delete (CSlimeController *)m_pDummyController; break;
	case DUMMY_TYPE_PET: delete (CPet *)m_pDummyController; break;
	}
}

void CDummyBase::Spawn()
{
	if (m_Alive)
		Die(-1);

	m_Health = m_MaxHealth;
	m_Armor = m_MaxArmor;
	m_Alive = true;
	m_SpawnTick = 0;
	m_EmoteType = EMOTE_NORMAL;
	m_EmoteStop = 0;

	m_Core.m_Pos = m_SpawnPos;
	m_Pos = m_SpawnPos;
	m_Core.m_Vel = vec2(0, 0);

	GameServer()->CreatePlayerSpawn(m_Pos);
}

void CDummyBase::Die(int Killer)
{
	m_Alive = false;
	m_SpawnTick = Server()->Tick() + Server()->TickSpeed();

	if (Killer >= 0 && rand() % 2 == 0)
	{
		int Level = 1;

		CPlayer *pPly = GameServer()->m_apPlayers[Killer];
		if (pPly && pPly->m_LoggedIn)
			Level = pPly->m_AccData.m_Level;

		const int Count = (Level / 1.1f) / sqrt(Level) * 1.4f * 10;

		int RandomForce = 3 - rand() % 7;
		new CPickupPhys(
			GameWorld(),
			m_Pos,
			m_Core.m_Vel + vec2(RandomForce, RandomForce),
			(rand() % 2 == 0) ? PICKUP_PHYS_TYPE_XP : PICKUP_PHYS_TYPE_MONEY,
			Count);
	}

	GameServer()->CreateDeath(m_Pos, 0);
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);
}

void CDummyBase::TakeDamage(vec2 Force, int Damage, int From, int Weapon)
{
	if(!m_Alive)
		return;

	if(From >= 0)
		Damage += MMOCore()->GetPlusDamage(From);

	if(Damage)
	{
		// Emote
		m_EmoteType = EMOTE_PAIN;
		m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

		if(m_Armor)
		{
			if(Damage <= m_Armor)
			{
				m_Armor -= Damage;
				Damage = 0;
			}
			else
			{
				Damage -= m_Armor;
				m_Armor = 0;
			}
		}

		m_Health -= Damage;

		if(From >= 0 && GameServer()->m_apPlayers[From])
		{
			GameServer()->CreateSound(m_Pos, SOUND_HIT);

			//int Steal = (100 - Server()->GetItemCount(From, ACCESSORY_ADD_STEAL_HP) > 30) ? 100 - Server()->GetItemCount(From, ACCESSORY_ADD_STEAL_HP) > 30 : 30;
			//pFrom->m_Health += Steal;
		}
	}

	if (m_Health <= 0)
		Die(From);

	vec2 Temp = m_Core.m_Vel + Force;
	m_Core.m_Vel = Temp;
}

void CDummyBase::FireWeapon()
{
	vec2 Direction = normalize(vec2(m_Input.m_TargetX, m_Input.m_TargetY));

	bool FullAuto = false;
	if(m_Core.m_ActiveWeapon == WEAPON_GRENADE || m_Core.m_ActiveWeapon == WEAPON_SHOTGUN || m_Core.m_ActiveWeapon == WEAPON_LASER)
		FullAuto = true;

	bool WillFire = (m_Input.m_Fire & 1) && !(m_PrevInput.m_Fire & 1);
	if (!WillFire)
		return;

	vec2 ProjStartPos = m_Pos + Direction * 28.f * 0.75f;

	int Weapon = m_Core.m_ActiveWeapon;
	if (Weapon == WEAPON_GUN ||
		Weapon == WEAPON_SHOTGUN ||
		Weapon == WEAPON_GRENADE)
	{
		// Get lifetime
		int LifeTime;
		if (Weapon == WEAPON_GUN)
			LifeTime = Server()->TickSpeed() * GameServer()->Tuning()->m_GunLifetime;
		else if (Weapon == WEAPON_SHOTGUN)
			LifeTime = Server()->TickSpeed() * GameServer()->Tuning()->m_ShotgunLifetime;
		else
			LifeTime = Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeLifetime;

		const float Fov = 60.f;
		int Spray = 1 + ((Weapon == WEAPON_SHOTGUN) ? 5 : 0);

		// Create projectile
		new CProjectile(
			GameWorld(),
			Weapon, // Type
			-1, // Owner
			ProjStartPos, // Pos
			Direction, // Dir
			LifeTime, // Span
			false, // Freeze
			(Weapon == WEAPON_GRENADE), // Explosive
			-1 // SoundImpact
		);

		// Spread
		const float fovRad = (Fov / 2.f) * pi / 180.f;
		const float aps = fovRad / Spray * 2;
		const float a = angle(Direction);

		for (int i = 1; i <= Spray; i++) {
			float m = (i % 2 == 0) ? -1 : 1;
			float a1 = a + aps * (i / 2) * m;
			vec2 dir = direction(a1);

			new CProjectile(
				GameWorld(),
				Weapon, // Type
				-1, // Owner
				ProjStartPos, // Pos
				dir, // Dir
				LifeTime, // Span
				false, // Freeze
				(Weapon == WEAPON_GRENADE), // Explosive
				-1 // SoundImpact
			);
		}

		// Make a sound :O
		if (Weapon == WEAPON_GUN)
			GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
		else if (Weapon == WEAPON_SHOTGUN)
			GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
		else
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
	}

	switch(m_Core.m_ActiveWeapon)
	{
	case WEAPON_HAMMER:
	{
		// reset objects Hit
		GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);

		if(m_Core.m_HammerHitDisabled)
			break;

		CEntity *apEnts[MAX_CLIENTS];
		int Hits = 0;
		int Num = GameServer()->m_World.FindEntities(ProjStartPos, GetProximityRadius() * 0.5f, apEnts,
			MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

		for(int i = 0; i < Num; ++i)
		{
			auto *pTarget = static_cast<CCharacter *>(apEnts[i]);

			if(!pTarget->IsAlive())
				continue;

			// set his velocity to fast upward (for now)
			if(length(pTarget->m_Pos - ProjStartPos) > 0.0f)
				GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - ProjStartPos) * GetProximityRadius() * 0.5f);
			else
				GameServer()->CreateHammerHit(ProjStartPos);

			vec2 Dir;
			if(length(pTarget->m_Pos - m_Pos) > 0.0f)
				Dir = normalize(pTarget->m_Pos - m_Pos);
			else
				Dir = vec2(0.f, -1.f);

			float Strength = GameServer()->Tuning()->m_HammerStrength;

			vec2 Temp = pTarget->GetCore().m_Vel + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;
			Temp = ClampVel(pTarget->m_MoveRestrictions, Temp);
			Temp -= pTarget->GetCore().m_Vel;
			pTarget->TakeDamage((vec2(0.f, -1.0f) + Temp) * Strength, 3 + m_Damage,
				-1, m_Core.m_ActiveWeapon);

			Hits++;
		}

		Num = GameServer()->m_World.FindEntities(ProjStartPos, GetProximityRadius(), apEnts,
			MAX_CLIENTS, CGameWorld::ENTTYPE_DUMMY);

		for(int i = 0; i < Num; ++i)
		{
			auto *pTarget = static_cast<CDummyBase *>(apEnts[i]);

			if(!pTarget->IsAlive())
				continue;

			// set his velocity to fast upward (for now)
			if(length(pTarget->m_Pos - ProjStartPos) > 0.0f)
				GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - ProjStartPos) * GetProximityRadius() * 0.5f);
			else
				GameServer()->CreateHammerHit(ProjStartPos);

			vec2 Dir;
			if(length(pTarget->m_Pos - m_Pos) > 0.0f)
				Dir = normalize(pTarget->m_Pos - m_Pos);
			else
				Dir = vec2(0.f, -1.f);

			float Strength = GameServer()->Tuning()->m_HammerStrength;

			vec2 Temp = pTarget->Core()->m_Vel + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;
			//Temp = ClampVel(pTarget->m_MoveRestrictions, Temp);
			Temp -= pTarget->Core()->m_Vel;
			pTarget->TakeDamage((vec2(0.f, -1.0f) + Temp) * Strength, 3, -1, m_Core.m_ActiveWeapon);

			Hits++;
		}

		// if we Hit anything, we have to wait for to reload
		if(Hits)
		{
			float FireDelay = GameServer()->Tuning()->m_HammerHitFireDelay;
			m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
		}
	}
	break;

	case WEAPON_LASER:
	{
		float LaserReach = GameServer()->Tuning()->m_LaserReach;

		new CLaser(GameWorld(), m_Pos, Direction, LaserReach, -1, WEAPON_LASER);
		GameServer()->CreateSound(m_Pos, SOUND_LASER_FIRE);
	}
	break;
	}

	m_AttackTick = Server()->Tick();
}

void CDummyBase::HandleTiles(int Tile)
{
	if (Tile == TILE_OFF_DAMAGE && m_DummyType != DUMMY_TYPE_PET)
		Die(-1);
	else if (Tile == TILE_ON_DAMAGE && m_DummyType != DUMMY_TYPE_PET)
		Die(-1);

	else if (Tile == TILE_WATER)
		m_Core.m_Vel.y -= GameServer()->Tuning()->m_Gravity * 1.1f;
}

void CDummyBase::Destroy()
{
	for(int i = 0; i < GameWorld()->m_Core.m_vDummies.size(); i++)
		if(GameWorld()->m_Core.m_vDummies[i] == &m_Core)
			GameWorld()->m_Core.m_vDummies.erase(GameWorld()->m_Core.m_vDummies.begin() + i);

	delete this;
}

void CDummyBase::Tick()
{
	m_Core.m_Alive = m_Alive;

	if (Server()->Tick() > m_SpawnTick && !m_Alive)
		Spawn();

	// Don't calc phys if dummy is dead
	if (!m_Alive)
		return;

	if (m_pDummyController)
		m_pDummyController->DummyTick();

	m_Core.m_Input = m_Input;
	m_Core.m_ActiveWeapon = m_Input.m_WantedWeapon;
	m_Core.Tick(true);
	m_Core.Move();
	m_Pos = m_Core.m_Pos;

	FireWeapon();

	int Tile = Collision()->GetTile(m_Pos.x, m_Pos.y);
	HandleTiles(Tile);

	m_PrevInput = m_Input;
}

void CDummyBase::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	int SelfID = GameServer()->m_MMOCore.GetNextBotSnapID(SnappingClient);
	if (SelfID == -1)
	{
		dbg_msg("dummy", "cant get dummy snap ID for %d(%s). cheat?", SnappingClient, Server()->ClientName(SnappingClient));
		return;
	}

	// Snap player
	CNetObj_ClientInfo *pClientInfo = Server()->SnapNewItem<CNetObj_ClientInfo>(SelfID);
	if(!pClientInfo)
		return;

	if (m_aFormatedName[0] != '\0')
		StrToInts(&pClientInfo->m_Name0, 4, m_aFormatedName);
	else
		StrToInts(&pClientInfo->m_Name0, 4, m_aName);
	StrToInts(&pClientInfo->m_Clan0, 3, m_aClan);
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfo.m_aSkinName);
	pClientInfo->m_Country = 0;
	pClientInfo->m_UseCustomColor = m_TeeInfo.m_UseCustomColor;
	pClientInfo->m_ColorBody = m_TeeInfo.m_ColorBody;
	pClientInfo->m_ColorFeet = m_TeeInfo.m_ColorFeet;

	CNetObj_PlayerInfo *pPlayerInfo = Server()->SnapNewItem<CNetObj_PlayerInfo>(SelfID);
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = 0;
	pPlayerInfo->m_Score = 0;
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = SelfID;
	pPlayerInfo->m_Team = 10;

	// Don't snap character if dummy is dead
	if (!m_Alive)
		return;

	// Snap character
	CNetObj_Character *pCharacter = Server()->SnapNewItem<CNetObj_Character>(SelfID);
	if(!pCharacter)
		return;

	m_Core.Write(pCharacter);

	pCharacter->m_Tick = Server()->Tick();
	pCharacter->m_Emote = (m_EmoteStop > Server()->Tick()) ? m_EmoteType : m_DefaultEmote;
	pCharacter->m_HookedPlayer = -1;
	pCharacter->m_AttackTick = m_AttackTick;
	pCharacter->m_Direction = m_Input.m_Direction;
	pCharacter->m_Weapon = m_Input.m_WantedWeapon;
	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;
	pCharacter->m_PlayerFlags = PLAYERFLAG_PLAYING;

	// Snap ddnet character (just for pets kek)
	CNetObj_DDNetCharacter *pDDNetCharacter = Server()->SnapNewItem<CNetObj_DDNetCharacter>(SelfID);
	if(!pDDNetCharacter)
		return;

	pDDNetCharacter->m_Flags = 0;
	if(m_DummyType == DUMMY_TYPE_PET)
		pDDNetCharacter->m_Flags = CHARACTERFLAG_SOLO;

	pDDNetCharacter->m_FreezeEnd = 0;
	pDDNetCharacter->m_Jumps = m_Core.m_Jumps;
	pDDNetCharacter->m_TeleCheckpoint = -1;
	pDDNetCharacter->m_StrongWeakID = 0;

	// Display Information
	pDDNetCharacter->m_JumpedTotal = m_Core.m_JumpedTotal;
	pDDNetCharacter->m_NinjaActivationTick = m_Core.m_Ninja.m_ActivationTick;
	pDDNetCharacter->m_FreezeStart = m_Core.m_FreezeStart;
	pDDNetCharacter->m_TargetX = m_Core.m_Input.m_TargetX;
	pDDNetCharacter->m_TargetY = m_Core.m_Input.m_TargetY;
}
