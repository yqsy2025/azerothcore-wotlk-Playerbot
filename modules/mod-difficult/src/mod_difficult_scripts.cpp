// 作者：牛皮德来

#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Unit.h"
#include "Difficult.h"

Difficult* Difficult::instance()
{
	static Difficult instance;
	return &instance;
}

void Difficult::LoadDifficultSettings()
{	//这部分是优化用的，一次性计算对应函数是否会启用相应的调整，用来降低函数无数次调用时候的逻辑判断次数，并用 Disable 参数代替 Enable 参数，消除额外的一个!运算
	if (sDifficult->IsEnabled && sDifficult->IsHPEnabled)
	{
		if (sDifficult->IsHPRealMode)
		{
			sDifficult->HPRealDisable = false;
			sDifficult->HPSpawnDisable = false;
		}
		else
		{
			sDifficult->HPRealDisable = true;
			sDifficult->HPSpawnDisable = false;
		}
	}
	else
	{
		sDifficult->HPRealDisable = true;
		sDifficult->HPSpawnDisable = true;
	}

	if (sDifficult->IsEnabled && sDifficult->IsHealingEnabled)
		sDifficult->HealingDisable = false;
	else
		sDifficult->HealingDisable = true;

	if (sDifficult->IsEnabled && sDifficult->IsDamageEnabled)
		sDifficult->DamageDisable = false;
	else
		sDifficult->DamageDisable = true;

	if (!sDifficult->IsEnabled)
	{
		return;		//这个为false的话,后面的override每个判定第一条都是直接return,所以不需要读取数据库了
	}

	if (QueryResult resultS = WorldDatabase.Query("SELECT * FROM 技能_难度调整"))
	{
		do
		{
			uint32 SpellID = (*resultS)[0].Get<uint32>();
			SpellDiffData data{};
			data.HaveDiff = 1;		//用来加快判断的
			data.OtherSpellPct = (*resultS)[1].Get<float>();
			sDifficult->SpellDiff[SpellID] = data;
		} while (resultS->NextRow());
	}

	if (QueryResult resultG = WorldDatabase.Query("SELECT * FROM 怪物_难度调整"))
	{
		do
		{
			uint32 GuaiID = (*resultG)[0].Get<uint32>();
			DifficultData data{};
			data.HaveDiff = 1;	//用来加快判断的,这样未命中的大部分怪可以快速结束匹配)
			data.NormalMode = ((*resultG)[1].Get<int>() & 1) == 1;
			data.HeroMode = ((*resultG)[1].Get<int>() & 2) == 2;
			data.OtherMeleePct = (*resultG)[2].Get<float>();
			data.OtherSpellPct = (*resultG)[3].Get<float>();
			data.OtherHPPct = (*resultG)[4].Get<float>();
			data.OtherHP = ((*resultG)[5].Get<int>() & 1) == 1;
			data.TakenMeleePct = (*resultG)[6].Get<float>();
			data.TakenSpellPct = (*resultG)[7].Get<float>();
			sDifficult->GuaiDiff[GuaiID] = data;
		} while (resultG->NextRow());
	}

	if (QueryResult resultA = WorldDatabase.Query("SELECT * FROM 地区（小）难度调整"))
	{
		do
		{
			uint32 AreaId = (*resultA)[0].Get<uint32>();
			DifficultData data{};
			data.HaveDiff = 1;	//用来加快判断的,这样未命中的大部分底图可以用3个&&判断结束过程,但是需要调整的怪从最大12个判断增加到最大15个判断(不用的地图对应sql条目删除可最优匹配)
			data.NormalMode = ((*resultA)[1].Get<int>() & 1) == 1;
			data.HeroMode = ((*resultA)[1].Get<int>() & 2) == 2;
			data.HealingNerfPct = (*resultA)[2].Get<float>();
			data.OtherMeleePct = (*resultA)[3].Get<float>();
			data.BossMeleePct = (*resultA)[4].Get<float>();
			data.OtherSpellPct = (*resultA)[5].Get<float>();
			data.BossSpellPct = (*resultA)[6].Get<float>();
			data.OtherDamage = ((*resultA)[7].Get<int>() & 1) == 1;
			data.BossDamage = ((*resultA)[7].Get<int>() & 2) == 2;
			data.OtherHPPct = (*resultA)[8].Get<float>();
			data.BossHPPct = (*resultA)[9].Get<float>();
			data.OtherHP = ((*resultA)[10].Get<int>() & 1) == 1;
			data.BossHP = ((*resultA)[10].Get<int>() & 2) == 2;
			sDifficult->AreaDiff[AreaId] = data;
		} while (resultA->NextRow());
	}

	if (QueryResult resultZ = WorldDatabase.Query("SELECT * FROM 地带（中）难度调整"))
	{
		do
		{
			uint32 ZoneId = (*resultZ)[0].Get<uint32>();
			DifficultData data{};
			data.HaveDiff = 1;
			data.NormalMode = ((*resultZ)[1].Get<int>() & 1) == 1;
			data.HeroMode = ((*resultZ)[1].Get<int>() & 2) == 2;
			data.HealingNerfPct = (*resultZ)[2].Get<float>();
			data.OtherMeleePct = (*resultZ)[3].Get<float>();
			data.BossMeleePct = (*resultZ)[4].Get<float>();
			data.OtherSpellPct = (*resultZ)[5].Get<float>();
			data.BossSpellPct = (*resultZ)[6].Get<float>();
			data.OtherDamage = ((*resultZ)[7].Get<int>() & 1) == 1;
			data.BossDamage = ((*resultZ)[7].Get<int>() & 2) == 2;
			data.OtherHPPct = (*resultZ)[8].Get<float>();
			data.BossHPPct = (*resultZ)[9].Get<float>();
			data.OtherHP = ((*resultZ)[10].Get<int>() & 1) == 1;
			data.BossHP = ((*resultZ)[10].Get<int>() & 2) == 2;
			sDifficult->ZoneDiff[ZoneId] = data;
		} while (resultZ->NextRow());
	}

	if (QueryResult resultM = WorldDatabase.Query("SELECT * FROM 地图（大）难度调整"))
	{
		do
		{
			uint32 MapId = (*resultM)[0].Get<uint32>();
			DifficultData data{};
			data.HaveDiff = 1;
			data.NormalMode = ((*resultM)[1].Get<int>() & 1) == 1;
			data.HeroMode = ((*resultM)[1].Get<int>() & 2) == 2;
			data.HealingNerfPct = (*resultM)[2].Get<float>();
			data.OtherMeleePct = (*resultM)[3].Get<float>();
			data.BossMeleePct = (*resultM)[4].Get<float>();
			data.OtherSpellPct = (*resultM)[5].Get<float>();
			data.BossSpellPct = (*resultM)[6].Get<float>();
			data.OtherDamage = ((*resultM)[7].Get<int>() & 1) == 1;
			data.BossDamage = ((*resultM)[7].Get<int>() & 2) == 2;
			data.OtherHPPct = (*resultM)[8].Get<float>();
			data.BossHPPct = (*resultM)[9].Get<float>();
			data.OtherHP = ((*resultM)[10].Get<int>() & 1) == 1;
			data.BossHP = ((*resultM)[10].Get<int>() & 2) == 2;
			sDifficult->MapDiff[MapId] = data;
		} while (resultM->NextRow());
	}
}

