/*
 * mod-dungeon-master — npc_dungeon_master.cpp
 * Gossip NPC: menu flow for difficulty/theme/dungeon selection.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "GossipDef.h"
#include "Player.h"
#include "Creature.h"
#include "Group.h"
#include "Log.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "DungeonMasterMgr.h"
#include "RoguelikeMgr.h"
#include "RoguelikeTypes.h"
#include "DMConfig.h"
#include <cstdio>
#include <mutex>
#include <random>

using namespace DungeonMaster;

// Gossip action IDs (encoded so ranges never overlap)
enum DMGossipActions
{
    GOSSIP_ACTION_MAIN_START    = 1,
    GOSSIP_ACTION_MAIN_INFO     = 2,
    GOSSIP_ACTION_MAIN_STATS    = 3,   // unused legacy, kept for safety

    GOSSIP_ACTION_DIFF_BASE     = 100,   // +diffId
    GOSSIP_ACTION_THEME_BASE    = 200,   // +themeId
    GOSSIP_ACTION_DUNGEON_BASE  = 300,   // +mapId (maps go up to ~700)
    GOSSIP_ACTION_DUNGEON_RANDOM = 10000,

    GOSSIP_ACTION_CONFIRM       = 10001,
    GOSSIP_ACTION_CANCEL        = 10002,
    GOSSIP_ACTION_SCALE_PARTY   = 10003,
    GOSSIP_ACTION_SCALE_TIER    = 10004,
    GOSSIP_ACTION_LEADERBOARD   = 10005, // legacy — redirects to board menu

    // Roguelike Mode
    GOSSIP_ACTION_ROGUELIKE_START       = 10010,
    GOSSIP_ACTION_ROGUELIKE_SCALE_PARTY = 10011,
    GOSSIP_ACTION_ROGUELIKE_SCALE_TIER  = 10012,
    GOSSIP_ACTION_ROGUELIKE_THEME       = 10100, // +themeId
    GOSSIP_ACTION_ROGUELIKE_QUIT        = 10200,
    GOSSIP_ACTION_ROGUELIKE_BOARD       = 10201,

    // Statistics & Leaderboards sub-menus
    GOSSIP_ACTION_STATS_MENU          = 10300,  // Stats & Leaderboards hub
    GOSSIP_ACTION_STATS_NORMAL        = 10301,  // My Normal Run Stats
    GOSSIP_ACTION_STATS_ROGUELIKE     = 10302,  // My Roguelike Stats
    GOSSIP_ACTION_BOARD_MENU          = 10310,  // Leaderboards hub
    GOSSIP_ACTION_BOARD_NORMAL        = 10311,  // Normal — Fastest Clears
    GOSSIP_ACTION_BOARD_RL_TIER       = 10312,  // Roguelike — Highest Tier
    GOSSIP_ACTION_BOARD_RL_FLOORS     = 10313,  // Roguelike — Most Floors
};

struct PlayerDMSelection
{
    uint32 DifficultyId  = 0;
    uint32 ThemeId       = 0;
    uint32 MapId         = 0;
    bool   ScaleToParty  = true;
    bool   IsRoguelike   = false;
};

static std::unordered_map<ObjectGuid, PlayerDMSelection> sSelections;
static std::mutex sSelMutex;

class npc_dungeon_master : public CreatureScript
{
public:
    npc_dungeon_master() : CreatureScript("npc_dungeon_master") {}

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!sDMConfig->IsEnabled())
        {
            ChatHandler(player->GetSession()).SendSysMessage(
                "|cFFFF0000[Dungeon Master]|r 副本大师暂未开启.");
            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
        if (sDungeonMasterMgr->GetSessionByPlayer(player->GetGUID()))
        {
            LOG_INFO("module", "DungeonMaster: NPC blocked {} — still in active session",
                player->GetName());
            ChatHandler(player->GetSession()).SendSysMessage(
                "|cFFFF0000[Dungeon Master]|r 你已在闯关副本进程中!");
            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
        if (sRoguelikeMgr->IsPlayerInRun(player->GetGUID()))
        {
            player->PlayerTalkClass->ClearMenus();

            // Player is in an active roguelike run (auto-transitions between dungeons)
            RoguelikeRun* run = sRoguelikeMgr->GetRunByPlayer(player->GetGUID());
            if (run)
            {
                char tierBuf[256];
                snprintf(tierBuf, sizeof(tierBuf),
                    "|cFF00FFFF[闯关挑战]|r 已激活 — |cFFFFD700层级 %u|r, "
                    "|cFFFFFFFF%u|r 层%s 已清理.",
                    run->CurrentTier, run->DungeonsCleared,
                    run->DungeonsCleared != 1 ? "s" : "");
                ChatHandler(player->GetSession()).SendSysMessage(tierBuf);
            }
            else
            {
                ChatHandler(player->GetSession()).SendSysMessage(
                    "|cFF00FFFF[闯关挑战]|r 你已在闯关副本进程中!!");
            }

            AddGossipItemFor(player, GOSSIP_ICON_BATTLE,
                "|cFFFF0000离开闯关副本模式|r",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ROGUELIKE_QUIT);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "取消",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);

            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return true;
        }
        if (sDungeonMasterMgr->IsOnCooldown(player->GetGUID()))
        {
            uint32 rem  = sDungeonMasterMgr->GetRemainingCooldown(player->GetGUID());
            LOG_INFO("module", "DungeonMaster: NPC blocked {} — cooldown {}s remaining",
                player->GetName(), rem);
            char buf[256];
            snprintf(buf, sizeof(buf),
                "|cFFFFFF00[副本大师]|r 等待 |cFFFFFFFF%u|r 分 |cFFFFFFFF%u|r 秒 在下一个闯关挑战前.",
                rem / 60, rem % 60);
            ChatHandler(player->GetSession()).SendSysMessage(buf);
            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
        ShowMainMenu(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        if (action == GOSSIP_ACTION_MAIN_START)
        {
            if (!sDungeonMasterMgr->CanCreateNewSession())
            {
                ChatHandler(player->GetSession()).SendSysMessage(
                    "|cFFFF0000[副本大师]|r 已有过多闯关挑战进程正在进行. 请稍后再试.");
                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections[player->GetGUID()] = {}; }
            ShowDifficultyMenu(player, creature);
        }
        else if (action == GOSSIP_ACTION_MAIN_INFO)
            ShowInfoMenu(player, creature);
        else if (action == GOSSIP_ACTION_MAIN_STATS)
            ShowStatsAndBoardsMenu(player, creature);
        else if (action == GOSSIP_ACTION_LEADERBOARD)
            ShowBoardMenu(player, creature);
        else if (action >= GOSSIP_ACTION_DIFF_BASE && action < GOSSIP_ACTION_THEME_BASE)
        {
            uint32 diffId = action - GOSSIP_ACTION_DIFF_BASE;
            bool isRoguelike = false;
            { std::lock_guard<std::mutex> lk(sSelMutex);
              sSelections[player->GetGUID()].DifficultyId = diffId;
              isRoguelike = sSelections[player->GetGUID()].IsRoguelike; }
            if (isRoguelike)
                ShowRoguelikeScalingMenu(player, creature);
            else
                ShowScalingMenu(player, creature);
        }
        else if (action == GOSSIP_ACTION_SCALE_PARTY)
        {
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections[player->GetGUID()].ScaleToParty = true; }
            ShowThemeMenu(player, creature);
        }
        else if (action == GOSSIP_ACTION_SCALE_TIER)
        {
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections[player->GetGUID()].ScaleToParty = false; }
            ShowThemeMenu(player, creature);
        }
        else if (action >= GOSSIP_ACTION_THEME_BASE && action < GOSSIP_ACTION_DUNGEON_BASE)
        {
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections[player->GetGUID()].ThemeId = action - GOSSIP_ACTION_THEME_BASE; }
            ShowDungeonMenu(player, creature);
        }
        else if (action == GOSSIP_ACTION_DUNGEON_RANDOM)
        {
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections[player->GetGUID()].MapId = 0; }
            ShowConfirmMenu(player, creature);
        }
        else if (action >= GOSSIP_ACTION_DUNGEON_BASE && action < GOSSIP_ACTION_CONFIRM)
        {
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections[player->GetGUID()].MapId = action - GOSSIP_ACTION_DUNGEON_BASE; }
            ShowConfirmMenu(player, creature);
        }
        else if (action == GOSSIP_ACTION_CONFIRM)
            StartChallenge(player, creature);
        else if (action == GOSSIP_ACTION_CANCEL)
        {
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections.erase(player->GetGUID()); }
            ShowMainMenu(player, creature);
        }
        // ---- Roguelike Actions ----
        else if (action == GOSSIP_ACTION_ROGUELIKE_START)
        {
            if (sRoguelikeMgr->IsPlayerInRun(player->GetGUID()))
            {
                ChatHandler(player->GetSession()).SendSysMessage(
                    "|cFFFF0000[闯关挑战]|r 你已在闯关挑战副本进程中!");
                player->PlayerTalkClass->SendCloseGossip();
                return true;
            }
            { std::lock_guard<std::mutex> lk(sSelMutex);
              sSelections[player->GetGUID()] = {};
              sSelections[player->GetGUID()].IsRoguelike = true; }
            ShowRoguelikeDifficultyMenu(player, creature);
        }
        else if (action == GOSSIP_ACTION_ROGUELIKE_SCALE_PARTY)
        {
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections[player->GetGUID()].ScaleToParty = true; }
            ShowRoguelikeThemeMenu(player, creature);
        }
        else if (action == GOSSIP_ACTION_ROGUELIKE_SCALE_TIER)
        {
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections[player->GetGUID()].ScaleToParty = false; }
            ShowRoguelikeThemeMenu(player, creature);
        }
        else if (action >= GOSSIP_ACTION_ROGUELIKE_THEME && action < GOSSIP_ACTION_ROGUELIKE_QUIT)
        {
            uint32 themeId = action - GOSSIP_ACTION_ROGUELIKE_THEME;
            { std::lock_guard<std::mutex> lk(sSelMutex); sSelections[player->GetGUID()].ThemeId = themeId; }
            StartRoguelike(player, creature);
        }
        else if (action == GOSSIP_ACTION_ROGUELIKE_QUIT)
        {
            if (sRoguelikeMgr->IsPlayerInRun(player->GetGUID()))
            {
                sRoguelikeMgr->QuitRun(player->GetGUID());
                ChatHandler(player->GetSession()).SendSysMessage(
                    "|cFF00FFFF[闯关挑战]|r 闯关被中途放弃.");
            }
            player->PlayerTalkClass->SendCloseGossip();
        }
        else if (action == GOSSIP_ACTION_ROGUELIKE_BOARD)
        {
            ShowRoguelikeLeaderboard(player, creature, false);
        }
        // ---- Statistics & Leaderboards ----
        else if (action == GOSSIP_ACTION_STATS_MENU)
            ShowStatsAndBoardsMenu(player, creature);
        else if (action == GOSSIP_ACTION_STATS_NORMAL)
            ShowNormalStats(player, creature);
        else if (action == GOSSIP_ACTION_STATS_ROGUELIKE)
            ShowRoguelikeStats(player, creature);
        else if (action == GOSSIP_ACTION_BOARD_MENU)
            ShowBoardMenu(player, creature);
        else if (action == GOSSIP_ACTION_BOARD_NORMAL)
            ShowNormalLeaderboard(player, creature);
        else if (action == GOSSIP_ACTION_BOARD_RL_TIER)
            ShowRoguelikeLeaderboard(player, creature, false);
        else if (action == GOSSIP_ACTION_BOARD_RL_FLOORS)
            ShowRoguelikeLeaderboard(player, creature, true);
        return true;
    }

private:
    // ---- Menu builders ----

    void ShowMainMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "开始闯关挑战模式",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_MAIN_START);
        if (sDMConfig->IsRoguelikeEnabled())
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|cFF00FFFF闯关挑战模式|r",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ROGUELIKE_START);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "这个系统如何运作?",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_MAIN_INFO);
        AddGossipItemFor(player, GOSSIP_ICON_TABARD, "|cFFFFD700统计数据 & 排行榜|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_STATS_MENU);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowDifficultyMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        uint8 lvl = player->GetLevel();

        for (const auto& d : sDMConfig->GetDifficulties())
        {
            char buf[256];
            if (!d.IsValidForLevel(lvl))
                snprintf(buf, sizeof(buf), "|cFF808080%s|r (等级 %u-%u) - |cFFFF0000需要 %u+|r",
                    d.Name.c_str(), d.MinLevel, d.MaxLevel, d.MinLevel);
            else if (!d.IsOnLevelFor(lvl))
                snprintf(buf, sizeof(buf), "%s |cFF808080(等级 %u-%u — 简单)|r",
                    d.Name.c_str(), d.MinLevel, d.MaxLevel);
            else
                snprintf(buf, sizeof(buf), "|cFF00FF00%s|r (等级 %u-%u)",
                    d.Name.c_str(), d.MinLevel, d.MaxLevel);

            AddGossipItemFor(player,
                d.IsValidForLevel(lvl) ? GOSSIP_ICON_BATTLE : GOSSIP_ICON_CHAT,
                buf, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_DIFF_BASE + d.Id);
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowScalingMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();

        PlayerDMSelection sel;
        { std::lock_guard<std::mutex> lk(sSelMutex);
          auto it = sSelections.find(player->GetGUID());
          if (it == sSelections.end()) { player->PlayerTalkClass->SendCloseGossip(); return; }
          sel = it->second; }

        const DifficultyTier* diff = sDMConfig->GetDifficulty(sel.DifficultyId);
        if (!diff) { player->PlayerTalkClass->SendCloseGossip(); return; }

        uint8 partyLevel = sDungeonMasterMgr->ComputeEffectiveLevel(player);

        char buf1[256], buf2[256];
        snprintf(buf1, sizeof(buf1),
            "|cFF00FF00等级随队伍动态适配|r (等级 %u) — 完全匹配你当前等级的挑战难度",
            partyLevel);
        snprintf(buf2, sizeof(buf2),
            "|cFFFFD700适配的副本难度|r (等级 %u-%u) — 原始难度区间",
            diff->MinLevel, diff->MaxLevel);

        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, buf1,
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_SCALE_PARTY);
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, buf2,
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_SCALE_TIER);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowThemeMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        for (const auto& t : sDMConfig->GetThemes())
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, t.Name, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_THEME_BASE + t.Id);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowDungeonMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();

        uint32 diffId;
        { std::lock_guard<std::mutex> lk(sSelMutex);
          auto it = sSelections.find(player->GetGUID());
          if (it == sSelections.end()) { player->PlayerTalkClass->SendCloseGossip(); return; }
          diffId = it->second.DifficultyId; }

        const DifficultyTier* diff = sDMConfig->GetDifficulty(diffId);
        if (!diff) { player->PlayerTalkClass->SendCloseGossip(); return; }

        auto dungeons = sDMConfig->GetDungeonsForLevel(diff->MinLevel, diff->MaxLevel);

        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|cFFFFD700随机副本|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_DUNGEON_RANDOM);

        for (const auto* dg : dungeons)
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s (等级 %u-%u)", dg->Name.c_str(), dg->MinLevel, dg->MaxLevel);
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, buf,
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_DUNGEON_BASE + dg->MapId);
        }

        if (dungeons.empty())
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                "|cFF808080无激活副本|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowConfirmMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();

        PlayerDMSelection sel;
        { std::lock_guard<std::mutex> lk(sSelMutex);
          auto it = sSelections.find(player->GetGUID());
          if (it == sSelections.end()) { player->PlayerTalkClass->SendCloseGossip(); return; }
          sel = it->second; }

        const DifficultyTier* diff = sDMConfig->GetDifficulty(sel.DifficultyId);
        const Theme*          theme = sDMConfig->GetTheme(sel.ThemeId);

        std::string dgName = "随机副本";
        if (sel.MapId > 0)
            if (const DungeonInfo* dg = sDMConfig->GetDungeon(sel.MapId))
                dgName = dg->Name;

        Group* g = player->GetGroup();
        uint32 ps = g ? g->GetMembersCount() : 1;

        char buf[256];
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFD700========== 闯关挑战概况 ==========|r");
        snprintf(buf, sizeof(buf), "  难度: |cFF00FF00%s|r", diff ? diff->Name.c_str() : "?");
        ChatHandler(player->GetSession()).SendSysMessage(buf);
        snprintf(buf, sizeof(buf), "  平衡:    |cFF00FF00%s|r",
            sel.ScaleToParty ? "队伍等级" : "副本难度");
        ChatHandler(player->GetSession()).SendSysMessage(buf);
        snprintf(buf, sizeof(buf), "  类型:      |cFF00FF00%s|r", theme ? theme->Name.c_str() : "?");
        ChatHandler(player->GetSession()).SendSysMessage(buf);
        snprintf(buf, sizeof(buf), "  副本:    |cFF00FF00%s|r", dgName.c_str());
        ChatHandler(player->GetSession()).SendSysMessage(buf);
        snprintf(buf, sizeof(buf), "  队伍: |cFFFFFFFF%u|r player(s)", ps);
        ChatHandler(player->GetSession()).SendSysMessage(buf);
        if (ps > 1)
            ChatHandler(player->GetSession()).SendSysMessage("|cFFFFFF00  所有队员将被传送!|r");
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFD700========================================|r");

        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|cFF00FF00>> START CHALLENGE <<|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CONFIRM);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 取消|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowInfoMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFD700========= 副本大师闯关挑战 =========|r");
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFFFFF1.|r 选择难度档位");
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFFFFF2.|r 选择难度平衡规则");
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFFFFF3.|r 选择怪物类型");
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFFFFF4.|r 自选副本或随机匹配");
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFFFFF5.|r 你将传送至已清理的副本区域");
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFFFFF6.|r 击败首领即可完成本次挑战");
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFFFFF7.|r 收获金币与装备奖励!");
        ChatHandler(player->GetSession()).SendSysMessage("|cFFFFD700==========================================|r");
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "<< Back", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    // ---- Statistics & Leaderboards Hub ----

    void ShowStatsAndBoardsMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        AddGossipItemFor(player, GOSSIP_ICON_TABARD, "我的副本挑战总战绩",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_STATS_NORMAL);
        if (sDMConfig->IsRoguelikeEnabled())
            AddGossipItemFor(player, GOSSIP_ICON_TABARD, "|cFF00FFFF我的闯关信息|r",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_STATS_ROGUELIKE);
        AddGossipItemFor(player, GOSSIP_ICON_TABARD, "|cFFFFD700英雄榜|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_BOARD_MENU);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    static void FormatTime(uint32 seconds, char* buf, size_t len)
    {
        if (seconds >= 3600)
            snprintf(buf, len, "%uh %02um %02us", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
        else
            snprintf(buf, len, "%um %02us", seconds / 60, seconds % 60);
    }

    void ShowNormalStats(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        PlayerStats st = sDungeonMasterMgr->GetPlayerStats(player->GetGUID());
        auto chat = ChatHandler(player->GetSession());
        char buf[256];

        chat.SendSysMessage("|cFFFFD700═══════════ 副本挑战统计 ═══════════|r");

        snprintf(buf, sizeof(buf), "  尝试: |cFFFFFFFF%u|r  —  完成: |cFF00FF00%u|r  —  失败: |cFFFF0000%u|r",
            st.TotalRuns, st.CompletedRuns, st.FailedRuns);
        chat.SendSysMessage(buf);

        if (st.TotalRuns > 0)
        {
            float winRate = st.CompletedRuns * 100.0f / st.TotalRuns;
            snprintf(buf, sizeof(buf), "  胜率: |cFFFFD700%.1f%%|r", winRate);
            chat.SendSysMessage(buf);
        }

        chat.SendSysMessage(" ");

        snprintf(buf, sizeof(buf), "  击杀怪物总数:  |cFFFFFFFF%u|r  —  击杀BOSS总数: |cFFFFFFFF%u|r",
            st.TotalMobsKilled, st.TotalBossesKilled);
        chat.SendSysMessage(buf);

        snprintf(buf, sizeof(buf), "  死亡总数: |cFFFF0000%u|r", st.TotalDeaths);
        chat.SendSysMessage(buf);

        if (st.TotalDeaths > 0 && st.TotalMobsKilled > 0)
        {
            float kd = static_cast<float>(st.TotalMobsKilled + st.TotalBossesKilled) / st.TotalDeaths;
            snprintf(buf, sizeof(buf), "  击杀死亡比: |cFFFFD700%.1f|r", kd);
            chat.SendSysMessage(buf);
        }

        if (st.FastestClear > 0)
        {
            char timeBuf[64];
            FormatTime(st.FastestClear, timeBuf, sizeof(timeBuf));
            snprintf(buf, sizeof(buf), "  最快通关时长: |cFF00FFFF%s|r", timeBuf);
            chat.SendSysMessage(buf);
        }

        chat.SendSysMessage("|cFFFFD700══════════════════════════════════════════|r");

        AddGossipItemFor(player, GOSSIP_ICON_TABARD, "|cFFFFD700查看英雄榜|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_BOARD_MENU);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_STATS_MENU);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowRoguelikeStats(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        RoguelikePlayerStats st = sRoguelikeMgr->GetRoguelikePlayerStats(player->GetGUID());
        auto chat = ChatHandler(player->GetSession());
        char buf[256];

        chat.SendSysMessage("|cFF00FFFF═══════════ 闯关挑战统计 ═══════════|r");

        snprintf(buf, sizeof(buf), "  总次数: |cFFFFFFFF%u|r", st.TotalRuns);
        chat.SendSysMessage(buf);

        snprintf(buf, sizeof(buf), "  已解锁最高难度: |cFFFFD700%u|r  —  最高抵达层级: |cFFFFD700%u|r",
            st.HighestTier, st.MostFloorsCleared);
        chat.SendSysMessage(buf);

        snprintf(buf, sizeof(buf), "  累计通关总层数: |cFFFFFFFF%u|r", st.TotalFloorsCleared);
        chat.SendSysMessage(buf);

        chat.SendSysMessage(" ");

        snprintf(buf, sizeof(buf), "  累计击杀小怪: |cFFFFFFFF%u|r  —  累计击杀BOSS: |cFFFFFFFF%u|r",
            st.TotalMobsKilled, st.TotalBossesKilled);
        chat.SendSysMessage(buf);

        snprintf(buf, sizeof(buf), "  死亡: |cFFFF0000%u|r", st.TotalDeaths);
        chat.SendSysMessage(buf);

        if (st.TotalDeaths > 0 && (st.TotalMobsKilled + st.TotalBossesKilled) > 0)
        {
            float kd = static_cast<float>(st.TotalMobsKilled + st.TotalBossesKilled) / st.TotalDeaths;
            snprintf(buf, sizeof(buf), "  Kill/Death Ratio: |cFFFFD700%.1f|r", kd);
            chat.SendSysMessage(buf);
        }

        if (st.TotalRuns > 0)
        {
            float avgFloors = static_cast<float>(st.TotalFloorsCleared) / st.TotalRuns;
            snprintf(buf, sizeof(buf), "  击杀死亡比: |cFF00FFFF%.1f|r", avgFloors);
            chat.SendSysMessage(buf);
        }

        if (st.LongestRunTime > 0)
        {
            char timeBuf[64];
            FormatTime(st.LongestRunTime, timeBuf, sizeof(timeBuf));
            snprintf(buf, sizeof(buf), "  最长用时: |cFF00FFFF%s|r", timeBuf);
            chat.SendSysMessage(buf);
        }

        chat.SendSysMessage("|cFF00FFFF══════════════════════════════════════════|r");

        AddGossipItemFor(player, GOSSIP_ICON_TABARD, "|cFFFFD700查看英雄榜|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_BOARD_MENU);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_STATS_MENU);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    // ---- Leaderboard Hub ----

    void ShowBoardMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        AddGossipItemFor(player, GOSSIP_ICON_TABARD,
            "|cFFFFD700副本挑战 — 最快通关记录|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_BOARD_NORMAL);
        if (sDMConfig->IsRoguelikeEnabled())
        {
            AddGossipItemFor(player, GOSSIP_ICON_TABARD,
                "|cFF00FFFF闯关模式 — 最高解锁难度|r",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_BOARD_RL_TIER);
            AddGossipItemFor(player, GOSSIP_ICON_TABARD,
                "|cFF00FFFF闯关模式 — 最高层级|r",
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_BOARD_RL_FLOORS);
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_STATS_MENU);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowNormalLeaderboard(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();

        auto entries = sDungeonMasterMgr->GetOverallLeaderboard(10);
        auto chat = ChatHandler(player->GetSession());

        chat.SendSysMessage("|cFFFFD700═══════ 副本挑战 — 最快通关榜单 ═══════|r");

        if (entries.empty())
        {
            chat.SendSysMessage("  |cFF808080暂无记录.|r");
        }
        else
        {
            uint32 rank = 0;
            uint32 myGuid = player->GetGUID().GetCounter();
            for (const auto& e : entries)
            {
                ++rank;
                char timeBuf[64];
                FormatTime(e.ClearTime, timeBuf, sizeof(timeBuf));

                const DifficultyTier* diff = sDMConfig->GetDifficulty(e.DifficultyId);
                const DungeonInfo* dg = sDMConfig->GetDungeon(e.MapId);

                bool isMe = (e.Guid == myGuid);
                char buf[384];
                snprintf(buf, sizeof(buf),
                    "  %s#%u%s |cFFFFFFFF%s|r — |cFF00FFFF%s|r — %s (%s) %uP%s%s",
                    isMe ? "|cFF00FF00" : "|cFFFFD700",
                    rank,
                    isMe ? "|r" : "|r",
                    e.CharName.c_str(),
                    timeBuf,
                    dg ? dg->Name.c_str() : "?",
                    diff ? diff->Name.c_str() : "?",
                    e.PartySize,
                    e.Scaled ? " |cFF00FF00[平衡]|r" : "",
                    isMe ? " |cFF00FF00<< 你|r" : "");
                chat.SendSysMessage(buf);
            }
        }

        chat.SendSysMessage("|cFFFFD700════════════════════════════════════════════════|r");
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_BOARD_MENU);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowRoguelikeLeaderboard(Player* player, Creature* creature, bool sortByFloors)
    {
        player->PlayerTalkClass->ClearMenus();

        auto entries = sRoguelikeMgr->GetRoguelikeLeaderboard(10, sortByFloors);
        auto chat = ChatHandler(player->GetSession());

        if (sortByFloors)
            chat.SendSysMessage("|cFF00FFFF═══════ 闯关模式 — 最高层级 ═══════|r");
        else
            chat.SendSysMessage("|cFF00FFFF═══════ 闯关模式 — 最高解锁难度 ═══════|r");

        if (entries.empty())
            chat.SendSysMessage("  |cFF808080暂无闯关模式记录.|r");
        else
        {
            uint32 rank = 0;
            uint32 myGuid = player->GetGUID().GetCounter();
            for (const auto& e : entries)
            {
                ++rank;
                char timeBuf[64];
                FormatTime(e.RunDuration, timeBuf, sizeof(timeBuf));

                bool isMe = (e.Guid == myGuid);
                char buf[384];
                snprintf(buf, sizeof(buf),
                    "  %s#%u%s |cFFFFFFFF%s|r — 档位 |cFFFFD700%u|r — |cFF00FF00%u|r 层级%s — |cFF00FFFF%s|r — %u 击杀 — %uP%s",
                    isMe ? "|cFF00FF00" : "|cFFFFD700",
                    rank,
                    isMe ? "|r" : "|r",
                    e.CharName.c_str(),
                    e.TierReached,
                    e.DungeonsCleared,
                    e.DungeonsCleared != 1 ? "s" : "",
                    timeBuf,
                    e.TotalKills,
                    e.PartySize,
                    isMe ? " |cFF00FF00<< 你|r" : "");
                chat.SendSysMessage(buf);
            }
        }

        chat.SendSysMessage("|cFF00FFFF══════════════════════════════════════════════|r");
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_BOARD_MENU);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    // ---- Roguelike Menus ----

    void ShowRoguelikeDifficultyMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        uint8 lvl = player->GetLevel();

        ChatHandler(player->GetSession()).SendSysMessage(
            "|cFF00FFFF========== 闯关模式 ==========|r");
        ChatHandler(player->GetSession()).SendSysMessage(
            "|cFFFFFFFF连续通关副本，每通关一次提升一档难度|r");
        ChatHandler(player->GetSession()).SendSysMessage(
            "|cFFFFFFFF敌人强度会不断提升，但你能获得强力增益效果.|r");
        ChatHandler(player->GetSession()).SendSysMessage(
            "|cFFFF0000一旦团灭，闯关直接结束!|r");
        ChatHandler(player->GetSession()).SendSysMessage(
            "|cFF00FFFF========================================|r");

        for (const auto& d : sDMConfig->GetDifficulties())
        {
            char buf[256];
            if (!d.IsValidForLevel(lvl))
                snprintf(buf, sizeof(buf), "|cFF808080%s|r (等级 %u-%u) - |cFFFF0000要求 %u+|r",
                    d.Name.c_str(), d.MinLevel, d.MaxLevel, d.MinLevel);
            else
                snprintf(buf, sizeof(buf), "|cFF00FFFF%s|r (等级 %u-%u)",
                    d.Name.c_str(), d.MinLevel, d.MaxLevel);

            AddGossipItemFor(player,
                d.IsValidForLevel(lvl) ? GOSSIP_ICON_BATTLE : GOSSIP_ICON_CHAT,
                buf, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_DIFF_BASE + d.Id);
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowRoguelikeScalingMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();

        uint8 partyLevel = sDungeonMasterMgr->ComputeEffectiveLevel(player);

        char buf1[256], buf2[256];
        snprintf(buf1, sizeof(buf1),
            "|cFF00FF00动态平衡队伍等级|r (Lv %u)", partyLevel);
        snprintf(buf2, sizeof(buf2),
            "|cFFFFD700启用副本难度|r — 原版等级区间");

        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, buf1,
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ROGUELIKE_SCALE_PARTY);
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, buf2,
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ROGUELIKE_SCALE_TIER);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void ShowRoguelikeThemeMenu(Player* player, Creature* creature)
    {
        player->PlayerTalkClass->ClearMenus();
        for (const auto& t : sDMConfig->GetThemes())
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, t.Name,
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ROGUELIKE_THEME + t.Id);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cFFFF0000<< 返回|r",
            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CANCEL);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void StartRoguelike(Player* player, Creature* /*creature*/)
    {
        player->PlayerTalkClass->SendCloseGossip();

        PlayerDMSelection sel;
        { std::lock_guard<std::mutex> lk(sSelMutex);
          auto it = sSelections.find(player->GetGUID());
          if (it == sSelections.end()) {
              ChatHandler(player->GetSession()).SendSysMessage(
                  "|cFFFF0000[闯关挑战]|r 选择已失效，请重新操作.");
              return; }
          sel = it->second;
          sSelections.erase(it); }

        const DifficultyTier* diff = sDMConfig->GetDifficulty(sel.DifficultyId);
        if (!diff || !diff->IsValidForLevel(player->GetLevel()))
        {
            ChatHandler(player->GetSession()).SendSysMessage(
                "|cFFFF0000[闯关挑战]|r 未达到所需等级!");
            return;
        }

        uint32 runId = 0; // unused, StartRun returns bool
        if (!sRoguelikeMgr->StartRun(player, sel.DifficultyId,
            sel.ThemeId, sel.ScaleToParty))
        {
            ChatHandler(player->GetSession()).SendSysMessage(
                "|cFFFF0000[闯关挑战]|r 无法开启闯关挑战模式!");
            return;
        }

        ChatHandler(player->GetSession()).SendSysMessage(
            "|cFF00FFFF[闯关挑战]|r 闯关已开启！通关副本挑战难度层级，祝你好运!");
    }

    // ---- Launch ----
    void StartChallenge(Player* player, Creature* /*creature*/)
    {
        player->PlayerTalkClass->SendCloseGossip();

        PlayerDMSelection sel;
        { std::lock_guard<std::mutex> lk(sSelMutex);
          auto it = sSelections.find(player->GetGUID());
          if (it == sSelections.end()) {
              ChatHandler(player->GetSession()).SendSysMessage("|cFFFF0000[未达到所需等级]|r 选择已失效，请再试一次.");
              return; }
          sel = it->second;
          sSelections.erase(it); }

        const DifficultyTier* diff = sDMConfig->GetDifficulty(sel.DifficultyId);
        if (!diff || !diff->IsValidForLevel(player->GetLevel()))
        {
            ChatHandler(player->GetSession()).SendSysMessage("|cFFFF0000[副本大师]|r 未达到所需等级t!");
            return;
        }

        // Resolve random dungeon
        uint32 mapId = sel.MapId;
        if (mapId == 0)
        {
            auto dgs = sDMConfig->GetDungeonsForLevel(diff->MinLevel, diff->MaxLevel);
            if (dgs.empty()) {
                ChatHandler(player->GetSession()).SendSysMessage("|cFFFF0000[副本大师]|r 没有激活的副本!");
                return; }
            static thread_local std::mt19937 rng{ std::random_device{}() };
            mapId = dgs[std::uniform_int_distribution<size_t>(0, dgs.size()-1)(rng)]->MapId;
        }

        Session* s = sDungeonMasterMgr->CreateSession(player, sel.DifficultyId, sel.ThemeId, mapId, sel.ScaleToParty);
        if (!s) {
            ChatHandler(player->GetSession()).SendSysMessage("|cFFFF0000[副本大师]|r 创建副本会话失败!");
            return; }

        if (!sDungeonMasterMgr->StartDungeon(s)) {
            ChatHandler(player->GetSession()).SendSysMessage("|cFFFF0000[副本大师]|r 副本初始化失败!");
            sDungeonMasterMgr->AbandonSession(s->SessionId); return; }

        if (!sDungeonMasterMgr->TeleportPartyIn(s)) {
            ChatHandler(player->GetSession()).SendSysMessage("|cFFFF0000[副本大师]|r 副本传送失败!");
            sDungeonMasterMgr->AbandonSession(s->SessionId); return; }

        if (sDMConfig->ShouldAnnounceCompletion())
        {
            const Theme* theme = sDMConfig->GetTheme(sel.ThemeId);
            const DungeonInfo* dg = sDMConfig->GetDungeon(mapId);
            char buf[256];
            snprintf(buf, sizeof(buf),
                "|cFF00FF00[副本大师]|r |cFFFFFFFF%s|r 开始了一个 |cFFFFD700%s|r |cFF00FFFF%s|r 副本挑战!",
                player->GetName().c_str(), diff->Name.c_str(),
                theme ? theme->Name.c_str() : "随机");

            char detail[256];
            snprintf(detail, sizeof(detail),
                "|cFFFFD700[副本大师]|r 难度: |cFF00FF00%s|r  类型: |cFF00FF00%s|r  副本: |cFF00FF00%s|r  平衡: |cFF00FF00%s|r",
                diff->Name.c_str(),
                theme ? theme->Name.c_str() : "随机",
                dg ? dg->Name.c_str() : "随机",
                sel.ScaleToParty ? "队伍等级" : "副本难度");

            // Broadcast to ALL party members
            for (const auto& pd : s->Players)
                if (Player* p = ObjectAccessor::FindPlayer(pd.PlayerGuid))
                {
                    ChatHandler(p->GetSession()).SendSysMessage(buf);
                    ChatHandler(p->GetSession()).SendSysMessage(detail);
                }
        }
    }
};

void AddSC_npc_dungeon_master()
{
    new npc_dungeon_master();
}
