/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AcceptInvitationAction.h"

#include "Event.h"
#include "ObjectAccessor.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotSecurity.h"
#include "Playerbots.h"
#include "WorldPacket.h"
// ================== IP邀请限制系统（高并发版） ==================
#include <array>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// ObjectGuid hash（必须）
struct GuidHash
{
    std::size_t operator()(ObjectGuid const& guid) const { return std::hash<uint64>()(guid.GetRawValue()); }
};

static constexpr size_t SHARD_COUNT = 16;

// 分片结构
struct InviteShard
{
    std::unordered_map<std::string, std::unordered_set<ObjectGuid, GuidHash>> map;
    std::mutex mutex;
};

static std::array<InviteShard, SHARD_COUNT> sInviteShards;

// 分片函数
static size_t GetShardIndex(const std::string& ip) { return std::hash<std::string>()(ip) % SHARD_COUNT; }

// 获取IP
std::string AcceptInvitationAction::GetPlayerIP(Player* player)
{
    if (!player || !player->GetSession())
        return "";

    return player->GetSession()->GetRemoteAddress();
}

// 清理无效bot（锁优化版）
void AcceptInvitationAction::CleanupInviteMapForIP(Player* inviter)
{
    if (!inviter)
        return;

    std::string ip = GetPlayerIP(inviter);
    if (ip.empty())
        return;

    size_t shardIdx = GetShardIndex(ip);
    auto& shard = sInviteShards[shardIdx];

    std::vector<ObjectGuid> bots;

    // 复制数据（短锁）
    {
        std::lock_guard<std::mutex> lock(shard.mutex);

        auto it = shard.map.find(ip);
        if (it == shard.map.end())
            return;

        bots.assign(it->second.begin(), it->second.end());
    }

    Group* group = inviter->GetGroup();
    std::vector<ObjectGuid> toRemove;

    // 锁外处理（避免卡顿）
    for (auto& guid : bots)
    {
        Player* bot = ObjectAccessor::FindPlayer(guid);

        if (!bot || !group || !group->IsMember(guid))
            toRemove.push_back(guid);
    }

    // 删除（短锁）
    {
        std::lock_guard<std::mutex> lock(shard.mutex);

        auto it = shard.map.find(ip);
        if (it == shard.map.end())
            return;

        for (auto& guid : toRemove)
            it->second.erase(guid);

        if (it->second.empty())
            shard.map.erase(it);
    }
}

