/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ChallengeModes.h"
#include "WorldSessionMgr.h"

ChallengeModes* ChallengeModes::instance()
{
    static ChallengeModes instance;
    return &instance;
}

bool ChallengeModes::challengeEnabledForPlayer(ChallengeModeSettings setting, Player* player) const
{
    if (!enabled() || !challengeEnabled(setting))
    {
        return false;
    }
    return player->GetPlayerSetting("mod-challenge-modes", setting).value;
}

bool ChallengeModes::challengeEnabled(ChallengeModeSettings setting) const
{
    switch (setting)
    {
        case SETTING_HARDCORE:
            return hardcoreEnable;
        case SETTING_SEMI_HARDCORE:
            return semiHardcoreEnable;
        case SETTING_SELF_CRAFTED:
            return selfCraftedEnable;
        case SETTING_ITEM_QUALITY_LEVEL:
            return itemQualityLevelEnable;
        case SETTING_SLOW_XP_GAIN:
            return slowXpGainEnable;
        case SETTING_VERY_SLOW_XP_GAIN:
            return verySlowXpGainEnable;
        case SETTING_QUEST_XP_ONLY:
            return questXpOnlyEnable;
        case SETTING_IRON_MAN:
            return ironManEnable;
        case HARDCORE_DEAD:
            break;
    }
    return false;
}

uint32 ChallengeModes::getDisableLevel(ChallengeModeSettings setting) const
{
    switch (setting)
    {
        case SETTING_HARDCORE:
            return hardcoreDisableLevel;
        case SETTING_SEMI_HARDCORE:
            return semiHardcoreDisableLevel;
        case SETTING_SELF_CRAFTED:
            return selfCraftedDisableLevel;
        case SETTING_ITEM_QUALITY_LEVEL:
            return itemQualityLevelDisableLevel;
        case SETTING_SLOW_XP_GAIN:
            return slowXpGainDisableLevel;
        case SETTING_VERY_SLOW_XP_GAIN:
            return verySlowXpGainDisableLevel;
        case SETTING_QUEST_XP_ONLY:
            return questXpOnlyDisableLevel;
        case SETTING_IRON_MAN:
            return ironManDisableLevel;
        case HARDCORE_DEAD:
            break;
    }
    return 0;
}

float ChallengeModes::getXpBonusForChallenge(ChallengeModeSettings setting) const
{
    switch (setting)
    {
        case SETTING_HARDCORE:
            return hardcoreXpBonus;
        case SETTING_SEMI_HARDCORE:
            return semiHardcoreXpBonus;
        case SETTING_SELF_CRAFTED:
            return selfCraftedXpBonus;
        case SETTING_ITEM_QUALITY_LEVEL:
            return itemQualityLevelXpBonus;
        case SETTING_SLOW_XP_GAIN:
            return slowXpGainBonus;
        case SETTING_VERY_SLOW_XP_GAIN:
            return verySlowXpGainBonus;
        case SETTING_QUEST_XP_ONLY:
            return questXpOnlyXpBonus;
        case SETTING_IRON_MAN:
            return ironManXpBonus;
        case HARDCORE_DEAD:
            break;
    }
    return 1;
}

const std::unordered_map<uint8, uint32> *ChallengeModes::getTitleMapForChallenge(ChallengeModeSettings setting) const
{
    switch (setting)
    {
        case SETTING_HARDCORE:
            return &hardcoreTitleRewards;
        case SETTING_SEMI_HARDCORE:
            return &semiHardcoreTitleRewards;
        case SETTING_SELF_CRAFTED:
            return &selfCraftedTitleRewards;
        case SETTING_ITEM_QUALITY_LEVEL:
            return &itemQualityLevelTitleRewards;
        case SETTING_SLOW_XP_GAIN:
            return &slowXpGainTitleRewards;
        case SETTING_VERY_SLOW_XP_GAIN:
            return &verySlowXpGainTitleRewards;
        case SETTING_QUEST_XP_ONLY:
            return &questXpOnlyTitleRewards;
        case SETTING_IRON_MAN:
            return &ironManTitleRewards;
        case HARDCORE_DEAD:
            break;
    }
    return {};
}

const std::unordered_map<uint8, uint32> *ChallengeModes::getTalentMapForChallenge(ChallengeModeSettings setting) const
{
    switch (setting)
    {
        case SETTING_HARDCORE:
            return &hardcoreTalentRewards;
        case SETTING_SEMI_HARDCORE:
            return &semiHardcoreTalentRewards;
        case SETTING_SELF_CRAFTED:
            return &selfCraftedTalentRewards;
        case SETTING_ITEM_QUALITY_LEVEL:
            return &itemQualityLevelTalentRewards;
        case SETTING_SLOW_XP_GAIN:
            return &slowXpGainTalentRewards;
        case SETTING_VERY_SLOW_XP_GAIN:
            return &verySlowXpGainTalentRewards;
        case SETTING_QUEST_XP_ONLY:
            return &questXpOnlyTalentRewards;
        case SETTING_IRON_MAN:
            return &ironManTalentRewards;
        case HARDCORE_DEAD:
            break;
    }
    return {};
}

const std::unordered_map<uint8, uint32> *ChallengeModes::getItemMapForChallenge(ChallengeModeSettings setting) const
{
    switch (setting)
    {
        case SETTING_HARDCORE:
            return &hardcoreItemRewards;
        case SETTING_SEMI_HARDCORE:
            return &semiHardcoreItemRewards;
        case SETTING_SELF_CRAFTED:
            return &selfCraftedItemRewards;
        case SETTING_ITEM_QUALITY_LEVEL:
            return &itemQualityLevelItemRewards;
        case SETTING_SLOW_XP_GAIN:
            return &slowXpGainItemRewards;
        case SETTING_VERY_SLOW_XP_GAIN:
            return &verySlowXpGainItemRewards;
        case SETTING_QUEST_XP_ONLY:
            return &questXpOnlyItemRewards;
        case SETTING_IRON_MAN:
            return &ironManItemRewards;
        case HARDCORE_DEAD:
            break;
    }
    return {};
}

