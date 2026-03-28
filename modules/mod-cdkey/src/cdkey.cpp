/*
数据库操作：
1=给予物品
2=给予金币
3=更改名称
4=更改阵营
5=更改种族
*/

#include "Config.h"
#include "Player.h"
#include "Chat.h"
#include "Creature.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Define.h"
#include "GossipDef.h"
#include "ScriptedGossip.h"
#include "ScriptedCreature.h"
#include "CreatureScript.h"

class Exchange_code : public CreatureScript
{
public:
    Exchange_code() : CreatureScript("Exchange_code") {}

    bool failedcode;

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->IsInCombat())
            return false;

        if (!sConfigMgr->GetOption<bool>("CdkeyEnable", 0))
            return false;

        // 添加文字说明（不能选择的菜单项）
        std::string introText = "本功能需要在网站充值后复制买到的兑换码，在此处兑换成相应数量的虚灵币！请根据以下选项选择相应操作：";
        AddGossipItemFor(player, GOSSIP_ICON_TALK, introText, GOSSIP_SENDER_MAIN, 3, introText, 0, false);
        std::string text = "点击接受然后输入兑换码";
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我想兑换我的兑换码.", GOSSIP_SENDER_MAIN, 1, text, 0, true);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "如何获得兑换码?", GOSSIP_SENDER_MAIN, 2);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我没有兑换码.", GOSSIP_SENDER_MAIN, 3);
        // === 新增：绑定沙塔斯菜单选项（完全独立，不影响原有功能）===
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, "|cff0000ff将沙塔斯广场设为我的家.|r", GOSSIP_SENDER_MAIN, 9, "你确定要把你的家绑定在沙塔斯广场吗？", 0, false);
        // === 新增结束 ===

        if (sConfigMgr->GetOption<bool>("AllowGM", 1))
            if (player->IsGameMaster())
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "[GM]生成一个兑换码.", GOSSIP_SENDER_MAIN, 4);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        std::string info = sConfigMgr->GetOption<std::string>("WebsiteAddress", "您可以访问在线商店（网址）获取兑换码");
        uint32 huobi = sConfigMgr->GetOption<uint32>("huobi", 38186);
        uint32 percount = sConfigMgr->GetOption<uint32>("percount", 10);
        uint32 rnd1 = urand(10000, 90000);
        uint32 rnd2 = urand(10000, 90000);
        uint32 rnd3 = urand(10000, 90000);
        uint32 rnd4 = urand(10000, 90000);
        uint32 rnd5 = urand(10000, 90000);

        std::string CreatedBy = player->GetName();
        std::ostringstream randomcode;
        randomcode << rnd1 << "-" << rnd2 << "-" << rnd3 << "-" << rnd4 << "-" << rnd5;
        ChatHandler chatHandle = ChatHandler(player->GetSession());

        switch (action)
        {
        case 2:
            creature->Whisper(info.c_str(), LANG_UNIVERSAL, player);
            CloseGossipMenuFor(player);
            break;
        case 3:
            creature->Whisper("没有你捣什么乱！！！", LANG_UNIVERSAL, player);
            CloseGossipMenuFor(player);
            break;
        case 4:
            player->PlayerTalkClass->ClearMenus();
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "生成充值物品兑换码.", GOSSIP_SENDER_MAIN, 5);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "生成更改名字兑换码.", GOSSIP_SENDER_MAIN, 6);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "生成更改阵营兑换码.", GOSSIP_SENDER_MAIN, 7);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "生成更改种族兑换码.", GOSSIP_SENDER_MAIN, 8);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 5:
            CharacterDatabase.Execute("INSERT INTO `_兑换码` (`action`, `action_data`, `quantity`, `code`, `status`, `PlayerGUID`, `PlayerIP`, `CreatedBy`) VALUES(1, {}, {}, '{}', 0, 0, '0', '{}')", huobi, percount, randomcode.str(), CreatedBy);
            chatHandle.PSendSysMessage("已成功创建,充值物品兑换码是 %s", randomcode.str().c_str());
            CloseGossipMenuFor(player);
            break;
        case 6:
            CharacterDatabase.Execute("INSERT INTO `_兑换码` (`action`, `action_data`, `quantity`, `code`, `status`, `PlayerGUID`, `PlayerIP`, `CreatedBy`) VALUES(3, 0, 0, '{}', 0, 0, '0', '{}')", randomcode.str(), CreatedBy);
            chatHandle.PSendSysMessage("已成功创建,更改名字兑换码是 %s", randomcode.str().c_str());
            CloseGossipMenuFor(player);
            break;
        case 7:
            CharacterDatabase.Execute("INSERT INTO `_兑换码` (`action`, `action_data`, `quantity`, `code`, `status`, `PlayerGUID`, `PlayerIP`,`CreatedBy`) VALUES(4, 0, 0, '{}', 0, 0, '0', '{}')", randomcode.str(), CreatedBy);
            chatHandle.PSendSysMessage("已成功创建,更改阵营兑换码是 %s", randomcode.str().c_str());
            CloseGossipMenuFor(player);
            break;
        case 8:
            CharacterDatabase.Execute("INSERT INTO `_兑换码` (`action`, `action_data`, `quantity`, `code`, `status`, `PlayerGUID`, `PlayerIP`, `CreatedBy`) VALUES(5, 0, 0, '{}', 0, 0, '0', '{}')", randomcode.str(), CreatedBy);
            chatHandle.PSendSysMessage("已成功创建,更改种族兑换码是 %s", randomcode.str().c_str());
            CloseGossipMenuFor(player);
            break;
            // === 新增：绑定沙塔斯功能 - 第一级菜单，显示确认对话框 ===
        case 9:
        {
            WorldLocation loc(530, -1915.1218f, 5534.347f, -12.0f);// 沙塔斯广场坐标
            player->SetHomebind(loc, 3703);
            // binding
            WorldPacket data(SMSG_BINDPOINTUPDATE, 4 + 4 + 4 + 4 + 4);
            data << float(loc.GetPositionX());
            data << float(loc.GetPositionY());
            data << float(loc.GetPositionZ());
            data << uint32(loc.GetMapId());
            data << uint32(3703);
            player->SendDirectMessage(&data);

            // zone update
            data.Initialize(SMSG_PLAYERBOUND, 8 + 4);
            data << player->GetGUID();
            data << uint32(3703);
            player->SendDirectMessage(&data);

            // NPC说话提示
            creature->Whisper("你的炉石已经成功绑定到沙塔斯广场，愿你的旅程从这里开始。", LANG_UNIVERSAL, player);

            CloseGossipMenuFor(player);
        }
        break;
        // === 新增结束 ===
        }
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32, const char* code)
    {
        uint32 playerguid = player->GetGUID().GetCounter();
        std::string playerIP = player->GetSession()->GetRemoteAddress();
        std::string rewardcode = code;
        std::ostringstream messageCode;
        messageCode << "Sorry " << player->GetName() << ", 不是有效兑换码或已兑换.";

        //检查代码
        QueryResult result = CharacterDatabase.Query("SELECT id, action, action_data, quantity, status FROM _兑换码 WHERE code = '{}'", rewardcode);

        std::size_t found = rewardcode.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-");

        if (found != std::string::npos)
        {
            creature->Say("你是来捣乱的吧？请输入正确信息!", LANG_UNIVERSAL, NULL);
            return false;
        }

        if (!result)
        {
            player->PlayDirectSound(9638); // 不
            creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
            creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return false;
        }

        std::transform(rewardcode.begin(), rewardcode.end(), rewardcode.begin(), ::toupper);
        ChatHandler chatHandle = ChatHandler(player->GetSession());

        do
        {
            Field* fields = result->Fetch();
            uint32 id = fields[0].Get<uint32>();
            uint32 action = fields[1].Get<uint32>();
            uint32 action_data = fields[2].Get<uint32>();
            uint32 quantity = fields[3].Get<uint32>();
            uint32 status = fields[4].Get<uint32>();
            uint32 nospace = 0;
            int count = 1;
            uint32 noSpaceForCount = 0;
            ItemPosCountVec dest;
            InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, action_data, quantity, &noSpaceForCount);

            if (status == 1)
            {
                player->PlayDirectSound(9638); // No
                creature->Whisper(messageCode.str().c_str(), LANG_UNIVERSAL, player);
                creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
                return false;
            }

            switch (action)
            {
            case 1: /* 兑换道具 */
                if (msg != EQUIP_ERR_OK)
                    count -= noSpaceForCount;

                if (count == 0 || dest.empty())
                {
                    player->SendSystemMessage("无法创建的物品,因为物品是唯一或没有足够空间.");
                    return false;
                }

                if (count > 0 && action_data)
                {
                    player->AddItem(action_data, quantity);
                }
                break;

            case 2: /* 兑换金币 */
                player->ModifyMoney(action_data * 10000);
                chatHandle.PSendSysMessage("兑换码成功兑换 [%u G]", action_data);
                break;

            case 3: /* 名称更改 */
                player->SetAtLoginFlag(AT_LOGIN_RENAME);
                player->SendSystemMessage("兑换码: 请小退完成更改名称.");
                break;

            case 4: /* 阵营变化 */
                player->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
                player->SendSystemMessage("兑换码: 请小退完成更改阵营.");
                break;

            case 5: /* 种族变化 */
                player->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
                player->SendSystemMessage("兑换码: 请小退完成更改种族.");
                break;
            }

        } while (result->NextRow());

        CharacterDatabase.Execute("UPDATE _兑换码 SET status = 1, PlayerGUID = '{}', PlayerIP = '{}' WHERE code = '{}'", playerguid, playerIP, rewardcode);

        CloseGossipMenuFor(player);
        return true;
    }

    struct npc_Exchange_codeAI : public ScriptedAI
    {
        npc_Exchange_codeAI(Creature* creature) : ScriptedAI(creature) {}
        uint32 say_timer;
        bool canSay;

        void Reset()
        {
            say_timer = 1000;
            canSay = false;
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who && who->GetTypeId() == TYPEID_PLAYER && me->IsWithinDist(who, 5.0f))
            {
                canSay = true;
            }
            else
                canSay = false;
        }

        void UpdateAI(uint32 diff)
        {
            if (say_timer <= diff)
            {
                if (canSay)
                {
                    // 修改对话提示，告知玩家新功能（这不会影响任何功能逻辑）
                    me->Say("你有买兑换码吗?来我这里换虚灵币! 也可以直接找我绑定沙塔斯！", LANG_UNIVERSAL, NULL);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_CHICKEN);
                    say_timer = 1200000;
                }
            }
            else
                say_timer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_Exchange_codeAI(creature);
    }
};

void Addmod_cdkeyScripts()
{
    new Exchange_code();
}