class Mod_Difficult_AllCreatureScript : public AllCreatureScript
{
public:
	Mod_Difficult_AllCreatureScript() : AllCreatureScript("Mod_Difficult_AllCreatureScript") {}

	void OnCreatureSelectLevel(const CreatureTemplate* creatureTemplate, Creature* creature) override
		//void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
	{
		if (sDifficult->HPSpawnDisable || !creature || !creature->IsAlive() || !creature->GetMap())
			return;

		if ((creature->IsHunterPet() || creature->IsPet() || creature->IsSummon()) && creature->IsControlledByPlayer())
			return;

		if (creature->IsCritter() || creature->IsTotem() || creature->IsTrigger())
			return;

		//如果启用了NPCbot机器人，则条件编译例外机器人血量调整
#if defined(MOD_PRESENT_NPCBOTS)
		if (creature->IsNPCBotOrPet())
			return;
#endif

		//CreatureTemplate const* creatureTemplate = creature->GetCreatureTemplate();
		CreatureBaseStats const* origCreatureStats = sObjectMgr->GetCreatureBaseStats(creature->GetLevel(), creatureTemplate->unit_class);
		uint32 baseHealth = origCreatureStats->GenerateHealth(creatureTemplate);
		uint32 newHp = baseHealth;
		bool RaidHero = creature->GetMap()->IsRaidOrHeroicDungeon();  //获取地图是否团队或者英雄本,下面判定是否 英雄本开启调整,且怪在英雄本 或者 普通本开启调整,且怪在普通本

		uint32 GuaiId = creature->GetEntry();
		if (sDifficult->GuaiDiff[GuaiId].HaveDiff && ((sDifficult->GuaiDiff[GuaiId].HeroMode && RaidHero) || (sDifficult->GuaiDiff[GuaiId].NormalMode && !RaidHero)))
		{
			if (sDifficult->GuaiDiff[GuaiId].OtherHP)		//单独改怪血量,只分副本难度,不分小怪和boss
				newHp = round(baseHealth * sDifficult->GuaiDiff[GuaiId].OtherHPPct);
		}
		else
		{
			uint32 AreaId = creature->GetAreaId();
			if (sDifficult->AreaDiff[AreaId].HaveDiff && ((sDifficult->AreaDiff[AreaId].HeroMode && RaidHero) || (sDifficult->AreaDiff[AreaId].NormalMode && !RaidHero)))
			{
				if (creature->isWorldBoss() || creature->IsDungeonBoss())
				{
					if (sDifficult->AreaDiff[AreaId].BossHP)		//是boss,并且要求boss改血量,则修改
						newHp = round(baseHealth * sDifficult->AreaDiff[AreaId].BossHPPct);
				}
				else
				{
					if (sDifficult->AreaDiff[AreaId].OtherHP)	//是小怪,并且要求小怪改血量,则修改
						newHp = round(baseHealth * sDifficult->AreaDiff[AreaId].OtherHPPct);
				}
			}
			else
			{
				uint32 ZoneId = creature->GetZoneId();
				if (sDifficult->ZoneDiff[ZoneId].HaveDiff && ((sDifficult->ZoneDiff[ZoneId].HeroMode && RaidHero) || (sDifficult->ZoneDiff[ZoneId].NormalMode && !RaidHero)))
				{
					if (creature->isWorldBoss() || creature->IsDungeonBoss())
					{
						if (sDifficult->ZoneDiff[ZoneId].BossHP)		//是boss,并且要求boss改血量,则修改
							newHp = round(baseHealth * sDifficult->ZoneDiff[ZoneId].BossHPPct);
					}
					else
					{
						if (sDifficult->ZoneDiff[ZoneId].OtherHP)	//是小怪,并且要求小怪改血量,则修改
							newHp = round(baseHealth * sDifficult->ZoneDiff[ZoneId].OtherHPPct);
					}
				}
				else
				{
					uint32 MapId = creature->GetMapId();
					if (sDifficult->MapDiff[MapId].HaveDiff && ((sDifficult->MapDiff[MapId].HeroMode && RaidHero) || (sDifficult->MapDiff[MapId].NormalMode && !RaidHero)))
					{
						if (creature->isWorldBoss() || creature->IsDungeonBoss())
						{
							if (sDifficult->MapDiff[MapId].BossHP)		//是boss,并且要求boss改血量,则修改
								newHp = round(baseHealth * sDifficult->MapDiff[MapId].BossHPPct);
						}
						else
						{
							if (sDifficult->MapDiff[MapId].OtherHP)	//是小怪,并且要求小怪改血量,则修改
								newHp = round(baseHealth * sDifficult->MapDiff[MapId].OtherHPPct);
						}
					}
				}
			}
		}

		if (creature->GetMaxHealth() == newHp)
			return;	//血量修改前后一致,直接退出

		float hpPct = creature->GetHealthPct();
		creature->SetMaxHealth(newHp);
		creature->SetCreateHealth(newHp);
		creature->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, (float)newHp);
		if (hpPct >= 100)	//修改后当前血量超过100%的,主要针对血量调低情况下,修改为当前血量为调整后的满血血量
		{
			creature->SetFullHealth();
			//creature->SetHealth(newHp);						//放弃这个,技能更改血量的boss可能不正确
			//creature->SetHealth(creature->GetMaxHealth());	//放弃这个,用服务器支持的满血设定
		}
		creature->UpdateAllStats();
		creature->ResetPlayerDamageReq();
	}

	void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
	{
		if (sDifficult->HPRealDisable || !creature || !creature->IsAlive() || !creature->GetMap())
			return;

		if ((creature->IsHunterPet() || creature->IsPet() || creature->IsSummon()) && creature->IsControlledByPlayer())
			return;

		if (creature->IsCritter() || creature->IsTotem() || creature->IsTrigger())
			return;

		//如果启用了NPCbot机器人，则条件编译例外机器人血量调整
#if defined(MOD_PRESENT_NPCBOTS)
		if (creature->IsNPCBotOrPet())
			return;
#endif

		CreatureTemplate const* creatureTemplate = creature->GetCreatureTemplate();
		CreatureBaseStats const* origCreatureStats = sObjectMgr->GetCreatureBaseStats(creature->GetLevel(), creatureTemplate->unit_class);
		uint32 baseHealth = origCreatureStats->GenerateHealth(creatureTemplate);
		uint32 newHp = baseHealth;
		bool RaidHero = creature->GetMap()->IsRaidOrHeroicDungeon();  //获取地图是否团队或者英雄本,下面判定是否 英雄本开启调整,且怪在英雄本 或者 普通本开启调整,且怪在普通本

		uint32 GuaiId = creature->GetEntry();
		if (sDifficult->GuaiDiff[GuaiId].HaveDiff && ((sDifficult->GuaiDiff[GuaiId].HeroMode && RaidHero) || (sDifficult->GuaiDiff[GuaiId].NormalMode && !RaidHero)))
		{
			if (sDifficult->GuaiDiff[GuaiId].OtherHP)		//单独改怪血量,只分副本难度,不分小怪和boss
				newHp = round(baseHealth * sDifficult->GuaiDiff[GuaiId].OtherHPPct);
		}
		else
		{
			uint32 AreaId = creature->GetAreaId();
			if (sDifficult->AreaDiff[AreaId].HaveDiff && ((sDifficult->AreaDiff[AreaId].HeroMode && RaidHero) || (sDifficult->AreaDiff[AreaId].NormalMode && !RaidHero)))
			{
				if (creature->isWorldBoss() || creature->IsDungeonBoss())
				{
					if (sDifficult->AreaDiff[AreaId].BossHP)		//是boss,并且要求boss改血量,则修改
						newHp = round(baseHealth * sDifficult->AreaDiff[AreaId].BossHPPct);
				}
				else
				{
					if (sDifficult->AreaDiff[AreaId].OtherHP)	//是小怪,并且要求小怪改血量,则修改
						newHp = round(baseHealth * sDifficult->AreaDiff[AreaId].OtherHPPct);
				}
			}
			else
			{
				uint32 ZoneId = creature->GetZoneId();
				if (sDifficult->ZoneDiff[ZoneId].HaveDiff && ((sDifficult->ZoneDiff[ZoneId].HeroMode && RaidHero) || (sDifficult->ZoneDiff[ZoneId].NormalMode && !RaidHero)))
				{
					if (creature->isWorldBoss() || creature->IsDungeonBoss())
					{
						if (sDifficult->ZoneDiff[ZoneId].BossHP)		//是boss,并且要求boss改血量,则修改
							newHp = round(baseHealth * sDifficult->ZoneDiff[ZoneId].BossHPPct);
					}
					else
					{
						if (sDifficult->ZoneDiff[ZoneId].OtherHP)	//是小怪,并且要求小怪改血量,则修改
							newHp = round(baseHealth * sDifficult->ZoneDiff[ZoneId].OtherHPPct);
					}
				}
				else
				{
					uint32 MapId = creature->GetMapId();
					if (sDifficult->MapDiff[MapId].HaveDiff && ((sDifficult->MapDiff[MapId].HeroMode && RaidHero) || (sDifficult->MapDiff[MapId].NormalMode && !RaidHero)))
					{
						if (creature->isWorldBoss() || creature->IsDungeonBoss())
						{
							if (sDifficult->MapDiff[MapId].BossHP)		//是boss,并且要求boss改血量,则修改
								newHp = round(baseHealth * sDifficult->MapDiff[MapId].BossHPPct);
						}
						else
						{
							if (sDifficult->MapDiff[MapId].OtherHP)	//是小怪,并且要求小怪改血量,则修改
								newHp = round(baseHealth * sDifficult->MapDiff[MapId].OtherHPPct);
						}
					}
				}
			}
		}

		if (creature->GetMaxHealth() == newHp)
			return;	//血量修改前后一致,直接退出

		float hpPct = creature->GetHealthPct();
		//creature->SetCreateHealth(newHp);
		//creature->SetMaxHealth(newHp);	//这个实时调血的一定不能加,加了后会导致战斗中用技能调整血量的boss回血不满或者复活后不满血
		creature->SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, (float)newHp);
		//creature->HandleStatModifier(UNIT_MOD_HEALTH, BASE_VALUE, (float)newHp, true);		//会不停的增加设定的血量
		//creature->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(hpPct), true);			//会不停的按百分比增加血量
		if (hpPct >= 100)	//修改后当前血量超过100%的,主要针对血量调低情况下,修改为当前血量为调整后的满血血量
		{
			creature->SetFullHealth();
			//creature->SetHealth(newHp);						//放弃这个,技能更改血量的boss可能不正确
			//creature->SetHealth(creature->GetMaxHealth());	//放弃这个,用服务器支持的满血设定
		}
		creature->UpdateAllStats();
		creature->ResetPlayerDamageReq();
	}
};