uint32 ChallengeModes::getItemRewardAmount(ChallengeModeSettings setting) const
{
    switch (setting)
    {
        case SETTING_HARDCORE:
            return hardcoreItemRewardAmount;
        case SETTING_SEMI_HARDCORE:
            return semiHardcoreItemRewardAmount;
        case SETTING_SELF_CRAFTED:
            return selfCraftedItemRewardAmount;
        case SETTING_ITEM_QUALITY_LEVEL:
            return itemQualityLevelItemRewardAmount;
        case SETTING_SLOW_XP_GAIN:
            return slowXpGainItemRewardAmount;
        case SETTING_VERY_SLOW_XP_GAIN:
            return verySlowXpGainItemRewardAmount;
        case SETTING_QUEST_XP_ONLY:
            return questXpOnlyItemRewardAmount;
        case SETTING_IRON_MAN:
            return ironManItemRewardAmount;
        case HARDCORE_DEAD:
            break;
    }
    return 0;
}

const std::unordered_map<uint8, uint32> *ChallengeModes::getAchievementMapForChallenge(ChallengeModeSettings setting) const
{
    switch (setting)
    {
        case SETTING_HARDCORE:
            return &hardcoreAchievementReward;
        case SETTING_SEMI_HARDCORE:
            return &semiHardcoreAchievementReward;
        case SETTING_SELF_CRAFTED:
            return &selfCraftedAchievementReward;
        case SETTING_ITEM_QUALITY_LEVEL:
            return &itemQualityLevelAchievementReward;
        case SETTING_SLOW_XP_GAIN:
            return &slowXpGainAchievementReward;
        case SETTING_VERY_SLOW_XP_GAIN:
            return &verySlowXpGainAchievementReward;
        case SETTING_QUEST_XP_ONLY:
            return &questXpOnlyAchievementReward;
        case SETTING_IRON_MAN:
            return &ironManAchievementReward;
        case HARDCORE_DEAD:
            break;
    }
    return {};
}

class ChallengeModes_WorldScript : public WorldScript
{
public:
    ChallengeModes_WorldScript()
        : WorldScript("ChallengeModes_WorldScript")
    {}

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        LoadConfig();
    }

