/*
 * Credits: silviu20092
 */

#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "Creature.h"
#include "Player.h"
#include "StringConvert.h"
#include "item_reforge.h"

class npc_reforger : public CreatureScript
{
private:
    std::unordered_map<uint32, ObjectGuid> itemMap;

    bool CloseGossip(Player* player, bool retVal = true)
    {
        CloseGossipMenuFor(player);
        return retVal;
    }

    bool AddEquipmentSlotMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);

        const std::vector<uint32>& reforgeableStats = sItemReforge->GetReforgeableStats();
        std::ostringstream oss;
        oss << "可重铸属性: ";
        bool hasStats = false;
        for (uint32 i = 0; i < reforgeableStats.size(); i++)
        {
            hasStats = true;
            oss << sItemReforge->StatTypeToString(reforgeableStats[i]);
            if (i < reforgeableStats.size() - 1)
                oss << ", ";
        }
        if (!hasStats)
            oss << ItemReforge::TextRed("无");

        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, oss.str(), GOSSIP_SENDER_MAIN + 1, EQUIPMENT_SLOT_END);

        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
        {
            Item* item = sItemReforge->GetItemInSlot(player, slot);
            std::ostringstream oss;
            oss << sItemReforge->GetSlotIcon(slot);
            oss << sItemReforge->GetSlotName(slot);

            if (item == nullptr)
                oss << " [" << ItemReforge::TextRed("无装备") << "]";
            else
            {
                if (sItemReforge->IsAlreadyReforged(item))
                    oss << " [" << ItemReforge::TextRed("已经重铸") << "]";
                else if (!sItemReforge->IsReforgeable(player, item))
                    oss << " [" << ItemReforge::TextRed("不可重铸") << "]";
                else
                    oss << " [" << ItemReforge::TextGreen("可重铸") << "]";
            }

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, oss.str(), GOSSIP_SENDER_MAIN + 1, slot);
        }

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool CanAdvanceWithReforging(Player* player, const Item* item) const
    {
        if (item == nullptr)
        {
            ItemReforge::SendMessage(player, "该位置没有装备");
            return false;
        }
        else if (sItemReforge->IsAlreadyReforged(item))
        {
            ItemReforge::SendMessage(player, "这件装备已经重铸.");
            return false;
        }
        else if (!sItemReforge->IsReforgeable(player, item))
        {
            ItemReforge::SendMessage(player, "这件装备不能重铸.");
            return false;
        }

        return true;
    }

    bool AddReforgingMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);

        ObjectGuid itemGuid = itemMap[player->GetGUID().GetCounter()];
        Item* item = player->GetItemByGuid(itemGuid);
        if (!CanAdvanceWithReforging(player, item))
            return CloseGossip(player, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, ItemReforge::ItemLinkForUI(item, player), GOSSIP_SENDER_MAIN + 2, GOSSIP_ACTION_INFO_DEF + 100);

        std::vector<_ItemStat> itemStats = sItemReforge->LoadItemStatInfo(item, true);
        for (const _ItemStat& stat : itemStats)
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "重铸 " + sItemReforge->StatTypeToString(stat.ItemStatType), GOSSIP_SENDER_MAIN + 2, stat.ItemStatType);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool AddReforgingStatsMenu(Player* player, Creature* creature, uint32 stat)
    {
        ClearGossipMenuFor(player);

        ObjectGuid itemGuid = itemMap[player->GetGUID().GetCounter()];
        Item* item = player->GetItemByGuid(itemGuid);
        if (!CanAdvanceWithReforging(player, item))
            return CloseGossip(player, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, ItemReforge::ItemLinkForUI(item, player), GOSSIP_SENDER_MAIN + 2, stat);

        const std::vector<uint32>& reforgeableStats = sItemReforge->GetReforgeableStats();
        std::vector<_ItemStat> itemStats = sItemReforge->LoadItemStatInfo(item);
        const _ItemStat* toReforgeStat = sItemReforge->FindItemStat(itemStats, stat);
        if (toReforgeStat == nullptr)
            return CloseGossip(player, false);

        uint32 taken = sItemReforge->CalculateReforgePct(toReforgeStat->ItemStatValue);
        uint32 newVal = toReforgeStat->ItemStatValue - taken;
        std::ostringstream oss;
        oss << "重铸有一定概率成功，每次都会扣除" << ItemReforge::TextRed("50金币和2件相同装备") << ".唯一装备会扣除10虚灵币和50金币.";
        oss << "重铸成功会扣减" << ItemReforge::TextRed(Acore::ToString((uint32)sItemReforge->GetPercentage())) << "%" << sItemReforge->StatTypeToString(stat) << "属性";
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, oss.str(), GOSSIP_SENDER_MAIN + 2, stat);
        oss.str("");

        oss << sItemReforge->StatTypeToString(stat) << " 重铸后的值: ";
        oss << ItemReforge::TextRed(Acore::ToString(newVal)) << " (-" << Acore::ToString(taken) << ")";
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, oss.str(), GOSSIP_SENDER_MAIN + 2, stat);
        oss.str("");

        for (const uint32& rstat : reforgeableStats)
        {
            if (sItemReforge->FindItemStat(itemStats, rstat) != nullptr)
                continue;

            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, ItemReforge::TextGreen("+" + Acore::ToString(taken) + " " + sItemReforge->StatTypeToString(rstat)), GOSSIP_SENDER_MAIN + 10 + stat, rstat, "你确定要重铸这件装备吗(每次消耗50G+2件同样装备)?", 0, false);
        }
        oss.str("");

        oss << "注意事项:交易、邮件、放银行、挂拍卖都会让装备重铸属性丢失；重铸有33%的成功率；无论是否成功都会扣钱和所需装备。";
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, oss.str(), GOSSIP_SENDER_MAIN + 2, stat);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回", GOSSIP_SENDER_MAIN + 2, GOSSIP_ACTION_INFO_DEF + 100);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool AddRemoveReforgeMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);

        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
        {
            Item* item = sItemReforge->GetItemInSlot(player, slot);
            std::ostringstream oss;
            oss << sItemReforge->GetSlotIcon(slot);
            oss << sItemReforge->GetSlotName(slot);

            if (item == nullptr)
                oss << " [" << ItemReforge::TextRed("无装备") << "]";
            else
            {
                if (sItemReforge->IsAlreadyReforged(item))
                    oss << " [" << ItemReforge::TextGreen("已重铸") << "]";
                else
                    oss << " [" << ItemReforge::TextRed("未重铸") << "]";
            }

            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, oss.str(), GOSSIP_SENDER_MAIN + 3, slot);
        }

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool AddRemoveReforgeStatsMenu(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);

        ObjectGuid itemGuid = itemMap[player->GetGUID().GetCounter()];
        Item* item = player->GetItemByGuid(itemGuid);
        if (!sItemReforge->CanRemoveReforge(item))
            return CloseGossip(player, false);

        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, ItemReforge::ItemLinkForUI(item, player), GOSSIP_SENDER_MAIN + 4, GOSSIP_ACTION_INFO_DEF);

        const ItemReforge::ReforgingData* reforging = sItemReforge->GetReforgingData(item);
        if (reforging == nullptr)
            return CloseGossip(player, false);

        std::vector<_ItemStat> itemStats = sItemReforge->LoadItemStatInfo(item);
        const _ItemStat* decreasedStat = sItemReforge->FindItemStat(itemStats, reforging->stat_decrease);
        if (decreasedStat == nullptr)
            return CloseGossip(player, false);

        std::ostringstream oss;
        oss << "将会恢复 " << sItemReforge->StatTypeToString(decreasedStat->ItemStatType) << " 到 " << ItemReforge::TextGreen(Acore::ToString(decreasedStat->ItemStatValue));
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, oss.str(), GOSSIP_SENDER_MAIN + 4, GOSSIP_ACTION_INFO_DEF);

        oss.str("");
        oss << ItemReforge::TextRed("-" + Acore::ToString(reforging->stat_value) + " " + sItemReforge->StatTypeToString(reforging->stat_increase));
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, oss.str(), GOSSIP_SENDER_MAIN + 4, GOSSIP_ACTION_INFO_DEF);

        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, ItemReforge::TextRed("[恢复]"), GOSSIP_SENDER_MAIN + 4, GOSSIP_ACTION_INFO_DEF + 1, "你确定吗?", 0, false);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }
