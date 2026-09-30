#include "ItemUpgrade.h"

#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include <cmath>

void ItemUpgradePlayerScript::OnAfterApplyItemMods(Player* player, Item* item, uint8 slot, bool apply)
{
    //LOG_ERROR("entities.player", "=== OnAfterApplyItemMods FIRED item={} slot={} apply={} ===",
        //item->GetGUID().GetCounter(), slot, apply ? "ON" : "OFF");

    QueryResult result = CharacterDatabase.Query(
        "SELECT instance_guid, upgrade_lvl, owner_guid, increase_ratio FROM _item_upgrade WHERE instance_guid = '{}'",
        item->GetGUID().GetCounter());

    if (!result)
    {
        return;
    }

    Field* fields = result->Fetch();
    int32 incRatio = fields[3].Get<uint32>();
    LOG_ERROR("entities.player", "  -> Found record! incRatio={}", incRatio);

    if (incRatio <= 0)
    {
        return;
    }

    ItemTemplate itemInstance = GetUpgradeItemInstance(item, result);
    ItemTemplate const* proto = &itemInstance;

    ApplyUpgradeItemBonuses(player, proto, slot, apply, false);
    // 顺便刷新客户端 tooltip
    if(apply)
        SendItemPacket(player, item);
}

ItemTemplate ItemUpgradePlayerScript::GetUpgradeItemInstance(Item* item, QueryResult result)
{
    ItemTemplate const* itemTemplate = item->GetTemplate();
    ItemTemplate itemInstance = *itemTemplate;

    // 清零需要重算的字段（不要动 Delay，它默认继承原值）
    itemInstance.Armor = 0;

    for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        itemInstance.ItemStat[i].ItemStatValue = 0;

    for (uint8 i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
    {
        itemInstance.Damage[i].DamageMin = 0;
        itemInstance.Damage[i].DamageMax = 0;
    }

    Field* fields = result->Fetch();
    int32 incRatio = fields[3].Get<uint32>();

    if (incRatio > 0)
    {
        // incRatio = 10 表示 +10%
        // 若希望 DPS 净增 10%，用 sqrt；若接受 DPS 涨 m²，直接用 multiplier
        double multiplier = 1.0 + static_cast<double>(incRatio) / 100.0;

        // ---- 伤害（处理所有段）----
        for (uint8 i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
        {
            int32 minDamage = itemTemplate->Damage[i].DamageMin;
            int32 maxDamage = itemTemplate->Damage[i].DamageMax;

            if (minDamage > 0 && maxDamage > 0)
            {
                itemInstance.Damage[i].DamageMin = static_cast<int32>(std::ceil(minDamage * multiplier));
                itemInstance.Damage[i].DamageMax = static_cast<int32>(std::ceil(maxDamage * multiplier));
            }
        }

        // ---- 攻速（Delay 越小越快，所以除以 multiplier）----
        if (itemTemplate->Delay > 0)
        {
            uint32 newDelay = static_cast<uint32>(std::ceil(
                static_cast<double>(itemTemplate->Delay) / multiplier));
            if (newDelay < 200)
                newDelay = 200;
            itemInstance.Delay = newDelay;
        }

        // ---- 护甲 ----
        int32 armor = itemTemplate->Armor;
        if (armor > 0)
            itemInstance.Armor = static_cast<int32>(std::ceil(armor * multiplier));

        // ---- 属性 ----
        for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        {
            if (i >= itemTemplate->StatsCount)
                continue;

            int32 value = itemTemplate->ItemStat[i].ItemStatValue;
            if (value == 0)
                continue;

            itemInstance.ItemStat[i].ItemStatValue = static_cast<int32>(std::ceil(value * multiplier));
        }
    }

    return itemInstance;
}

void ItemUpgradePlayerScript::ApplyUpgradeItemBonuses(Player* player, ItemTemplate const* proto,
    uint8 slot, bool apply, bool onlyLevelScale)
{
    if (!player || slot >= INVENTORY_SLOT_BAG_END || !proto)
        return;

    ScalingStatDistributionEntry const* ssd = proto->ScalingStatDistribution
        ? sScalingStatDistributionStore.LookupEntry(proto->ScalingStatDistribution)
        : nullptr;

    if (onlyLevelScale && !ssd)
        return;

    uint32 ssdLevel = player->GetLevel();
    uint32 customScalingStatValue = 0;

    sScriptMgr->OnPlayerCustomScalingStatValueBefore(player, proto, slot, apply, customScalingStatValue);

    uint32 scalingStatValue = proto->ScalingStatValue > 0
        ? proto->ScalingStatValue
        : customScalingStatValue;

    if (ssd && ssdLevel > ssd->MaxLevel)
        ssdLevel = ssd->MaxLevel;

    ScalingStatValuesEntry const* ssv = proto->ScalingStatValue
        ? sScalingStatValuesStore.LookupEntry(ssdLevel)
        : nullptr;

    if (onlyLevelScale && !ssv)
        return;

    for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
    {
        uint32 statType = 0;
        int32 value = 0;

        if (ssv)
        {
            if (ssd)
            {
                if (ssd->StatMod[i] < 0)
                    continue;

                statType = ssd->StatMod[i];
                value = (ssv->getssdMultiplier(scalingStatValue) * ssd->Modifier[i]) / 10000;
            }
            else
            {
                if (i >= proto->StatsCount)
                    continue;

                sScriptMgr->OnPlayerCustomScalingStatValue(player, proto, statType, value, i,
                    scalingStatValue, ssv);
            }
        }
        else
        {
            if (i >= proto->StatsCount)
                continue;

            statType = proto->ItemStat[i].ItemStatType;
            value = proto->ItemStat[i].ItemStatValue;
        }

        if (value == 0)
            continue;

        switch (statType)
        {
            case ITEM_MOD_MANA:
                player->HandleStatFlatModifier(UNIT_MOD_MANA, BASE_VALUE, float(value), apply);
                break;
            case ITEM_MOD_HEALTH:
                player->HandleStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(value), apply);
                break;
            case ITEM_MOD_AGILITY:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, float(value), apply);
                player->UpdateStatBuffMod(STAT_AGILITY);
                break;
            case ITEM_MOD_STRENGTH:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, float(value), apply);
                player->UpdateStatBuffMod(STAT_STRENGTH);
                break;
            case ITEM_MOD_INTELLECT:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(value), apply);
                player->UpdateStatBuffMod(STAT_INTELLECT);
                break;
            case ITEM_MOD_SPIRIT:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_SPIRIT, BASE_VALUE, float(value), apply);
                player->UpdateStatBuffMod(STAT_SPIRIT);
                break;
            case ITEM_MOD_STAMINA:
                player->HandleStatFlatModifier(UNIT_MOD_STAT_STAMINA, BASE_VALUE, float(value), apply);
                player->UpdateStatBuffMod(STAT_STAMINA);
                break;
            case ITEM_MOD_DEFENSE_SKILL_RATING:
                player->ApplyRatingMod(CR_DEFENSE_SKILL, value, apply);
                break;
            case ITEM_MOD_DODGE_RATING:
                player->ApplyRatingMod(CR_DODGE, value, apply);
                break;
            case ITEM_MOD_PARRY_RATING:
                player->ApplyRatingMod(CR_PARRY, value, apply);
                break;
            case ITEM_MOD_BLOCK_RATING:
                player->ApplyRatingMod(CR_BLOCK, value, apply);
                break;
            case ITEM_MOD_HIT_MELEE_RATING:
                player->ApplyRatingMod(CR_HIT_MELEE, value, apply);
                break;
            case ITEM_MOD_HIT_RANGED_RATING:
                player->ApplyRatingMod(CR_HIT_RANGED, value, apply);
                break;
            case ITEM_MOD_HIT_SPELL_RATING:
                player->ApplyRatingMod(CR_HIT_SPELL, value, apply);
                break;
            case ITEM_MOD_CRIT_MELEE_RATING:
                player->ApplyRatingMod(CR_CRIT_MELEE, value, apply);
                break;
            case ITEM_MOD_CRIT_RANGED_RATING:
                player->ApplyRatingMod(CR_CRIT_RANGED, value, apply);
                break;
            case ITEM_MOD_CRIT_SPELL_RATING:
                player->ApplyRatingMod(CR_CRIT_SPELL, value, apply);
                break;
            case ITEM_MOD_HIT_TAKEN_MELEE_RATING:
                player->ApplyRatingMod(CR_HIT_TAKEN_MELEE, value, apply);
                break;
            case ITEM_MOD_HIT_TAKEN_RANGED_RATING:
                player->ApplyRatingMod(CR_HIT_TAKEN_RANGED, value, apply);
                break;
            case ITEM_MOD_HIT_TAKEN_SPELL_RATING:
                player->ApplyRatingMod(CR_HIT_TAKEN_SPELL, value, apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_MELEE_RATING:
                player->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, value, apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_RANGED_RATING:
                player->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, value, apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_SPELL_RATING:
                player->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, value, apply);
                break;
            case ITEM_MOD_HASTE_MELEE_RATING:
                player->ApplyRatingMod(CR_HASTE_MELEE, value, apply);
                break;
            case ITEM_MOD_HASTE_RANGED_RATING:
                player->ApplyRatingMod(CR_HASTE_RANGED, value, apply);
                break;
            case ITEM_MOD_HASTE_SPELL_RATING:
                player->ApplyRatingMod(CR_HASTE_SPELL, value, apply);
                break;
            case ITEM_MOD_HIT_RATING:
                player->ApplyRatingMod(CR_HIT_MELEE, value, apply);
                player->ApplyRatingMod(CR_HIT_RANGED, value, apply);
                player->ApplyRatingMod(CR_HIT_SPELL, value, apply);
                break;
            case ITEM_MOD_CRIT_RATING:
                player->ApplyRatingMod(CR_CRIT_MELEE, value, apply);
                player->ApplyRatingMod(CR_CRIT_RANGED, value, apply);
                player->ApplyRatingMod(CR_CRIT_SPELL, value, apply);
                break;
            case ITEM_MOD_HIT_TAKEN_RATING:
                player->ApplyRatingMod(CR_HIT_TAKEN_MELEE, value, apply);
                player->ApplyRatingMod(CR_HIT_TAKEN_RANGED, value, apply);
                player->ApplyRatingMod(CR_HIT_TAKEN_SPELL, value, apply);
                break;
            case ITEM_MOD_CRIT_TAKEN_RATING:
                player->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, value, apply);
                player->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, value, apply);
                player->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, value, apply);
                break;
            case ITEM_MOD_RESILIENCE_RATING:
                // WotLK/AzerothCore uses the three crit-taken rating channels for resilience.
                player->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, value, apply);
                player->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, value, apply);
                player->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, value, apply);
                break;
            case ITEM_MOD_HASTE_RATING:
                player->ApplyRatingMod(CR_HASTE_MELEE, value, apply);
                player->ApplyRatingMod(CR_HASTE_RANGED, value, apply);
                player->ApplyRatingMod(CR_HASTE_SPELL, value, apply);
                break;
            case ITEM_MOD_EXPERTISE_RATING:
                player->ApplyRatingMod(CR_EXPERTISE, value, apply);
                break;
            case ITEM_MOD_ATTACK_POWER:
                player->HandleStatFlatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(value), apply);
                player->HandleStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(value), apply);
                break;
            case ITEM_MOD_RANGED_ATTACK_POWER:
                player->HandleStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(value), apply);
                break;
            case ITEM_MOD_MANA_REGENERATION:
                player->ApplyManaRegenBonus(value, apply);
                break;
            case ITEM_MOD_ARMOR_PENETRATION_RATING:
                player->ApplyRatingMod(CR_ARMOR_PENETRATION, value, apply);
                break;
            case ITEM_MOD_SPELL_POWER:
                player->ApplySpellPowerBonus(value, apply);
                break;
            case ITEM_MOD_HEALTH_REGEN:
                player->ApplyHealthRegenBonus(value, apply);
                break;
            case ITEM_MOD_SPELL_PENETRATION:
                player->ApplySpellPenetrationBonus(value, apply);
                break;
            case ITEM_MOD_BLOCK_VALUE:
                player->HandleBaseModFlatValue(SHIELD_BLOCK_VALUE, float(value), apply);
                break;
            case ITEM_MOD_SPELL_HEALING_DONE:
            case ITEM_MOD_SPELL_DAMAGE_DONE:
                break;
        }
    }

    if (ssv)
    {
        if (int32 spellBonus = ssv->getSpellBonus(scalingStatValue))
            player->ApplySpellPowerBonus(spellBonus, apply);
    }

    uint32 armor = proto->Armor;
    if (armor)
    {
        UnitModifierFlatType modType = TOTAL_VALUE;
        if (proto->Class == ITEM_CLASS_ARMOR)
        {
            switch (proto->SubClass)
            {
                case ITEM_SUBCLASS_ARMOR_CLOTH:
                case ITEM_SUBCLASS_ARMOR_LEATHER:
                case ITEM_SUBCLASS_ARMOR_MAIL:
                case ITEM_SUBCLASS_ARMOR_PLATE:
                case ITEM_SUBCLASS_ARMOR_SHIELD:
                    modType = BASE_VALUE;
                    break;
                default:
                    break;
            }
        }
        player->HandleStatFlatModifier(UNIT_MOD_ARMOR, modType, float(armor), apply);
    }

    uint8 attackType = Player::GetAttackBySlot(slot);
    if (attackType != MAX_ATTACK)
    {
        float averageDamage = float((proto->Damage->DamageMax + proto->Damage->DamageMin) / 2);

        switch (attackType)
        {
            case BASE_ATTACK:
            default:
                player->HandleStatFlatModifier(UNIT_MOD_DAMAGE_MAINHAND, BASE_VALUE, averageDamage, apply);
                break;
            case OFF_ATTACK:
                player->HandleStatFlatModifier(UNIT_MOD_DAMAGE_OFFHAND, BASE_VALUE, averageDamage, apply);
                break;
            case RANGED_ATTACK:
                player->HandleStatFlatModifier(UNIT_MOD_DAMAGE_RANGED, BASE_VALUE, averageDamage, apply);
                break;
        }
    }

    if (player->getClass() == CLASS_DRUID)
    {
        int32 dpsMod = 0;
        int32 feralBonus = 0;

        if (ssv)
        {
            dpsMod = ssv->getDPSMod(scalingStatValue);
            feralBonus += ssv->getFeralBonus(scalingStatValue);
        }

        feralBonus += proto->getFeralBonus(dpsMod);
        sScriptMgr->OnPlayerGetFeralApBonus(player, feralBonus, dpsMod, proto, ssv);

        if (feralBonus)
            player->ApplyFeralAPBonus(feralBonus, apply);
    }
}