private:
    static void LoadStringToMap(std::unordered_map<uint8, uint32> &mapToLoad, const std::string &configString)
    {
        std::string delimitedValue;
        std::stringstream configIdStream;

        configIdStream.str(configString);
        // Process each config ID in the string, delimited by the comma - "," and then space " "
        while (std::getline(configIdStream, delimitedValue, ','))
        {
            std::string pairOne, pairTwo;
            std::stringstream configPairStream(delimitedValue);
            configPairStream>>pairOne>>pairTwo;
            auto configLevel = atoi(pairOne.c_str());
            auto rewardValue = atoi(pairTwo.c_str());
            mapToLoad[configLevel] = rewardValue;
        }
    }

    static void LoadConfig()
    {
        sChallengeModes->challengesEnabled = sConfigMgr->GetOption<bool>("ChallengeModes.Enable", false);
        if (sChallengeModes->enabled())
        {
            for (auto& [confName, rewardMap] : sChallengeModes->rewardConfigMap)
            {
                rewardMap->clear();
                LoadStringToMap(*rewardMap, sConfigMgr->GetOption<std::string>(confName, ""));
            }

            sChallengeModes->hardcoreEnable          = sConfigMgr->GetOption<bool>("Hardcore.Enable", true);
            sChallengeModes->semiHardcoreEnable      = sConfigMgr->GetOption<bool>("SemiHardcore.Enable", true);
            sChallengeModes->selfCraftedEnable       = sConfigMgr->GetOption<bool>("SelfCrafted.Enable", true);
            sChallengeModes->itemQualityLevelEnable  = sConfigMgr->GetOption<bool>("ItemQualityLevel.Enable", true);
            sChallengeModes->slowXpGainEnable        = sConfigMgr->GetOption<bool>("SlowXpGain.Enable", true);
            sChallengeModes->verySlowXpGainEnable    = sConfigMgr->GetOption<bool>("VerySlowXpGain.Enable", true);
            sChallengeModes->questXpOnlyEnable       = sConfigMgr->GetOption<bool>("QuestXpOnly.Enable", true);
            sChallengeModes->ironManEnable           = sConfigMgr->GetOption<bool>("IronMan.Enable", true);

            sChallengeModes->hardcoreDisableLevel          = sConfigMgr->GetOption<uint32>("Hardcore.DisableLevel", 0);
            sChallengeModes->semiHardcoreDisableLevel      = sConfigMgr->GetOption<uint32>("SemiHardcore.DisableLevel", 0);
            sChallengeModes->selfCraftedDisableLevel       = sConfigMgr->GetOption<uint32>("SelfCrafted.DisableLevel", 0);
            sChallengeModes->itemQualityLevelDisableLevel  = sConfigMgr->GetOption<uint32>("ItemQualityLevel.DisableLevel", 0);
            sChallengeModes->slowXpGainDisableLevel        = sConfigMgr->GetOption<uint32>("SlowXpGain.DisableLevel", 0);
            sChallengeModes->verySlowXpGainDisableLevel    = sConfigMgr->GetOption<uint32>("VerySlowXpGain.DisableLevel", 0);
            sChallengeModes->questXpOnlyDisableLevel       = sConfigMgr->GetOption<uint32>("QuestXpOnly.DisableLevel", 0);
            sChallengeModes->ironManDisableLevel           = sConfigMgr->GetOption<uint32>("IronMan.DisableLevel", 0);

            sChallengeModes->hardcoreXpBonus         = sConfigMgr->GetOption<float>("Hardcore.XPMultiplier", 1.0f);
            sChallengeModes->semiHardcoreXpBonus     = sConfigMgr->GetOption<float>("SemiHardcore.XPMultiplier", 1.0f);
            sChallengeModes->selfCraftedXpBonus      = sConfigMgr->GetOption<float>("SelfCrafted.XPMultiplier", 1.0f);
            sChallengeModes->itemQualityLevelXpBonus = sConfigMgr->GetOption<float>("ItemQualityLevel.XPMultiplier", 1.0f);
            sChallengeModes->questXpOnlyXpBonus      = sConfigMgr->GetOption<float>("QuestXpOnly.XPMultiplier", 1.0f);
            sChallengeModes->slowXpGainBonus         = sConfigMgr->GetOption<float>("SlowXpGain.XPMultiplier", 0.50f);
            sChallengeModes->verySlowXpGainBonus     = sConfigMgr->GetOption<float>("VerySlowXpGain.XPMultiplier", 0.25f);
            sChallengeModes->ironManXpBonus          = sConfigMgr->GetOption<float>("IronMan.XPMultiplier", 1.0f);

            sChallengeModes->hardcoreItemRewardAmount         = sConfigMgr->GetOption<uint32>("Hardcore.ItemRewardAmount", 1);
            sChallengeModes->semiHardcoreItemRewardAmount     = sConfigMgr->GetOption<uint32>("SemiHardcore.ItemRewardAmount", 1);
            sChallengeModes->selfCraftedItemRewardAmount      = sConfigMgr->GetOption<uint32>("SelfCrafted.ItemRewardAmount", 1);
            sChallengeModes->itemQualityLevelItemRewardAmount = sConfigMgr->GetOption<uint32>("ItemQualityLevel.ItemRewardAmount", 1);
            sChallengeModes->slowXpGainItemRewardAmount       = sConfigMgr->GetOption<uint32>("SlowXpGain.ItemRewardAmount", 1);
            sChallengeModes->verySlowXpGainItemRewardAmount   = sConfigMgr->GetOption<uint32>("VerySlowXpGain.ItemRewardAmount", 1);
            sChallengeModes->questXpOnlyItemRewardAmount      = sConfigMgr->GetOption<uint32>("QuestXpOnly.ItemRewardAmount", 1);
            sChallengeModes->ironManItemRewardAmount          = sConfigMgr->GetOption<uint32>("IronMan.ItemRewardAmount", 1);

            LoadStringToMap(sChallengeModes->hardcoreAchievementReward, sConfigMgr->GetOption<std::string>("Hardcore.AchievementReward", ""));
            LoadStringToMap(sChallengeModes->semiHardcoreAchievementReward, sConfigMgr->GetOption<std::string>("SemiHardcore.AchievementReward", ""));
            LoadStringToMap(sChallengeModes->selfCraftedAchievementReward, sConfigMgr->GetOption<std::string>("SelfCrafted.AchievementReward", ""));
            LoadStringToMap(sChallengeModes->itemQualityLevelAchievementReward, sConfigMgr->GetOption<std::string>("ItemQualityLevel.AchievementReward", ""));
            LoadStringToMap(sChallengeModes->slowXpGainAchievementReward, sConfigMgr->GetOption<std::string>("SlowXpGain.AchievementReward", ""));
            LoadStringToMap(sChallengeModes->verySlowXpGainAchievementReward, sConfigMgr->GetOption<std::string>("VerySlowXpGain.AchievementReward", ""));
            LoadStringToMap(sChallengeModes->questXpOnlyAchievementReward, sConfigMgr->GetOption<std::string>("QuestXpOnly.AchievementReward", ""));
            LoadStringToMap(sChallengeModes->ironManAchievementReward, sConfigMgr->GetOption<std::string>("IronMan.AchievementReward", ""));
        }
    }
};

class ChallengeMode : public PlayerScript
{
public:
    explicit ChallengeMode(const char *scriptName,
                           ChallengeModeSettings settingName)
            : PlayerScript(scriptName), settingName(settingName)
    { }

    static bool mapContainsKey(const std::unordered_map<uint8, uint32>* mapToCheck, uint8 key)
    {
        return (mapToCheck->find(key) != mapToCheck->end());
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(settingName, player))
        {
            return;
        }
        amount *= sChallengeModes->getXpBonusForChallenge(settingName);
    }

