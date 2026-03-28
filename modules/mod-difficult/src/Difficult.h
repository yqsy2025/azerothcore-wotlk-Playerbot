#ifndef DEF_REALDIFF_H
#define DEF_REALDIFF_H

#include "Player.h"
#include "Config.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

struct DifficultData
{
	bool HaveDiff;
	bool NormalMode;
	bool HeroMode;

	float HealingNerfPct;

	float OtherMeleePct;
	float BossMeleePct;
	float OtherSpellPct;
	float BossSpellPct;
	bool OtherDamage;
	bool BossDamage;

	float OtherHPPct;
	float BossHPPct;
	bool OtherHP;
	bool BossHP;

	float TakenMeleePct;
	float TakenSpellPct;
};

struct SpellDiffData
{
	bool HaveDiff;
	float OtherSpellPct;
};

class Difficult
{
public:
	static Difficult* instance();

	void LoadDifficultSettings();

	bool IsEnabled{ false };
	bool IsDebugEnabled{ false };
	bool IsHPEnabled{ false };
	bool IsHPRealMode{ true };
	bool IsHealingEnabled{ false };
	bool IsDamageEnabled{ false };

	bool HPRealDisable{ true };
	bool HPSpawnDisable{ true };
	bool HealingDisable{ true };
	bool DamageDisable{ true };

	std::map<uint32, DifficultData> GuaiDiff;
	std::map<uint32, DifficultData> AreaDiff;
	std::map<uint32, DifficultData> ZoneDiff;
	std::map<uint32, DifficultData> MapDiff;
	std::map<uint32, SpellDiffData> SpellDiff;
};

#define sDifficult Difficult::instance()

#endif