class Mod_Difficult_UnitScript : public UnitScript
{
public:
	//Mod_Difficult_UnitScript() : UnitScript("Mod_Difficult_UnitScript") { }
	Mod_Difficult_UnitScript() : UnitScript("Mod_Difficult_UnitScript", true, { UNITHOOK_MODIFY_HEAL_RECEIVED, UNITHOOK_ON_AURA_APPLY, UNITHOOK_MODIFY_MELEE_DAMAGE, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK }) {}

	void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* spellInfo) override
	{
		if (sDifficult->HealingDisable || !target || !target->GetMap() || !healer || !spellInfo)
			return;

#if defined(MOD_PRESENT_NPCBOTS)
		if (!(target->IsPlayer() || target->IsPet() || target->IsGuardian() || target->IsNPCBotOrPet()))
#else
		if (!(target->IsPlayer() || target->IsPet() || target->IsGuardian()))
#endif
			return;

		if (spellInfo->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES) || spellInfo->Mechanic == MECHANIC_BANDAGE) // 跳过不受影响的法术（药水）
			return;

		bool RaidHero = target->GetMap()->IsRaidOrHeroicDungeon();  //获取地图是否团队或者英雄本,下面判定是否 英雄本开启调整,且怪在英雄本 或者 普通本开启调整,且怪在普通本
		uint32 originHeal = heal;

		uint32 AreaId = target->GetAreaId();
		if (sDifficult->AreaDiff[AreaId].HaveDiff && ((sDifficult->AreaDiff[AreaId].HeroMode && RaidHero) || (sDifficult->AreaDiff[AreaId].NormalMode && !RaidHero)))
			heal = heal * sDifficult->AreaDiff[AreaId].HealingNerfPct;
		else
		{
			uint32 ZoneId = target->GetZoneId();
			if (sDifficult->ZoneDiff[ZoneId].HaveDiff && ((sDifficult->ZoneDiff[ZoneId].HeroMode && RaidHero) || (sDifficult->ZoneDiff[ZoneId].NormalMode && !RaidHero)))
				heal = heal * sDifficult->ZoneDiff[ZoneId].HealingNerfPct;
			else
			{
				uint32 MapId = target->GetMapId();
				if (sDifficult->MapDiff[MapId].HaveDiff && ((sDifficult->MapDiff[MapId].HeroMode && RaidHero) || (sDifficult->MapDiff[MapId].NormalMode && !RaidHero)))
					heal = heal * sDifficult->MapDiff[MapId].HealingNerfPct;
			}
		}

		if (sDifficult->IsDebugEnabled && heal != originHeal)	//前面有治疗者不为null的判定,所以这里不用加healer为空的检查
		{
			if (Player* player = healer->ToPlayer())	//只对玩家做debug,并且只提醒治疗者
			{
				if (player->GetSession())
					ChatHandler(player->GetSession()).PSendSysMessage("奶技能  {} {} 疗效从 {} 调整为 {}", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, originHeal, heal);
			}
		}
	}

	void OnAuraApply(Unit* target, Aura* aura) override
	{
		if (sDifficult->HealingDisable || !target || !target->GetMap() || !aura)
			return;

#if defined(MOD_PRESENT_NPCBOTS)
		if (!(target->IsPlayer() || target->IsPet() || target->IsGuardian() || target->IsNPCBotOrPet()))
#else
		if (!(target->IsPlayer() || target->IsPet() || target->IsGuardian()))
#endif
			return;

		bool RaidHero = target->GetMap()->IsRaidOrHeroicDungeon();  //获取地图是否团队或者英雄本,下面判定是否 英雄本开启调整,且怪在英雄本 或者 普通本开启调整,且怪在普通本
		float NerfPct = 1.0f;

		uint32 AreaId = target->GetAreaId();
		if (sDifficult->AreaDiff[AreaId].HaveDiff && ((sDifficult->AreaDiff[AreaId].HeroMode && RaidHero) || (sDifficult->AreaDiff[AreaId].NormalMode && !RaidHero)))
			NerfPct = sDifficult->AreaDiff[AreaId].HealingNerfPct;
		else
		{
			uint32 ZoneId = target->GetZoneId();
			if (sDifficult->ZoneDiff[ZoneId].HaveDiff && ((sDifficult->ZoneDiff[ZoneId].HeroMode && RaidHero) || (sDifficult->ZoneDiff[ZoneId].NormalMode && !RaidHero)))
				NerfPct = sDifficult->ZoneDiff[ZoneId].HealingNerfPct;
			else
			{
				uint32 MapId = target->GetMapId();
				if (sDifficult->MapDiff[MapId].HaveDiff && ((sDifficult->MapDiff[MapId].HeroMode && RaidHero) || (sDifficult->MapDiff[MapId].NormalMode && !RaidHero)))
					NerfPct = sDifficult->MapDiff[MapId].HealingNerfPct;
			}
		}

		if (NerfPct != 1.0f)
		{
			if (SpellInfo const* spellInfo = aura->GetSpellInfo())
			{
				if (spellInfo->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES))	// 跳过不受影响的法术（药水）
					return;

				if (spellInfo->HasAura(SPELL_AURA_SCHOOL_ABSORB))
				{
					Unit::AuraEffectList const& AuraEffectList = target->GetAuraEffectsByType(SPELL_AURA_SCHOOL_ABSORB);
					for (AuraEffect* eff : AuraEffectList)
					{
						if ((eff->GetAuraType() != SPELL_AURA_SCHOOL_ABSORB) || (eff->GetSpellInfo()->Id != spellInfo->Id))
							continue;

						int32 absorb = eff->GetAmount();
						eff->SetAmount(absorb * NerfPct);

						if (sDifficult->IsDebugEnabled && aura->GetCaster())	//前面有aura不为null的判定,所以只需要确认aura的施法者不为null就可以发送调试信息,不判定会宕机
						{
							if (Player* player = aura->GetCaster()->ToPlayer())	//只对玩家做debug,对玩家的宠物和守卫不检查
							{
								if (player->GetSession())
									ChatHandler(player->GetSession()).PSendSysMessage("技减伤  {} {} 吸收从 {} 调整为 {}", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, absorb, eff->GetAmount());
							}
						}
					}
				}
			}
		}
	}

	void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
	{
		if (sDifficult->DamageDisable || !attacker || !attacker->GetMap() || !target || !target->GetMap())
			return;

		bool RaidHero = target->GetMap()->IsRaidOrHeroicDungeon();  //获取地图是否团队或者英雄本,下面判定是否 英雄本开启调整,且怪在英雄本 或者 普通本开启调整,且怪在普通本
		uint32 originDamage = damage;

		if (target->ToCreature())
		{
			uint32 D_GuaiId = target->GetEntry();
			if (sDifficult->GuaiDiff[D_GuaiId].HaveDiff && ((sDifficult->GuaiDiff[D_GuaiId].HeroMode && RaidHero) || (sDifficult->GuaiDiff[D_GuaiId].NormalMode && !RaidHero)))
			{
				damage *= sDifficult->GuaiDiff[D_GuaiId].TakenMeleePct;	//单独承受物理伤害,只分副本难度,不分小怪和boss

				if (sDifficult->IsDebugEnabled && damage != originDamage)	//前面有目标不为null的判定,所以这里不用加target为空的检查
				{
					if (Player* player = attacker->ToPlayer())	//只对玩家造成的物理伤害做debug
					{
						if (player->GetSession())
						{
							ChatHandler(player->GetSession()).PSendSysMessage("怪承受肉搏  伤害从 {} 调整为 {}", originDamage, damage);
						}
					}
				}
			}
			return;    //如果目标是怪,直接返回,不需要返回damage,因为damage是引用,修改就生效
		}

#if defined(MOD_PRESENT_NPCBOTS)
		if (!((target->IsPlayer() || target->IsPet() || target->IsGuardian() || target->IsNPCBotOrPet()) && attacker->ToCreature()))
#else
		if (!((target->IsPlayer() || target->IsPet() || target->IsGuardian()) && attacker->ToCreature()))
#endif
			return;

		uint32 GuaiId = attacker->GetEntry();
		if (sDifficult->GuaiDiff[GuaiId].HaveDiff && ((sDifficult->GuaiDiff[GuaiId].HeroMode && RaidHero) || (sDifficult->GuaiDiff[GuaiId].NormalMode && !RaidHero)))
		{
			damage *= sDifficult->GuaiDiff[GuaiId].OtherMeleePct;	//单独改怪物理伤害,只分副本难度,不分小怪和boss
		}
		else
		{
			uint32 AreaId = attacker->GetAreaId();
			if (sDifficult->AreaDiff[AreaId].HaveDiff && ((sDifficult->AreaDiff[AreaId].HeroMode && RaidHero) || (sDifficult->AreaDiff[AreaId].NormalMode && !RaidHero)))
				if (attacker->ToCreature()->isWorldBoss() || attacker->ToCreature()->IsDungeonBoss())	//如果是boss
				{
					if (sDifficult->AreaDiff[AreaId].BossDamage)		//并且开启boss的调整
						damage *= sDifficult->AreaDiff[AreaId].BossMeleePct;		//修改boss肉搏伤害
					else
						return;	//boss不要求修改则退出
				}
				else	//是小怪
				{
					if (sDifficult->AreaDiff[AreaId].OtherDamage)	//并且开启小怪调整
						damage *= sDifficult->AreaDiff[AreaId].OtherMeleePct;	//修改小怪肉搏伤害
					else
						return;	//小怪不要求修改则退出
				}
			else
			{
				uint32 ZoneId = attacker->GetZoneId();
				if (sDifficult->ZoneDiff[ZoneId].HaveDiff && ((sDifficult->ZoneDiff[ZoneId].HeroMode && RaidHero) || (sDifficult->ZoneDiff[ZoneId].NormalMode && !RaidHero)))
					if (attacker->ToCreature()->isWorldBoss() || attacker->ToCreature()->IsDungeonBoss())	//如果是boss
					{
						if (sDifficult->ZoneDiff[ZoneId].BossDamage)		//并且开启boss的调整
							damage *= sDifficult->ZoneDiff[ZoneId].BossMeleePct;		//修改boss肉搏伤害
						else
							return;	//boss不要求修改则退出
					}
					else	//是小怪
					{
						if (sDifficult->ZoneDiff[ZoneId].OtherDamage)	//并且开启小怪调整
							damage *= sDifficult->ZoneDiff[ZoneId].OtherMeleePct;	//修改小怪肉搏伤害
						else
							return;	//小怪不要求修改则退出
					}
				else
				{
					uint32 MapId = attacker->GetMapId();
					if (sDifficult->MapDiff[MapId].HaveDiff && ((sDifficult->MapDiff[MapId].HeroMode && RaidHero) || (sDifficult->MapDiff[MapId].NormalMode && !RaidHero)))
						if (attacker->ToCreature()->isWorldBoss() || attacker->ToCreature()->IsDungeonBoss())	//如果是boss
						{
							if (sDifficult->MapDiff[MapId].BossDamage)		//并且开启boss的调整
								damage *= sDifficult->MapDiff[MapId].BossMeleePct;		//修改boss肉搏伤害
							else
								return;	//boss不要求修改则退出
						}
						else	//是小怪
						{
							if (sDifficult->MapDiff[MapId].OtherDamage)	//并且开启小怪调整
								damage *= sDifficult->MapDiff[MapId].OtherMeleePct;	//修改小怪肉搏伤害
							else
								return;	//小怪不要求修改则退出
						}
					else	//加入所有退出机制,避免其他怪也参与计算
						return;
				}
			}
		}

		if (sDifficult->IsDebugEnabled && damage != originDamage)	//前面有目标不为null的判定,所以这里不用加target为空的检查
		{
			if (Player* player = target->ToPlayer())	//只对玩家做debug,对玩家的宠物和守卫不检查
			{
				if (player->GetSession())
				{
					ChatHandler(player->GetSession()).PSendSysMessage("怪肉搏  伤害从 {} 调整为 {}", originDamage, damage);
				}
			}
		}
	}

	void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
	{
		if (sDifficult->DamageDisable || !spellInfo)
			return;

		//单独改列出的技能伤害,一旦修改完成,不再修改怪和副本难度对应的技能伤害
		if (sDifficult->SpellDiff[spellInfo->Id].HaveDiff)
		{
			damage *= sDifficult->SpellDiff[spellInfo->Id].OtherSpellPct;
			return;
		}

		if (!attacker || !attacker->GetMap() || !target || !target->GetMap())
			return;

		bool RaidHero = target->GetMap()->IsRaidOrHeroicDungeon();  //获取地图是否团队或者英雄本,下面判定是否 英雄本开启调整,且怪在英雄本 或者 普通本开启调整,且怪在普通本
		int32 originDamage = damage;

		if (target->ToCreature())
		{
			uint32 D_GuaiId = target->GetEntry();
			if (sDifficult->GuaiDiff[D_GuaiId].HaveDiff && ((sDifficult->GuaiDiff[D_GuaiId].HeroMode && RaidHero) || (sDifficult->GuaiDiff[D_GuaiId].NormalMode && !RaidHero)))
			{
				damage *= sDifficult->GuaiDiff[D_GuaiId].TakenSpellPct;	//单独承受法术伤害,只分副本难度,不分小怪和boss

				if (sDifficult->IsDebugEnabled && damage != originDamage)	//前面有目标不为null的判定,所以这里不用加target为空的检查
				{
					if (Player* player = attacker->ToPlayer())	//只对玩家造成的法术伤害做debug
					{
						if (player->GetSession())
						{
							ChatHandler(player->GetSession()).PSendSysMessage("怪承受技能  {} {} 伤害从 {} 调整为 {}", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, originDamage, damage);
						}
					}
				}
			}
			return;    //如果目标是怪,直接返回,不需要返回damage,因为damage是引用,修改就生效
		}

#if defined(MOD_PRESENT_NPCBOTS)
		if (!((target->IsPlayer() || target->IsPet() || target->IsGuardian() || target->IsNPCBotOrPet()) && attacker->ToCreature()))
#else
		if (!((target->IsPlayer() || target->IsPet() || target->IsGuardian()) && attacker->ToCreature()))
#endif
			return;

		uint32 GuaiId = attacker->GetEntry();
		if (sDifficult->GuaiDiff[GuaiId].HaveDiff && ((sDifficult->GuaiDiff[GuaiId].HeroMode && RaidHero) || (sDifficult->GuaiDiff[GuaiId].NormalMode && !RaidHero)))
		{
			damage *= sDifficult->GuaiDiff[GuaiId].OtherSpellPct;	//单独改怪技能伤害,只分副本难度,不分小怪和boss
		}
		else
		{
			uint32 AreaId = attacker->GetAreaId();
			if (sDifficult->AreaDiff[AreaId].HaveDiff && ((sDifficult->AreaDiff[AreaId].HeroMode && RaidHero) || (sDifficult->AreaDiff[AreaId].NormalMode && !RaidHero)))
				if (attacker->ToCreature()->isWorldBoss() || attacker->ToCreature()->IsDungeonBoss())	//如果是boss
				{
					if (sDifficult->AreaDiff[AreaId].BossDamage)		//并且开启boss的调整
						damage *= sDifficult->AreaDiff[AreaId].BossSpellPct;		//修改boss技能伤害
					else
						return;	//boss不要求修改则退出
				}
				else	//是小怪
				{
					if (sDifficult->AreaDiff[AreaId].OtherDamage)	//并且开启小怪调整
						damage *= sDifficult->AreaDiff[AreaId].OtherSpellPct;	//修改小怪技能伤害
					else
						return;	//小怪不要求修改则退出
				}
			else
			{
				uint32 ZoneId = attacker->GetZoneId();
				if (sDifficult->ZoneDiff[ZoneId].HaveDiff && ((sDifficult->ZoneDiff[ZoneId].HeroMode && RaidHero) || (sDifficult->ZoneDiff[ZoneId].NormalMode && !RaidHero)))
					if (attacker->ToCreature()->isWorldBoss() || attacker->ToCreature()->IsDungeonBoss())	//如果是boss
					{
						if (sDifficult->ZoneDiff[ZoneId].BossDamage)		//并且开启boss的调整
							damage *= sDifficult->ZoneDiff[ZoneId].BossSpellPct;		//修改boss技能伤害
						else
							return;	//boss不要求修改则退出
					}
					else	//是小怪
					{
						if (sDifficult->ZoneDiff[ZoneId].OtherDamage)	//并且开启小怪调整
							damage *= sDifficult->ZoneDiff[ZoneId].OtherSpellPct;	//修改小怪技能伤害
						else
							return;	//小怪不要求修改则退出
					}
				else
				{
					uint32 MapId = attacker->GetMapId();
					if (sDifficult->MapDiff[MapId].HaveDiff && ((sDifficult->MapDiff[MapId].HeroMode && RaidHero) || (sDifficult->MapDiff[MapId].NormalMode && !RaidHero)))
						if (attacker->ToCreature()->isWorldBoss() || attacker->ToCreature()->IsDungeonBoss())	//如果是boss
						{
							if (sDifficult->MapDiff[MapId].BossDamage)		//并且开启boss的调整
								damage *= sDifficult->MapDiff[MapId].BossSpellPct;		//修改boss技能伤害
							else
								return;	//boss不要求修改则退出
						}
						else	//是小怪
						{
							if (sDifficult->MapDiff[MapId].OtherDamage)	//并且开启小怪调整
								damage *= sDifficult->MapDiff[MapId].OtherSpellPct;	//修改小怪技能伤害
							else
								return;	//小怪不要求修改则退出
						}
					else	//加入所有退出机制,避免其他怪也参与计算
						return;
				}
			}
		}

		if (sDifficult->IsDebugEnabled && damage != originDamage)	//前面有目标不为null的判定,所以这里不用加target为空的检查
		{
			if (Player* player = target->ToPlayer())	//只对玩家做debug,对玩家的宠物和守卫不检查
			{
				if (player->GetSession())
				{
					ChatHandler(player->GetSession()).PSendSysMessage("怪技能  {} {} 伤害从 {} 调整为 {}", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, originDamage, damage);
				}
			}
		}
	}

	void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
	{
		if (sDifficult->DamageDisable || !spellInfo)
			return;

		//单独改列出的技能伤害,一旦修改完成,不再修改怪和副本难度对应的技能伤害
		if (sDifficult->SpellDiff[spellInfo->Id].HaveDiff)
		{
			damage *= sDifficult->SpellDiff[spellInfo->Id].OtherSpellPct;
			return;
		}

		if (!attacker || !attacker->GetMap() || !target || !target->GetMap())
			return;

		bool RaidHero = target->GetMap()->IsRaidOrHeroicDungeon();  //获取地图是否团队或者英雄本,下面判定是否 英雄本开启调整,且怪在英雄本 或者 普通本开启调整,且怪在普通本
		uint32 originDamage = damage;

		if (target->ToCreature())
		{
			uint32 D_GuaiId = target->GetEntry();
			if (sDifficult->GuaiDiff[D_GuaiId].HaveDiff && ((sDifficult->GuaiDiff[D_GuaiId].HeroMode && RaidHero) || (sDifficult->GuaiDiff[D_GuaiId].NormalMode && !RaidHero)))
			{
				damage *= sDifficult->GuaiDiff[D_GuaiId].TakenSpellPct;	//单独承受法术伤害,只分副本难度,不分小怪和boss

				if (sDifficult->IsDebugEnabled && damage != originDamage)	//前面有目标不为null的判定,所以这里不用加target为空的检查
				{
					if (Player* player = attacker->ToPlayer())	//只对玩家造成的法术伤害做debug
					{
						if (player->GetSession())
						{
							ChatHandler(player->GetSession()).PSendSysMessage("怪承受Dot  {} {} 伤害从 {} 调整为 {}", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, originDamage, damage);
						}
					}
				}
			}
			return;    //如果目标是怪,直接返回,不需要返回damage,因为damage是引用,修改就生效
		}

#if defined(MOD_PRESENT_NPCBOTS)
		if (!((target->IsPlayer() || target->IsPet() || target->IsGuardian() || target->IsNPCBotOrPet()) && attacker->ToCreature()))
#else
		if (!((target->IsPlayer() || target->IsPet() || target->IsGuardian()) && attacker->ToCreature()))
#endif
			return;

		uint32 GuaiId = attacker->GetEntry();
		if (sDifficult->GuaiDiff[GuaiId].HaveDiff && ((sDifficult->GuaiDiff[GuaiId].HeroMode && RaidHero) || (sDifficult->GuaiDiff[GuaiId].NormalMode && !RaidHero)))
		{
			damage *= sDifficult->GuaiDiff[GuaiId].OtherSpellPct;	//单独改怪伤害,只分副本难度,不分小怪和boss
		}
		else
		{
			uint32 AreaId = attacker->GetAreaId();
			if (sDifficult->AreaDiff[AreaId].HaveDiff && ((sDifficult->AreaDiff[AreaId].HeroMode && RaidHero) || (sDifficult->AreaDiff[AreaId].NormalMode && !RaidHero)))
				if (attacker->ToCreature()->isWorldBoss() || attacker->ToCreature()->IsDungeonBoss())	//如果是boss
				{
					if (sDifficult->AreaDiff[AreaId].BossDamage)		//并且开启boss的调整
						damage *= sDifficult->AreaDiff[AreaId].BossSpellPct;		//修改boss持续技能伤害
					else
						return;	//boss不要求修改则退出
				}
				else	//是小怪
				{
					if (sDifficult->AreaDiff[AreaId].OtherDamage)	//不是boss,并且开启小怪调整
						damage *= sDifficult->AreaDiff[AreaId].OtherSpellPct;	//修改小怪持续技能伤害
					else
						return;	//小怪不要求修改则退出
				}
			else
			{
				uint32 ZoneId = attacker->GetZoneId();
				if (sDifficult->ZoneDiff[ZoneId].HaveDiff && ((sDifficult->ZoneDiff[ZoneId].HeroMode && RaidHero) || (sDifficult->ZoneDiff[ZoneId].NormalMode && !RaidHero)))
					if (attacker->ToCreature()->isWorldBoss() || attacker->ToCreature()->IsDungeonBoss())	//如果是boss
					{
						if (sDifficult->ZoneDiff[ZoneId].BossDamage)		//并且开启boss的调整
							damage *= sDifficult->ZoneDiff[ZoneId].BossSpellPct;		//修改boss持续技能伤害
						else
							return;	//boss不要求修改则退出
					}
					else	//是小怪
					{
						if (sDifficult->ZoneDiff[ZoneId].OtherDamage)	//不是boss,并且开启小怪调整
							damage *= sDifficult->ZoneDiff[ZoneId].OtherSpellPct;	//修改小怪持续技能伤害
						else
							return;	//小怪不要求修改则退出
					}
				else
				{
					uint32 MapId = attacker->GetMapId();
					if (sDifficult->MapDiff[MapId].HaveDiff && ((sDifficult->MapDiff[MapId].HeroMode && RaidHero) || (sDifficult->MapDiff[MapId].NormalMode && !RaidHero)))
						if (attacker->ToCreature()->isWorldBoss() || attacker->ToCreature()->IsDungeonBoss())	//如果是boss
						{
							if (sDifficult->MapDiff[MapId].BossDamage)		//并且开启boss的调整
								damage *= sDifficult->MapDiff[MapId].BossSpellPct;		//修改boss持续技能伤害
							else
								return;	//boss不要求修改则退出
						}
						else	//是小怪
						{
							if (sDifficult->MapDiff[MapId].OtherDamage)	//不是boss,并且开启小怪调整
								damage *= sDifficult->MapDiff[MapId].OtherSpellPct;	//修改小怪持续技能伤害
							else
								return;	//小怪不要求修改则退出
						}
					else	//加入所有退出机制,避免其他怪也参与计算
						return;
				}
			}
		}

		if (sDifficult->IsDebugEnabled && damage != originDamage)	//前面有目标不为null的判定,所以这里不用加target为空的检查
		{
			if (Player* player = target->ToPlayer())	//只对玩家做debug,对玩家的宠物和守卫不检查
			{
				if (player->GetSession())
				{
					ChatHandler(player->GetSession()).PSendSysMessage("怪Dot   {} {} 伤害从 {} 调整为 {}", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, originDamage, damage);
				}
			}
		}
	}
};

class Mod_Difficult_WorldScript : public WorldScript
{
public:
	Mod_Difficult_WorldScript() : WorldScript("Mod_Difficult_WorldScript", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) {}

	void OnAfterConfigLoad(bool /*reload*/) override
	{
		sDifficult->IsEnabled = sConfigMgr->GetOption<bool>("ModDifficult.Enable", false);
		sDifficult->IsDebugEnabled = sConfigMgr->GetOption<bool>("ModDifficult.Debug", false);
		sDifficult->IsHealingEnabled = sConfigMgr->GetOption<bool>("ModDifficult.Healing", false);
		sDifficult->IsDamageEnabled = sConfigMgr->GetOption<bool>("ModDifficult.Damage", false);
		sDifficult->IsHPEnabled = sConfigMgr->GetOption<bool>("ModDifficult.HP", false);
		sDifficult->IsHPRealMode = sConfigMgr->GetOption<bool>("ModDifficult.HPRealMode", false);
		sDifficult->LoadDifficultSettings();
	}
};

// Add all scripts in one
void AddModDifficultScripts()
{
	new Mod_Difficult_WorldScript();
	new Mod_Difficult_AllCreatureScript();
	new Mod_Difficult_UnitScript();
}