void OnPlayerLevelChanged(Player* player, uint8 /*oldlevel*/) override
{
    if (!sChallengeModes->challengeEnabledForPlayer(settingName, player))
    {
        return;
    }

    const std::unordered_map<uint8, uint32>* titleRewardMap = sChallengeModes->getTitleMapForChallenge(settingName);
    const std::unordered_map<uint8, uint32>* talentRewardMap = sChallengeModes->getTalentMapForChallenge(settingName);
    const std::unordered_map<uint8, uint32>* itemRewardMap = sChallengeModes->getItemMapForChallenge(settingName);
    const std::unordered_map<uint8, uint32>* achievementRewardMap = sChallengeModes->getAchievementMapForChallenge(settingName);
    uint8 level = player->GetLevel();

    if (mapContainsKey(titleRewardMap, level))
    {
        CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(titleRewardMap->at(level));
        if (!titleInfo)
        {
            LOG_ERROR("mod-challenge-modes", "Invalid title ID {}!", titleRewardMap->at(level));
            return;
        }
        ChatHandler handler(player->GetSession());
        std::string tNameLink = handler.GetNameLink(player);
        std::string titleNameStr = Acore::StringFormat(player->getGender() == GENDER_MALE ? titleInfo->nameMale[handler.GetSessionDbcLocale()] : titleInfo->nameFemale[handler.GetSessionDbcLocale()], player->GetName());
        player->SetTitle(titleInfo);
    }

    //if (mapContainsKey(talentRewardMap, level))
    //{
    //    uint8 talents[3] = { 0, 0, 0 };

    //    // 调用函数，获取天赋点数
    //    //player->GetTalentTreePoints(talents);

    //    //// 计算总天赋点数
    //    //uint8 totalPoints = talents[0] + talents[1] + talents[2];
    //    if (player->CalculateTalentsPoints()< 97) //+GetFreeTalentPoints() 限制最多天赋点(101-5)
    //        player->RewardExtraBonusTalentPoints(talentRewardMap->at(level));
    //}

    if (mapContainsKey(achievementRewardMap, level))
    {
        AchievementEntry const* achievementInfo = sAchievementStore.LookupEntry(achievementRewardMap->at(level));
        if (!achievementInfo)
        {
            LOG_ERROR("mod-challenge-modes", "Invalid Achievement ID {}!", achievementRewardMap->at(level));
            return;
        }

        ChatHandler handler(player->GetSession());
        std::string tNameLink = handler.GetNameLink(player);
        player->CompletedAchievement(achievementInfo);
    }

    if (mapContainsKey(itemRewardMap, level))
    {
        uint32 itemEntry = itemRewardMap->at(level);
        uint32 itemAmount = sChallengeModes->getItemRewardAmount(settingName); // Fetch item amount from config
        player->SendItemRetrievalMail({ { itemEntry, itemAmount } });
        // 添加金币奖励
        uint32 goldAmount = 1 * GOLD; // 每次奖励1金币
        if (player->GetMoney() < 5 * goldAmount)
        {
            player->ModifyMoney(goldAmount);
            ChatHandler(player->GetSession()).PSendSysMessage("你已获得额外奖励：%u金币！", goldAmount / GOLD);
        }
    }
    if (sChallengeModes->getDisableLevel(settingName) && sChallengeModes->getDisableLevel(settingName) <= level)
    {
        player->UpdatePlayerSetting("mod-challenge-modes", settingName, 0);
    }
}

private:
    ChallengeModeSettings settingName;
};

void AnnounceDeathToScreen(Player* killed, const std::string& killerName, const std::string& location)
{
    std::string zhiye;
    switch (killed->getClass())
    {
        case CLASS_WARRIOR: // TITLE Warrior
            zhiye = "战士";
            break;
        case CLASS_PALADIN: // TITLE Paladin
            zhiye = "骑士";
            break;
        case CLASS_HUNTER: // TITLE Hunter
            zhiye = "猎人";
            break;
        case CLASS_ROGUE: // TITLE Rogue
            zhiye = "盗贼";
            break;
        case CLASS_PRIEST: // TITLE Priest
            zhiye = "牧师";
            break;
        case CLASS_DEATH_KNIGHT: // TITLE Death Knight
            zhiye = "死亡骑士";
            break;
        case CLASS_SHAMAN: // TITLE Shaman
            zhiye = "萨满";
            break;
        case CLASS_MAGE: // TITLE Mage
            zhiye = "法师";
            break;
        case CLASS_WARLOCK: // TITLE Warlock
            zhiye = "术士";
            break;
        case CLASS_DRUID: // TITLE Druid
            zhiye = "德鲁伊";
            break;
    default:
        zhiye = "猎人";
        break;
    }
    // 构建消息
    std::string message = Acore::StringFormat(
        "|TInterface\\Icons\\Spell_Nature_WispSplode:24:24|t |cffFFFFFF『吃席通告』|r 普天同庆开席啦：挑战模式{}玩家〖{}〗在{}被☾{}☽击杀！享年{}级！！",
        zhiye, killed->GetName(), location, killerName, killed->GetLevel());

    // 创建屏幕消息数据包 (如果支持字体大小调整的协议)
    WorldPacket data(SMSG_NOTIFICATION, message.size() + 1); // 确保 +1 为 '\0'
    data << message;
    sWorldSessionMgr->SendServerMessage(SERVER_MSG_STRING, message);
    // 向所有玩家发送
    sWorldSessionMgr->SendGlobalMessage(&data);

}

class ChallengeMode_Hardcore : public ChallengeMode
{
public:
    ChallengeMode_Hardcore() : ChallengeMode("ChallengeMode_Hardcore", SETTING_HARDCORE) {}

    //void OnLogin(Player* player) override
    //{
    //    if (!sChallengeModes->challengeEnabledForPlayer(SETTING_HARDCORE, player) || !sChallengeModes->challengeEnabledForPlayer(HARDCORE_DEAD, player))
    //    {
    //        return;
    //    }
    //    player->KillPlayer();
    //    player->GetSession()->KickPlayer("Hardcore character died");
    //}
    void OnPlayerLogin(Player* player) override
    {
        // 安全检查：确保 player 和 sChallengeModes 是有效的
        if (!player || !sChallengeModes)
        {
            return; // 如果 player 或 sChallengeModes 无效，直接返回
        }
        // 检查玩家是否启用了Hardcore模式
        if (sChallengeModes->challengeEnabledForPlayer(SETTING_HARDCORE, player))
        {
            // 如果玩家在Hardcore模式下已经死亡
            if (sChallengeModes->challengeEnabledForPlayer(HARDCORE_DEAD, player))
            {
                player->KillPlayer(); // 杀死玩家
                player->GetSession()->KickPlayer("Hardcore character died"); // 踢出玩家并提示
            }
            else
            {
                // 如果玩家启用了Hardcore模式但未死亡，发送提示消息
                ChatHandler(player->GetSession()).PSendSysMessage("你已开启挑战(一命)模式，请注意安全！");
                return;
            }
        }
    }

