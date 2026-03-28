/*
Credits
Script reworked by Micrah/Milestorme and Poszer (Poszer is the Best)
Module Created by Micrah/Milestorme
Original Script from AshmaneCore https://github.com/conan513 Single Player Project
*/

#include "Configuration/Config.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"

class spp_dynamic_xp_rate : public PlayerScript
{
public:
    spp_dynamic_xp_rate() : PlayerScript("spp_dynamic_xp_rate") { };

    void OnPlayerLogin(Player* player) override
    {
        if (sConfigMgr->GetOption<bool>("Dynamic.XP.Rate.Announce", true))
        {
            ChatHandler(player->GetSession()).SendSysMessage("服务器已开启 |cff4CFF00职业动态经验 |r机制.");
        }
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        if (sConfigMgr->GetOption<bool>("Dynamic.XP.Rate", true))
        {
            uint32 base_rate = sConfigMgr->GetOption<uint32>("Dynamic.XP.base_rate", 1); // 默认基础倍率为1

            // 基础倍率设置，按等级区间不同设定
            switch (player->GetLevel() / 10)
            {
            case 0: case 1: // Level 0-19
                base_rate = base_rate + 1;
                break;
            case 2: case 3: case 4: case 5: // Level 20-59
                base_rate = base_rate + 2;
                break;
            case 6: case 7: // Level 60-79
                base_rate = base_rate + 3;
                break;
            default:
                return; // 超出等级范围，不调整经验值
            }

            // 计算基础经验倍率
            amount *= base_rate;

            // 根据职业调整经验倍率
            if (player->getClass() == CLASS_WARRIOR || player->getClass() == CLASS_SHAMAN || player->getClass() == CLASS_PRIEST)
            {
                // 战士、萨满、牧师加成 2
                amount = amount * base_rate + (sConfigMgr->GetOption<uint32>("Dynamic.XP.zhansamu",(100 * base_rate)) * base_rate);
            }
            else if (player->getClass() == CLASS_PALADIN || player->getClass() == CLASS_DRUID)
            {
                // 圣骑士、德鲁伊加成 1
                amount = amount * base_rate + (sConfigMgr->GetOption<uint32>("Dynamic.XP.qishide", (70 * base_rate)) * base_rate);
            }
        }
    }
};


void AddSC_dynamic_xp_rate()
{
    new spp_dynamic_xp_rate();
}
