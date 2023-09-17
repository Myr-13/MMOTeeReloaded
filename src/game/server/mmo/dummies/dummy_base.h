#ifndef GAME_SERVER_ENTITIES_DUMMIES_DUMMY_BASE_H
#define GAME_SERVER_ENTITIES_DUMMIES_DUMMY_BASE_H

#include <game/server/entity.h>
#include <game/gamecore.h>
#include <game/server/teeinfo.h>
#include <game/server/mmo/mmo_core_structs.h>

enum
{
	DUMMY_TYPE_STAND,
	DUMMY_TYPE_SLIME,
	DUMMY_TYPE_LIVING_FIRE = 8,
	DUMMY_TYPE_PET
};

enum
{
	DUMMY_AI_TYPE_NONE,
	DUMMY_AI_TYPE_ATTACK,
	DUMMY_AI_TYPE_PET
};

class CDummyBase : public CEntity
{
	friend class CDummyController;

	CCharacterCore m_Core;
	CTeeInfo m_TeeInfo;

	vec2 m_SpawnPos;
	bool m_Alive;
	int m_SpawnTick;
	int m_ReloadTimer;
	int m_AttackTick;

	char m_aName[MAX_NAME_LENGTH];
	char m_aFormatedName[MAX_NAME_LENGTH];
	char m_aClan[MAX_CLAN_LENGTH];

public:
	CNetObj_PlayerInput m_Input;
	CNetObj_PlayerInput m_PrevInput;

	int m_Health;
	int m_Armor;
	bool m_NoDamage;
	std::vector<SBotLootData> m_vLoot;

private:
	int m_DummyType;
	int m_DummyAIType;
	class CDummyController *m_pDummyController;

	int m_DefaultEmote;
	int m_EmoteType;
	int m_EmoteStop;

public:
	CDummyBase(CGameWorld *pWorld, vec2 Pos, int DummyType, int DummyAIType);
	~CDummyBase();

	void Spawn();
	void Die(int Killer);
	void TakeDamage(vec2 Force, int Damage, int From, int Weapon);
	void FireWeapon();
	void HandleTiles(int Tile);

	virtual void Destroy() override;
	virtual void Tick() override;
	virtual void Snap(int SnappingClient) override;

	// Setters
	void SetName(const char *pName) { str_copy(m_aName, pName); }
	void SetClan(const char *pClan) { str_copy(m_aClan, pClan); }
	void SetTeeInfo(CTeeInfo Info) { m_TeeInfo = Info; }
	void FormatLevelName() { str_format(m_aFormatedName, sizeof(m_aFormatedName), "%d:%s", m_Level, m_aName); };

	// Getters
	bool IsAlive() { return m_Alive; }
	int GetDummyType() { return m_DummyType; }
	CCharacterCore *Core() { return &m_Core; }
	bool IsNoDamage() { return m_NoDamage; }
	class CDummyController *DummyController() { return m_pDummyController; }

	// Stats
	int m_Level;
	int m_MaxHealth;
	int m_MaxArmor;
	int m_Damage;
};

#endif // GAME_SERVER_ENTITIES_DUMMIES_DUMMY_BASE_H