    void OnPlayerReleasedGhost(Player* player) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_HARDCORE, player))
        {
            return;
        }
        player->UpdatePlayerSetting("mod-challenge-modes", HARDCORE_DEAD, 1);
        player->GetSession()->KickPlayer("Hardcore character died");
    }

    void OnPlayerPVPKill(Player* killer, Player* killed) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_HARDCORE, killed) || killed->InBattleground() || killed->InArena())
        {
            return;
        }
        killed->UpdatePlayerSetting("mod-challenge-modes", HARDCORE_DEAD, 1);
        // 全服通报
        std::string killerName = killer ? killer->GetNameForLocaleIdx(LOCALE_zhCN) : "未知";
        std::string location = killed->GetMap() ? killed->GetMap()->GetMapName() : "未知地点";

        // 调用屏幕消息
        AnnounceDeathToScreen(killed, killerName, location);
    }

    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_HARDCORE, killed))
        {
            return;
        }
        killed->UpdatePlayerSetting("mod-challenge-modes", HARDCORE_DEAD, 1);
        // 全服通报
        std::string killerName = killer ? killer->GetNameForLocaleIdx(LOCALE_zhCN) : "未知";
        std::string location = killed->GetMap() ? killed->GetMap()->GetMapName() : "未知地点";

        // 调用屏幕消息
        AnnounceDeathToScreen(killed, killerName, location);
    }

    void OnPlayerResurrect(Player* player, float /*restore_percent*/, bool /*applySickness*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_HARDCORE, player) || player->InBattleground() || player->InArena())
        {
            return;
        }
        // A better implementation is to not allow the resurrect but this will need a new hook added first
        player->UpdatePlayerSetting("mod-challenge-modes", HARDCORE_DEAD, 1);
        player->KillPlayer();
        player->GetSession()->KickPlayer("Hardcore character died");
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        ChallengeMode::OnPlayerGiveXP(player, amount, victim, xpSource);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldlevel) override
    {
        ChallengeMode::OnPlayerLevelChanged(player, oldlevel);
    }

    bool OnPlayerCanGroupInvite(Player* player, std::string& memberName) override
    {
        // 获取被邀请者的对象
        Player* invitedPlayer = ObjectAccessor::FindPlayerByName(memberName);
        if (!invitedPlayer)
        {
            return false; // 如果找不到玩家对象，不能邀请
        }

        // 检查等级差异是否超过5级
        if (std::abs(static_cast<int>(player->GetLevel()) - static_cast<int>(invitedPlayer->GetLevel())) > 5)
        {
            return false; // 等级相差超过5，不能邀请
        }

        // 如果不受挑战模式限制，允许邀请
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_HARDCORE, player))
        {
            return true;
        }

        return true; // 默认情况下不允许邀请
    }

    bool OnPlayerCanGroupAccept(Player* player, Group* group) override
    {
        // 获取队伍中的玩家列表
        if (!group)
        {
            return false; // 如果队伍为空，不允许接受
        }

        for (GroupReference const* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* groupMember = itr->GetSource();
            if (!groupMember || groupMember == player)
            {
                continue; // 跳过自己或无效的成员
            }

            // 检查等级差异是否超过5级
            if (std::abs(static_cast<int>(player->GetLevel()) - static_cast<int>(groupMember->GetLevel())) > 5)
            {
                return false; // 等级相差超过5，不允许加入
            }
        }

        // 如果不受挑战模式限制，允许接受
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_HARDCORE, player))
        {
            return true;
        }

        return true; // 默认情况下不允许接受
    }
};

class ChallengeMode_SemiHardcore : public ChallengeMode
{
public:
    ChallengeMode_SemiHardcore() : ChallengeMode("ChallengeMode_SemiHardcore", SETTING_SEMI_HARDCORE) {}

    void OnPlayerKilledByCreature(Creature* killer, Player* player) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_SEMI_HARDCORE, player))
        {
            return;
        }
        for (uint8 i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                if (pItem->GetTemplate() && !pItem->IsEquipped())
                    continue;
                uint8 slot = pItem->GetSlot();
                ChatHandler(player->GetSession()).PSendSysMessage("|cffDA70D6你失去了你的装备 |cffffffff|Hitem:%d:0:0:0:0:0:0:0:0|h[%s]|h|r", pItem->GetEntry(), pItem->GetTemplate()->Name1.c_str());
                player->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);
            }
        }
        player->SetMoney(0);
        // 全服通报
        std::string killerName = killer ? killer->GetNameForLocaleIdx(LOCALE_zhCN) : "未知";
        std::string location = player->GetMap() ? player->GetMap()->GetMapName() : "未知地点";

        // 调用屏幕消息
        AnnounceDeathToScreen(player, killerName, location);
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        ChallengeMode::OnPlayerGiveXP(player, amount, victim, xpSource);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldlevel) override
    {
        ChallengeMode::OnPlayerLevelChanged(player, oldlevel);
    }

    bool OnPlayerCanGroupInvite(Player* player, std::string& memberName) override
    {
        // 获取被邀请者的对象
        Player* invitedPlayer = ObjectAccessor::FindPlayerByName(memberName);
        if (!invitedPlayer)
        {
            return false; // 如果找不到玩家对象，不能邀请
        }

        // 检查等级差异是否超过5级
        if (std::abs(static_cast<int>(player->GetLevel()) - static_cast<int>(invitedPlayer->GetLevel())) > 5)
        {
            return false; // 等级相差超过5，不能邀请
        }

        // 如果不受挑战模式限制，允许邀请
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_SEMI_HARDCORE, player))
        {
            return true;
        }

        return true; // 默认情况下允许邀请
    }

    bool OnPlayerCanGroupAccept(Player* player, Group* group) override
    {
        // 获取队伍中的玩家列表
        if (!group)
        {
            return false; // 如果队伍为空，不允许接受
        }

        for (GroupReference const* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* groupMember = itr->GetSource();
            if (!groupMember || groupMember == player)
            {
                continue; // 跳过自己或无效的成员
            }

            // 检查等级差异是否超过5级
            if (std::abs(static_cast<int>(player->GetLevel()) - static_cast<int>(groupMember->GetLevel())) > 5)
            {
                return false; // 等级相差超过5，不允许加入
            }
        }

        // 如果不受挑战模式限制，允许接受
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_SEMI_HARDCORE, player))
        {
            return true;
        }

        return true; // 默认情况下允许接受
    }
};

class ChallengeMode_SelfCrafted : public ChallengeMode
{
public:
    ChallengeMode_SelfCrafted() : ChallengeMode("ChallengeMode_SelfCrafted", SETTING_SELF_CRAFTED) {}

    bool OnPlayerCanEquipItem(Player* player, uint8 /*slot*/, uint16& /*dest*/, Item* pItem, bool /*swap*/, bool /*not_loading*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_SELF_CRAFTED, player))
        {
            return true;
        }
        if (!pItem->GetTemplate()->HasSignature())
        {
            return false;
        }
        return pItem->GetGuidValue(ITEM_FIELD_CREATOR) == player->GetGUID();
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        ChallengeMode::OnPlayerGiveXP(player, amount, victim, xpSource);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldlevel) override
    {
        ChallengeMode::OnPlayerLevelChanged(player, oldlevel);
    }
};

class ChallengeMode_ItemQualityLevel : public ChallengeMode
{
public:
    ChallengeMode_ItemQualityLevel() : ChallengeMode("ChallengeMode_ItemQualityLevel", SETTING_ITEM_QUALITY_LEVEL) {}

    bool OnPlayerCanEquipItem(Player* player, uint8 /*slot*/, uint16& /*dest*/, Item* pItem, bool /*swap*/, bool /*not_loading*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_ITEM_QUALITY_LEVEL, player))
        {
            return true;
        }
        return pItem->GetTemplate()->Quality <= ITEM_QUALITY_NORMAL;
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        ChallengeMode::OnPlayerGiveXP(player, amount, victim, xpSource);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldlevel) override
    {
        ChallengeMode::OnPlayerLevelChanged(player, oldlevel);
    }
};

class ChallengeMode_SlowXpGain : public ChallengeMode
{
public:
    ChallengeMode_SlowXpGain() : ChallengeMode("ChallengeMode_SlowXpGain", SETTING_SLOW_XP_GAIN) {}

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        ChallengeMode::OnPlayerGiveXP(player, amount, victim, xpSource);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldlevel) override
    {
        ChallengeMode::OnPlayerLevelChanged(player, oldlevel);
    }
};

class ChallengeMode_VerySlowXpGain : public ChallengeMode
{
public:
    ChallengeMode_VerySlowXpGain() : ChallengeMode("ChallengeMode_VerySlowXpGain", SETTING_VERY_SLOW_XP_GAIN) {}

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        ChallengeMode::OnPlayerGiveXP(player, amount, victim, xpSource);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldlevel) override
    {
        ChallengeMode::OnPlayerLevelChanged(player, oldlevel);
    }

    bool OnPlayerCanGroupInvite(Player* player, std::string& memberName) override
    {
        // 获取被邀请者的对象
        Player* invitedPlayer = ObjectAccessor::FindPlayerByName(memberName);
        if (!invitedPlayer)
        {
            return false; // 如果找不到玩家对象，不能邀请
        }

        // 检查等级差异是否超过5级
        if (std::abs(static_cast<int>(player->GetLevel()) - static_cast<int>(invitedPlayer->GetLevel())) > 5)
        {
            return false; // 等级相差超过5，不能邀请
        }

        // 如果不受挑战模式限制，允许邀请
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_VERY_SLOW_XP_GAIN, player))
        {
            return true;
        }

        return true; // 默认情况下允许邀请
    }

    bool OnPlayerCanGroupAccept(Player* player, Group* group) override
    {
        // 获取队伍中的玩家列表
        if (!group)
        {
            return false; // 如果队伍为空，不允许接受
        }

        for (GroupReference const* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* groupMember = itr->GetSource();
            if (!groupMember || groupMember == player)
            {
                continue; // 跳过自己或无效的成员
            }

            // 检查等级差异是否超过5级
            if (std::abs(static_cast<int>(player->GetLevel()) - static_cast<int>(groupMember->GetLevel())) > 5)
            {
                return false; // 等级相差超过5，不允许加入
            }
        }

        // 如果不受挑战模式限制，允许接受
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_VERY_SLOW_XP_GAIN, player))
        {
            return true;
        }

        return true; // 默认情况下允许接受
    }
};

class ChallengeMode_QuestXpOnly : public ChallengeMode
{
public:
    ChallengeMode_QuestXpOnly() : ChallengeMode("ChallengeMode_QuestXpOnly", SETTING_QUEST_XP_ONLY) {}

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_QUEST_XP_ONLY, player))
        {
            return;
        }
        if (victim)
        {
            // Still award XP to pets - they won't be able to pass the player's level
            Pet* pet = player->GetPet();
            if (pet && xpSource == XPSOURCE_KILL)
                pet->GivePetXP(player->GetGroup() ? amount / 2 : amount);
            amount = 0;
        }
        else
        {
            ChallengeMode::OnPlayerGiveXP(player, amount, victim, xpSource);
        }
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldlevel) override
    {
        ChallengeMode::OnPlayerLevelChanged(player, oldlevel);
    }
};

class ChallengeMode_IronMan : public ChallengeMode
{
public:
    ChallengeMode_IronMan() : ChallengeMode("ChallengeMode_IronMan", SETTING_IRON_MAN) {}

    void OnPlayerResurrect(Player* player, float /*restore_percent*/, bool /*applySickness*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_IRON_MAN, player))
        {
            return;
        }
        // A better implementation is to not allow the resurrect but this will need a new hook added first
        player->KillPlayer();
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        ChallengeMode::OnPlayerGiveXP(player, amount, victim, xpSource);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldlevel) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_IRON_MAN, player))
        {
            return;
        }
        player->SetFreeTalentPoints(0); // Remove all talent points
        ChallengeMode::OnPlayerLevelChanged(player, oldlevel);
    }

    void OnPlayerTalentsReset(Player* player, bool /*noCost*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_IRON_MAN, player))
        {
            return;
        }
        player->SetFreeTalentPoints(0); // Remove all talent points
    }

    bool OnPlayerCanEquipItem(Player* player, uint8 /*slot*/, uint16& /*dest*/, Item* pItem, bool /*swap*/, bool /*not_loading*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_IRON_MAN, player))
        {
            return true;
        }
        return pItem->GetTemplate()->Quality <= ITEM_QUALITY_NORMAL;
    }

    bool OnPlayerCanApplyEnchantment(Player* player, Item* /*item*/, EnchantmentSlot /*slot*/, bool /*apply*/, bool /*apply_dur*/, bool /*ignore_condition*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_IRON_MAN, player))
        {
            return true;
        }
        // Are there any exceptions in WotLK? If so need to be added here
        return false;
    }

    void OnPlayerLearnSpell(Player* player, uint32 spellID) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_IRON_MAN, player))
        {
            return;
        }
        // These professions are class skills so they are always acceptable
        switch (spellID)
        {
            case RUNEFORGING:
            case POISONS:
            case BEAST_TRAINING:
                return;
            default:
                break;
        }
        // Do not allow learning any trade skills
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID);
        if (!spellInfo)
            return;
        bool shouldForget = false;
        for (uint8 i = 0; i < 3; i++)
        {
            if (spellInfo->Effects[i].Effect == SPELL_EFFECT_TRADE_SKILL)
            {
                shouldForget = true;
            }
        }
        if (shouldForget)
        {
            player->removeSpell(spellID, SPEC_MASK_ALL, false);
        }
    }

    bool OnPlayerCanUseItem(Player* player, ItemTemplate const* proto, InventoryResult& /*result*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_IRON_MAN, player))
        {
            return true;
        }
        // Do not allow using elixir, potion, or flask
        if (proto->Class == ITEM_CLASS_CONSUMABLE &&
                (proto->SubClass == ITEM_SUBCLASS_POTION ||
                proto->SubClass == ITEM_SUBCLASS_ELIXIR ||
                proto->SubClass == ITEM_SUBCLASS_FLASK))
        {
            return false;
        }
        // Do not allow food that gives food buffs
        if (proto->Class == ITEM_CLASS_CONSUMABLE && proto->SubClass == ITEM_SUBCLASS_FOOD)
        {
            for (const auto & Spell : proto->Spells)
            {
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(Spell.SpellId);
                if (!spellInfo)
                    continue;

                for (uint8 i = 0; i < 3; i++)
                {
                    if (spellInfo->Effects[i].ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool OnPlayerCanGroupInvite(Player* player, std::string& /*membername*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_IRON_MAN, player))
        {
            return true;
        }
        return false;
    }

    bool OnPlayerCanGroupAccept(Player* player, Group* /*group*/) override
    {
        if (!sChallengeModes->challengeEnabledForPlayer(SETTING_IRON_MAN, player))
        {
            return true;
        }
        return false;
    }

};