void ItemUpgradePlayerScript::SendItemPacket(Player* player, Item* item)
{
    if (!player || !item)
        return;

    // 查升级记录
    QueryResult result = CharacterDatabase.Query(
        "SELECT instance_guid, upgrade_lvl, owner_guid, increase_ratio FROM _item_upgrade WHERE instance_guid = '{}'",
        item->GetGUID().GetCounter());

    ItemTemplate const* pProto = nullptr;
    ItemTemplate upgradedInstance;

    if (result)
    {
        upgradedInstance = GetUpgradeItemInstance(item, result);
        pProto = &upgradedInstance;
    }
    else
    {
        pProto = sObjectMgr->GetItemTemplate(item->GetEntry());
    }

    if (!pProto)
        return;

    // ============================================================
    // 构造 SMSG_ITEM_QUERY_SINGLE_RESPONSE
    // 字段顺序完全照搬 ItemReforge::SendItemPacket
    // ============================================================
    std::string Name = pProto->Name1;
    std::string Description = pProto->Description;

    int loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    if (loc_idx >= 0)
    {
        if (ItemLocale const* il = sObjectMgr->GetItemLocale(pProto->ItemId))
        {
            ObjectMgr::GetLocaleString(il->Name, loc_idx, Name);
            ObjectMgr::GetLocaleString(il->Description, loc_idx, Description);
        }
    }

    WorldPacket queryData(SMSG_ITEM_QUERY_SINGLE_RESPONSE, 600);
    queryData << pProto->ItemId;
    queryData << pProto->Class;
    queryData << pProto->SubClass;
    queryData << pProto->SoundOverrideSubclass;
    queryData << Name;
    queryData << uint8(0x00);
    queryData << uint8(0x00);
    queryData << uint8(0x00);
    queryData << pProto->DisplayInfoID;
    queryData << pProto->Quality;
    queryData << pProto->Flags;
    queryData << pProto->Flags2;
    queryData << pProto->BuyPrice;
    queryData << pProto->SellPrice;
    queryData << pProto->InventoryType;
    queryData << pProto->AllowableClass;
    queryData << pProto->AllowableRace;
    //queryData << pProto->ItemLevel;
    // 原始 ItemLevel + 升级等级 × 10
    uint32 displayItemLevel = pProto->ItemLevel;

    if (result)   // result 是 _item_upgrade 表的查询结果
    {
        Field* fields = result->Fetch();
        uint32 upgradeLvl = fields[1].Get<uint32>();   // upgrade_lvl
        displayItemLevel += upgradeLvl * 10;
    }

    queryData << displayItemLevel;
    queryData << pProto->RequiredLevel;
    queryData << pProto->RequiredSkill;
    queryData << pProto->RequiredSkillRank;
    queryData << pProto->RequiredSpell;
    queryData << pProto->RequiredHonorRank;
    queryData << pProto->RequiredCityRank;
    queryData << pProto->RequiredReputationFaction;
    queryData << pProto->RequiredReputationRank;
    queryData << int32(pProto->MaxCount);
    queryData << int32(pProto->Stackable);
    queryData << pProto->ContainerSlots;

    // ============================================================
    // 关键：这里用放大后的 pProto->ItemStat
    // ============================================================
    queryData << pProto->StatsCount;
    for (uint32 i = 0; i < pProto->StatsCount; ++i)
    {
        queryData << pProto->ItemStat[i].ItemStatType;
        queryData << pProto->ItemStat[i].ItemStatValue;
    }

    queryData << pProto->ScalingStatDistribution;
    queryData << pProto->ScalingStatValue;

    for (int i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
    {
        queryData << pProto->Damage[i].DamageMin;
        queryData << pProto->Damage[i].DamageMax;
        queryData << pProto->Damage[i].DamageType;
    }

    queryData << pProto->Armor;
    queryData << pProto->HolyRes;
    queryData << pProto->FireRes;
    queryData << pProto->NatureRes;
    queryData << pProto->FrostRes;
    queryData << pProto->ShadowRes;
    queryData << pProto->ArcaneRes;

    queryData << pProto->Delay;
    queryData << pProto->AmmoType;
    queryData << pProto->RangedModRange;

    for (int s = 0; s < MAX_ITEM_PROTO_SPELLS; ++s)
    {
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(pProto->Spells[s].SpellId);
        if (spell)
        {
            bool db_data = pProto->Spells[s].SpellCooldown >= 0 || pProto->Spells[s].SpellCategoryCooldown >= 0;
            queryData << pProto->Spells[s].SpellId;
            queryData << pProto->Spells[s].SpellTrigger;
            queryData << int32(pProto->Spells[s].SpellCharges);

            if (db_data)
            {
                queryData << uint32(pProto->Spells[s].SpellCooldown);
                queryData << uint32(pProto->Spells[s].SpellCategory);
                queryData << uint32(pProto->Spells[s].SpellCategoryCooldown);
            }
            else
            {
                queryData << uint32(spell->RecoveryTime);
                queryData << uint32(spell->GetCategory());
                queryData << uint32(spell->CategoryRecoveryTime);
            }
        }
        else
        {
            queryData << uint32(0);
            queryData << uint32(0);
            queryData << uint32(0);
            queryData << uint32(-1);
            queryData << uint32(0);
            queryData << uint32(-1);
        }
    }

    queryData << pProto->Bonding;
    queryData << Description;
    queryData << pProto->PageText;
    queryData << pProto->LanguageID;
    queryData << pProto->PageMaterial;
    queryData << pProto->StartQuest;
    queryData << pProto->LockID;
    queryData << int32(pProto->Material);
    queryData << pProto->Sheath;
    queryData << pProto->RandomProperty;
    queryData << pProto->RandomSuffix;
    queryData << pProto->Block;
    queryData << pProto->ItemSet;
    queryData << pProto->MaxDurability;
    queryData << pProto->Area;
    queryData << pProto->Map;
    queryData << pProto->BagFamily;
    queryData << pProto->TotemCategory;

    for (int s = 0; s < MAX_ITEM_PROTO_SOCKETS; ++s)
    {
        queryData << pProto->Socket[s].Color;
        queryData << pProto->Socket[s].Content;
    }

    queryData << pProto->socketBonus;
    queryData << pProto->GemProperties;
    queryData << pProto->RequiredDisenchantSkill;
    queryData << pProto->ArmorDamageModifier;
    queryData << pProto->Duration;
    queryData << pProto->ItemLimitCategory;
    queryData << pProto->HolidayId;

    player->GetSession()->SendPacket(&queryData);
}

void ItemUpgradePlayerScript::OnPlayerLogin(Player* player)
{
    if (!player || (player->GetSession() && player->GetSession()->IsHeadless()))
        return;

    // 遍历所有装备
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
            SendItemPacket(player, item);
    }
}

void Addmod_item_upgradeScripts()
{
    new ItemUpgradePlayerScript();
}

