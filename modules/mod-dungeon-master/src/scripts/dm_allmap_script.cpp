/*
 * mod-dungeon-master — dm_allmap_script.cpp
 * Triggers dungeon population when a player enters the instance map.
 */

#include "ScriptMgr.h"
#include "Map.h"
#include "Player.h"
#include "DungeonMasterMgr.h"
#include "DMConfig.h"
#include "Chat.h"
#include "Log.h"
#include <cstdio>

using namespace DungeonMaster;

class dm_allmap_script : public AllMapScript
{
public:
    dm_allmap_script() : AllMapScript("dm_allmap_script") {}

    void OnPlayerEnterAll(Map* map, Player* player) override
    {
        if (!sDMConfig->IsEnabled() || !map || !player)
            return;

        // Only care about dungeon maps
        if (!map->IsDungeon())
            return;

        Session* session = sDungeonMasterMgr->GetSessionByPlayer(player->GetGUID());
        if (!session)
        {
            LOG_DEBUG("module", "DungeonMaster: OnPlayerEnterAll — {} entered map {} but has no session",
                player->GetName(), map->GetId());
            return;
        }

        LOG_INFO("module", "DungeonMaster: OnPlayerEnterAll — {} entered map {} (session {} state {} mapId {} mobs {} bosses {})",
            player->GetName(), map->GetId(), session->SessionId,
            static_cast<int>(session->State), session->MapId,
            session->TotalMobs, session->TotalBosses);

        if (session->State != SessionState::InProgress)
            return;

        if (map->GetId() != session->MapId)
            return;

        // Only populate once — guard against duplicate triggers.
        // The Update tick also triggers populate as a reliable fallback.
        if (session->TotalMobs > 0 || session->TotalBosses > 0)
            return;

        InstanceMap* instance = map->ToInstanceMap();
        if (!instance)
            return;

        session->InstanceId = instance->GetInstanceId();

        ChatHandler(player->GetSession()).SendSysMessage(
            "|cFF00FF00[副本大师]|r 正在筹备挑战...");

        sDungeonMasterMgr->PopulateDungeon(session, instance);

        LOG_INFO("module", "DungeonMaster: Session {} — populated via OnPlayerEnterAll (player {}, map {}, mobs {}, bosses {})",
            session->SessionId, player->GetName(), map->GetId(),
            session->TotalMobs, session->TotalBosses);

        char buf[256];
        snprintf(buf, sizeof(buf),
            "|cFF00FF00[副本大师]|r |cFFFFFFFF%u|r 怪物和 |cFFFFFFFF%u|r boss(首领) 已生成. "
            "怪物等级: |cFFFFFFFF%u-%u|r. 祝你好运!",
            session->TotalMobs, session->TotalBosses,
            session->LevelBandMin, session->LevelBandMax);
        ChatHandler(player->GetSession()).SendSysMessage(buf);
    }
};

void AddSC_dm_allmap_script()
{
    new dm_allmap_script();
}