class gobject_challenge_modes : public GameObjectScript
{
private:
    static bool playerSettingEnabled(Player* player, uint8 settingIndex)
    {
        return player->GetPlayerSetting("mod-challenge-modes", settingIndex).value;
    }

public:
    gobject_challenge_modes() : GameObjectScript("gobject_challenge_modes") { }

    struct gobject_challenge_modesAI: GameObjectAI
    {
        explicit gobject_challenge_modesAI(GameObject* object) : GameObjectAI(object) { };

        bool CanBeSeen(Player const* player) override
        {
            if ((player->GetLevel() > 1 && player->getClass() != CLASS_DEATH_KNIGHT) || (player->GetLevel() > 55))
            {
                return false;
            }
            return sChallengeModes->enabled();
        }
    };

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if ((player->GetLevel() > 1 && player->getClass() != CLASS_DEATH_KNIGHT) || (player->GetLevel() > 55))
        {
            return false;
        }
        if (sChallengeModes->challengeEnabled(SETTING_HARDCORE) && !playerSettingEnabled(player, SETTING_HARDCORE) && !playerSettingEnabled(player, SETTING_SEMI_HARDCORE))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "硬核模式-死亡玩家将无法复活", 0, SETTING_HARDCORE);
        }
        if (sChallengeModes->challengeEnabled(SETTING_SEMI_HARDCORE) && !playerSettingEnabled(player, SETTING_HARDCORE) && !playerSettingEnabled(player, SETTING_SEMI_HARDCORE))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "半硬核模式-死亡玩家将失去所有穿戴的装备和携带的金币", 0, SETTING_SEMI_HARDCORE);
        }
        if (sChallengeModes->challengeEnabled(SETTING_SELF_CRAFTED) && !playerSettingEnabled(player, SETTING_SELF_CRAFTED) && !playerSettingEnabled(player, SETTING_IRON_MAN))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "自给模式-玩家只能穿戴自己制作的装备", 0, SETTING_SELF_CRAFTED);
        }
        if (sChallengeModes->challengeEnabled(SETTING_ITEM_QUALITY_LEVEL) && !playerSettingEnabled(player, SETTING_ITEM_QUALITY_LEVEL))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "乞丐模式-玩家只能穿戴品质一般或较差的装备", 0, SETTING_ITEM_QUALITY_LEVEL);
        }
        if (sChallengeModes->challengeEnabled(SETTING_SLOW_XP_GAIN) && !playerSettingEnabled(player, SETTING_SLOW_XP_GAIN) && !playerSettingEnabled(player, SETTING_VERY_SLOW_XP_GAIN))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "龟速模式-玩家获得经验降为1/2", 0, SETTING_SLOW_XP_GAIN);
        }
        if (sChallengeModes->challengeEnabled(SETTING_VERY_SLOW_XP_GAIN) && !playerSettingEnabled(player, SETTING_SLOW_XP_GAIN) && !playerSettingEnabled(player, SETTING_VERY_SLOW_XP_GAIN))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "蜗牛模式-玩家获得经验降为1/4", 0, SETTING_VERY_SLOW_XP_GAIN);
        }
        if (sChallengeModes->challengeEnabled(SETTING_QUEST_XP_ONLY) && !playerSettingEnabled(player, SETTING_QUEST_XP_ONLY))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "任务模式-玩家只能通过任务获得经验", 0, SETTING_QUEST_XP_ONLY);
        }
        if (sChallengeModes->challengeEnabled(SETTING_IRON_MAN) && !playerSettingEnabled(player, SETTING_IRON_MAN) && !playerSettingEnabled(player, SETTING_SELF_CRAFTED))
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "炼狱模式 = 硬核模式+自给模式+不能组队+不能交易+不能吃喝+不能附魔", 0, SETTING_IRON_MAN);
        }
        // 新增选项：直接升到70级
        //if (player->GetLevel() < 70)
        //    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "不选择任何特殊模式，直接升级到70级", 0, 9999);
        SendGossipMenuFor(player, 12669, go->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, GameObject* /*go*/, uint32 /*sender*/, uint32 action) override
    {
        if (action == 9999) // 处理直接升级到70级的逻辑
        {
            if (playerSettingEnabled(player, SETTING_IRON_MAN) || playerSettingEnabled(player, SETTING_HARDCORE) || playerSettingEnabled(player, SETTING_SEMI_HARDCORE) ||
                playerSettingEnabled(player, SETTING_SELF_CRAFTED) || playerSettingEnabled(player, SETTING_ITEM_QUALITY_LEVEL) || playerSettingEnabled(player, SETTING_SLOW_XP_GAIN) ||
                playerSettingEnabled(player, SETTING_VERY_SLOW_XP_GAIN) || playerSettingEnabled(player, SETTING_QUEST_XP_ONLY))
            {
                ChatHandler(player->GetSession()).PSendSysMessage("你已激活了挑战模式无法再秒升70级！");
                return true;
            }
            if (player->GetLevel() < 70)
            {
                player->GiveLevel(70); // 提升玩家等级到70
                ChatHandler(player->GetSession()).PSendSysMessage("你已直接升级到70级！");
                CloseGossipMenuFor(player);
                return true;
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("你已经达到70级或更高，无需再次升级。");
                CloseGossipMenuFor(player);
                return true;
            }
        }
        else
        {
            if ((player->GetLevel() > 1 && player->getClass() != CLASS_DEATH_KNIGHT) || (player->GetLevel() > 55))
            {
                return false;
            }
            player->UpdatePlayerSetting("mod-challenge-modes", action, 1);
            //ChatHandler(player->GetSession()).PSendSysMessage("你选择的挑战模式已经激活.");
            CloseGossipMenuFor(player);
            // 全服通报
            std::string location = player->GetMap() ? player->GetMap()->GetMapName() : "未知地点";
            // 构建消息
            std::string message = Acore::StringFormat("|cffFFFFFF『挑战公告』|r 玩家☆☆{}☆☆在{}开启挑战模式！",
                player->GetName(), location);

            //ChatHandler(player->GetSession()).SendWorldText(message);
            sWorldSessionMgr->SendServerMessage(SERVER_MSG_STRING, message);
            // 创建屏幕消息数据包 (如果支持字体大小调整的协议)
            WorldPacket data(SMSG_NOTIFICATION, message.size() + 1); // 确保 +1 为 '\0'
            data << message;

            //// 向所有玩家发送
            sWorldSessionMgr->SendGlobalMessage(&data);
            return true;
        }
    }

    GameObjectAI* GetAI(GameObject* object) const override
    {
        return new gobject_challenge_modesAI(object);
    }
};

// Add all scripts in one
void AddSC_mod_challenge_modes()
{
    new ChallengeModes_WorldScript();
    new gobject_challenge_modes();
    new ChallengeMode_Hardcore();
    new ChallengeMode_SemiHardcore();
    new ChallengeMode_SelfCrafted();
    new ChallengeMode_ItemQualityLevel();
    new ChallengeMode_SlowXpGain();
    new ChallengeMode_VerySlowXpGain();
    new ChallengeMode_QuestXpOnly();
    new ChallengeMode_IronMan();
}