public:
    npc_reforger() : CreatureScript("npc_reforger") {}

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!sItemReforge->GetEnabled())
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cffb50505未激活|r", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        else
        {
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "选择要重铸的装备槽位", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "从物品中移除重铸", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "不要了", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (!sItemReforge->GetEnabled())
            return CloseGossip(player);

        if (sender == GOSSIP_SENDER_MAIN)
        {
            if (action == GOSSIP_ACTION_INFO_DEF)
            {
                ClearGossipMenuFor(player);
                return OnGossipHello(player, creature);
            }
            else if (action == GOSSIP_ACTION_INFO_DEF + 1)
                return AddEquipmentSlotMenu(player, creature);
            else if (action == GOSSIP_ACTION_INFO_DEF + 2)
                return CloseGossip(player);
            else if (action == GOSSIP_ACTION_INFO_DEF + 3)
                return AddRemoveReforgeMenu(player, creature);
        }
        else if (sender == GOSSIP_SENDER_MAIN + 1)
        {
            uint8 slot = (uint8)action;
            if (slot == EQUIPMENT_SLOT_END)
                return AddEquipmentSlotMenu(player, creature);

            Item* item = sItemReforge->GetItemInSlot(player, slot);
            if (!CanAdvanceWithReforging(player, item))
                return AddEquipmentSlotMenu(player, creature);
            else
            {
                itemMap[player->GetGUID().GetCounter()] = item->GetGUID();
                return AddReforgingMenu(player, creature);
            }
        }
        else if (sender == GOSSIP_SENDER_MAIN + 2)
        {
            if (action == GOSSIP_ACTION_INFO_DEF + 100)
                return AddReforgingMenu(player, creature);
            else
                return AddReforgingStatsMenu(player, creature, action);
        }
        else if (sender == GOSSIP_SENDER_MAIN + 3)
        {
            uint8 slot = (uint8)action;
            Item* item = sItemReforge->GetItemInSlot(player, slot);
            if (!sItemReforge->CanRemoveReforge(item))
                return AddRemoveReforgeMenu(player, creature);
            else
            {
                itemMap[player->GetGUID().GetCounter()] = item->GetGUID();
                return AddRemoveReforgeStatsMenu(player, creature);
            }
        }
        else if (sender == GOSSIP_SENDER_MAIN + 4)
        {
            if (action == GOSSIP_ACTION_INFO_DEF)
                return AddRemoveReforgeStatsMenu(player, creature);
            else
            {
                if (sItemReforge->RemoveReforge(player, itemMap[player->GetGUID().GetCounter()]))
                    sItemReforge->VisualFeedback(player);

                return CloseGossip(player);
            }
        }
        else if (sender >= GOSSIP_SENDER_MAIN + 10)
        {
            uint32 decreaseStat = sender - (GOSSIP_SENDER_MAIN + 10);
            uint32 increaseStat = action;
            // 检查玩家是否有足够的金币和虚灵币
            if (player->GetMoney() < 500000) // 50金 = 500000铜
            {
                ItemReforge::SendMessage(player, "你没有足够的金币，需要50金币.");
                return CloseGossip(player);
            }

            // 检查虚灵币数量
            //if (!player->HasItemCount(38186, 10))
            //{
            //    ItemReforge::SendMessage(player, "你没有足够的虚灵币，需要10个.");
            //    return CloseGossip(player);
            //}
            //uint8 slot = (uint8)action;
            //Item* item = sItemReforge->GetItemInSlot(player, slot);
                // 获取重铸装备的 ID
            //uint32 itemID = item->GetTemplate()->ItemId;
            ObjectGuid itemGuid = itemMap[player->GetGUID().GetCounter()];
            Item* item = player->GetItemByGuid(itemGuid);
            uint32 itemID = item->GetTemplate()->ItemId;
            // 检查玩家是否拥有两件与所选重铸装备相同 ID 的装备
            //if (item->GetTemplate()->HasFlag(ITEM_FLAG_UNIQUE_EQUIPPABLE))
            if (item->GetTemplate()->MaxCount == 1)
            {
                //检查虚灵币数量
                if (!player->HasItemCount(38186, 10))
                {
                    ItemReforge::SendMessage(player, "你没有足够的虚灵币，需要10个.");
                    return CloseGossip(player);
                }
                if (player->GetMoney() < 500000)
                {
                    ItemReforge::SendMessage(player, "你没有足够的金币，需要50G.");
                    return CloseGossip(player);
                }
            }
            else if (player->GetItemCount(itemID, false, item) < 2)
            {
                ItemReforge::SendMessage(player, "你还需要与当前重铸装备相同的2件装备，否则无法进行重铸.");
                return CloseGossip(player);
            }

            // 再次检查并扣除费用
            if (item->GetTemplate()->MaxCount != 1)//非唯一装备
            { 
                if (player->GetMoney() >= 500000 && player->GetItemCount(itemID, false, item) >= 2)
                {
                    // 扣除费用
                    player->ModifyMoney(-500000);
                    player->DestroyItemCount(itemID, 2u, true);
                    //player->DestroyItemCount(38186, 10, true);

                    // 33%概率成功
                    if (roll_chance_f(25.0f))
                    {
                        if (!sItemReforge->Reforge(player, itemMap[player->GetGUID().GetCounter()], decreaseStat, increaseStat))
                        {
                            ItemReforge::SendMessage(player, "重铸失败，但费用已扣除（50金币和2件相同装备）.");
                        }
                        else
                        {
                            sItemReforge->VisualFeedback(player);
                            ItemReforge::SendMessage(player, "恭喜您，重铸成功！");
                        }
                    }
                    else
                    {
                        ItemReforge::SendMessage(player, "重铸失败，但费用已扣除（50金币和2件相同装备）.");
                    }
                }
                else
                {
                    ItemReforge::SendMessage(player, "重铸条件不满足，无法重铸.");
                }
            }
            else
            {
                if (player->GetMoney() >= 500000 && player->HasItemCount(38186, 10))
                {
                    // 扣除费用
                    player->ModifyMoney(-500000);
                    player->DestroyItemCount(38186, 10, true);

                    // 20%概率成功
                    if (roll_chance_f(20.0f))
                    {
                        if (!sItemReforge->Reforge(player, itemMap[player->GetGUID().GetCounter()], decreaseStat, increaseStat))
                        {
                            ItemReforge::SendMessage(player, "重铸失败，但费用已扣除（50金币和10虚灵币）.");
                        }
                        else
                        {
                            sItemReforge->VisualFeedback(player);
                            ItemReforge::SendMessage(player, "恭喜您，重铸成功！");
                        }
                    }
                    else
                    {
                        ItemReforge::SendMessage(player, "重铸失败，但费用已扣除（50金币和10虚灵币）.");
                    }
                }
                else
                {
                    ItemReforge::SendMessage(player, "重铸条件不满足，无法重铸.");
                }
            }
        }
        return CloseGossip(player, false);
    }
};

void AddSC_npc_reforger()
{
    new npc_reforger();
}