bool AcceptInvitationAction::Execute(Event event)
{
    Group* grp = bot->GetGroupInvite();
    if (!grp)
        return false;

    WorldPacket packet = event.getPacket();
    uint8 flag;
    std::string name;
    packet >> flag >> name;

    Player* inviter = ObjectAccessor::FindPlayer(grp->GetLeaderGUID());
    if (!inviter)
        return false;

    // ================= IP限制 =================
    std::string ip = GetPlayerIP(inviter);

    // 清理无效bot
    CleanupInviteMapForIP(inviter);

    size_t shardIdx = GetShardIndex(ip);
    auto& shard = sInviteShards[shardIdx];

    // 检查上限
    {
        std::lock_guard<std::mutex> lock(shard.mutex);

        auto& botSet = shard.map[ip];

        if (botSet.size() >= 4)
        {
            WorldPacket data(SMSG_GROUP_DECLINE, 10);
            data << bot->GetName();
            inviter->SendDirectMessage(&data);

            bot->UninviteFromGroup();

            botAI->TellMaster("当前IP下机器人已达上限（最多4个）");

            return false;
        }
    }
    // =========================================

    // 原安全检查
    if (!botAI->GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, false, inviter))
    {
        WorldPacket data(SMSG_GROUP_DECLINE, 10);
        data << bot->GetName();
        inviter->SendDirectMessage(&data);
        bot->UninviteFromGroup();
        return false;
    }

    if (bot->isAFK())
        bot->ToggleAFK();

    WorldPacket p;
    uint32 roles_mask = 0;
    p << roles_mask;
    bot->GetSession()->HandleGroupAcceptOpcode(p);

    // 必须确认进组成功
    if (!bot->GetGroup() || !bot->GetGroup()->IsMember(inviter->GetGUID()))
        return false;

    // ================= 记录bot =================
    {
        std::lock_guard<std::mutex> lock(shard.mutex);
        shard.map[ip].insert(bot->GetGUID());
    }
    // ==========================================

    // ================= 原逻辑 =================

    for (uint8 d = 0; d < MAX_DIFFICULTY; ++d)
    {
        std::vector<InstanceSave*> toUnbind;
        BoundInstancesMap const& m_boundInstances =
            sInstanceSaveMgr->PlayerGetBoundInstances(bot->GetGUID(), Difficulty(d));

        for (auto const& itr : m_boundInstances)
            toUnbind.push_back(itr.second.save);

        for (auto const& inst : toUnbind)
            sInstanceSaveMgr->PlayerUnbindInstance(bot->GetGUID(), inst->GetMapId(), inst->GetDifficulty(), true, bot);
    }

    if (sRandomPlayerbotMgr.IsRandomBot(bot))
        botAI->SetMaster(inviter);

    botAI->ResetStrategies();
    botAI->ChangeStrategy("+follow,-lfg,-bg", BOT_STATE_NON_COMBAT);
    botAI->Reset();

    botAI->TellMaster("你好,请多关照");

    if (sPlayerbotAIConfig.summonWhenGroup && bot->GetDistance(inviter) > sPlayerbotAIConfig.sightDistance)
    {
        Teleport(inviter, bot, true);
    }

    return true;
}
/*bool AcceptInvitationAction::Execute(Event event)
{
    Group* grp = bot->GetGroupInvite();
    if (!grp)
        return false;
    WorldPacket packet = event.getPacket();
    uint8 flag;
    std::string name;
    packet >> flag >> name;

    Player* inviter = ObjectAccessor::FindPlayer(grp->GetLeaderGUID());
    if (!inviter)
        return false;

    if (!botAI->GetSecurity()->CheckLevelFor(PLAYERBOT_SECURITY_INVITE, false, inviter))
        //||(!bot->HasHealSpec() && !inviter->IsGameMaster()))  // && !bot->HasHealSpec())//允许奶妈组队
    {
        WorldPacket data(SMSG_GROUP_DECLINE, 10);
        data << bot->GetName();
        inviter->SendDirectMessage(&data);
        bot->UninviteFromGroup();
        return false;
    }

    if (bot->isAFK())
        bot->ToggleAFK();

    WorldPacket p;
    uint32 roles_mask = 0;
    p << roles_mask;
    bot->GetSession()->HandleGroupAcceptOpcode(p);

    if (!bot->GetGroup() || !bot->GetGroup()->IsMember(inviter->GetGUID()))
        return false;

    //重置团本CD
    for (uint8 d = 0; d < MAX_DIFFICULTY; ++d)
    {
        std::vector<InstanceSave*> toUnbind;
        BoundInstancesMap const& m_boundInstances =
            sInstanceSaveMgr->PlayerGetBoundInstances(bot->GetGUID(), Difficulty(d));
        for (BoundInstancesMap::const_iterator itr = m_boundInstances.begin(); itr != m_boundInstances.end(); ++itr)
        {
            InstanceSave* instanceSave = itr->second.save;
            toUnbind.push_back(instanceSave);
        }
        for (std::vector<InstanceSave*>::const_iterator itr = toUnbind.begin(); itr != toUnbind.end(); ++itr)
            sInstanceSaveMgr->PlayerUnbindInstance(bot->GetGUID(), (*itr)->GetMapId(), (*itr)->GetDifficulty(), true, bot);
    }
    if (sRandomPlayerbotMgr.IsRandomBot(bot))
        botAI->SetMaster(inviter);
    // else
    // PlayerbotRepository::instance().Save(botAI);

    botAI->ResetStrategies();
    botAI->ChangeStrategy("+follow,-lfg,-bg", BOT_STATE_NON_COMBAT);
    botAI->Reset();

    botAI->TellMaster("你好,请多关照");

    if (sPlayerbotAIConfig.summonWhenGroup && bot->GetDistance(inviter) > sPlayerbotAIConfig.sightDistance)
    {
        Teleport(inviter, bot, true);
    }
    return true;
}*/
