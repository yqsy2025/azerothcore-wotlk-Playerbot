#include "jianding.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Chat.h"
#include "Item.h"  // Required for Item class
#include <unordered_map>
#include "../Server/game/AI/ScriptedAI/ScriptedGossip.h"
#include "../Server/game/Spells/Spell.h"
#include <sstream>
#pragma execution_character_set("UTF-8")

// ========== 修改点1：添加全局变量和辅助函数 ==========
// 用于存储玩家选择的锁定状态（全局）
std::unordered_map<ObjectGuid, std::vector<bool>> g_playerLockSelections;
// 存储玩家选中的物品GUID
std::unordered_map<ObjectGuid, ObjectGuid> g_playerSelectedItem;

// 声明函数供Gossip使用
void ShowLockSelectionMenu(Player* player, Item* targetItem);
void OnGossipSelect(Player* player, uint32 sender, uint32 action);

class XilianRandomEnchantItem : public ItemScript
{
public:

    XilianRandomEnchantItem() : ItemScript("XilianRandomEnchantItem") {}

    // 读取配置文件（和worldserve.conf里面设置挂钩,这是默认值）
    uint32 shuaxinEntry = sConfigMgr->GetOption<uint32>("shuaxinEntry", 90003);

    // 锁定石物品ID
    const std::unordered_map<uint8, uint32> lockStoneEntries = {
        {0, 60116}, // 锁定第一条
        {1, 60117}, // 锁定第二条
        {2, 60118}, // 锁定第三条
        {3, 60119}, // 锁定第四条
        {4, 60120}  // 锁定第五条
    };

    // ========== 修改点2：将原有的OnUse改为使用Gossip ==========
    bool OnUse(Player* player, Item* item, SpellCastTargets const& targets) override
    {
        // 是否在战斗
        if (player->IsInCombat())
        {
            player->GetSession()->SendAreaTriggerMessage("您正处于交战状态");
            return false;
        }

        // 获取目标物品
        Item* tar = targets.GetItemTarget();
        if (!tar)
        {
            player->GetSession()->SendAreaTriggerMessage("目标物品不存在或已被移动");
            return false;
        }

        // 检查物品是否符合洗练条件
        uint32 Quality = tar->GetTemplate()->Quality;
        uint32 Class = tar->GetTemplate()->Class;
        if ((Quality > 6 || Quality < 2) || (Class != 2 && Class != 4))
        {
            player->GetSession()->SendAreaTriggerMessage("该物品无法进行洗练（仅限绿色及以上品质的武器和护甲）");
            return false;
        }
        // 即使没有随机属性，也允许进入洗练界面，没有属性时默认不锁任何条
        // 存储玩家选中的物品
        g_playerSelectedItem[player->GetGUID()] = tar->GetGUID();

        // 显示选择菜单
        ShowLockSelectionMenu(player, tar);
        return false;
    }

    void ExecuteReforge(Player* player, Item* targetItem, const std::vector<bool>& lockedSlots)
    {
        if (!player || !targetItem)
            return;

        if (lockedSlots.size() != 5)
        {
            std::vector<bool> defaultLock(5, false);
            ShuaxinEnchant(player, targetItem, defaultLock);
            return;
        }

        // 计算锁定数量
        uint8 lockCount = 0;
        for (bool locked : lockedSlots) {
            if (locked) lockCount++;
        }

        // 根据锁定数量确定锁定倍数
        uint32 lockMultiplier = 1;
        switch (lockCount) {
        case 1: lockMultiplier = 1; break;
        case 2: lockMultiplier = 2; break;
        case 3: lockMultiplier = 3; break;
        case 4: lockMultiplier = 4; break;
        case 5: lockMultiplier = 5; break;
        default: lockMultiplier = 1;
        }

        // 总倍数 = 基础倍数 × 锁定倍数
        uint32 requiredCount = lockMultiplier;

        std::string countStr = std::to_string(requiredCount);
        std::string targetItemLink = targetItem->GetTemplate()->Name1;
        const ItemLocale* itemLocale = sObjectMgr->GetItemLocale(targetItem->GetEntry());
        if (itemLocale && itemLocale->Name.size() > 4 && !itemLocale->Name[4].empty())
        {
            targetItemLink = itemLocale->Name[4];
        }

        // 检查是否有足够的洗练道具
        if (!player->HasItemCount(shuaxinEntry, requiredCount))
        {
            std::string message = "|CFFFF0000您的命运之轮数量不足 " + countStr + "|CFFFF0000 个！";
            if (Item* shuaxinItem = player->GetItemByEntry(shuaxinEntry))
            {
                std::string itemLink = shuaxinItem->GetTemplate()->Name1;
                message = "|CFFFF0000您的" + itemLink + "|CFFFF0000数量不足 " + countStr + "|CFFFF0000 个！";
            }
            player->GetSession()->SendAreaTriggerMessage("%s", message.c_str());
            ChatHandler(player->GetSession()).SendSysMessage(message.c_str());
            return;
        }

        // 消耗锁定石（注意：这里不使用锁定石，而是使用玩家选择的锁定状态）
        for (const auto& [slot, entry] : lockStoneEntries)
        {
            if (lockedSlots[slot])
            {
                if (!player->HasItemCount(entry, 1))
                {
                    player->GetSession()->SendAreaTriggerMessage(
                        "缺少锁定石，无法锁定该属性！");
                    return;
                }
            }
        }
        // 真正扣除锁定石
        for (const auto& [slot, entry] : lockStoneEntries)
        {
            if (lockedSlots[slot])
            {
                player->DestroyItemCount(entry, 1, true);
            }
        }

        // 执行洗练
        ShuaxinEnchant(player, targetItem, lockedSlots);

        // 消耗洗练道具
        player->DestroyItemCount(shuaxinEntry, requiredCount, true);

        // 发送成功消息
        player->GetSession()->SendAreaTriggerMessage("|CFFFF0000随机属性洗练成功！");

        std::string successMsg;
        if (Item* shuaxinItem = player->GetItemByEntry(shuaxinEntry))
        {
            std::string itemLink = shuaxinItem->GetTemplate()->Name1;
            successMsg = "洗练随机属性" + targetItemLink + "消耗" + itemLink + " x " + countStr + " 个！";
        }
        else
        {
            successMsg = "洗练随机属性" + targetItemLink + "消耗命运之轮 x " + countStr + " 个";
        }
        ChatHandler(player->GetSession()).SendSysMessage(successMsg.c_str());
    }

    void ShuaxinEnchant(Player* player, Item* item, const std::vector<bool>& lockedSlots)
    {
        uint32 Quality = item->GetTemplate()->Quality;
        uint32 Class = item->GetTemplate()->Class;
        uint32 itemLevel = item->GetTemplate()->ItemLevel;

        if ((Quality > 6 || Quality < 2) ||
            (Class != 2 && Class != 4))
        {
            return;
        }

        uint32 slotEnch[5] = { 7, 8, 9, 10, 11 };
        int slotRand[5] = { -1, -1, -1, -1, -1 };

        // 提前缓存锁定属性
        std::vector<uint32> lockedEnchants;
        for (int i = 0; i < 5; i++)
        {
            if (lockedSlots[i])
            {
                uint32 ench =
                    item->GetEnchantmentId(
                        EnchantmentSlot(slotEnch[i])
                    );


                if (ench)
                {
                    lockedEnchants.push_back(ench);
                }
            }
        }

        int lockCount = lockedEnchants.size();
        //=========================
        // 先清除未锁定槽位
        //=========================
        for (int i = 0; i < 5; ++i)
        {
            if (!lockedSlots[i])
            {
                player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), false);
                item->SetEnchantment(EnchantmentSlot(slotEnch[i]), 0, 0, 0);
            }
        }

        //=========================
        // 根据锁定数量计算额外词条
        //=========================
        int targetCount = lockCount;
        // 锁定几条，至少保留几条
        if (targetCount < 1)
            targetCount = 1;

        // 第二条概率
        double secondChance = 70;

        // 第三条概率
        double thirdChance = 40;

        // 第四条概率
        double fourthChance = 20;

        // 第五条概率
        double fifthChance = 10;
        //=========================
        // 根据锁定数量增加对应下一档概率
        //=========================
        if (lockCount == 1)
            secondChance += 10;

        if (lockCount == 2)
            thirdChance += 10;

        if (lockCount == 3)
            fourthChance += 10;

        if (lockCount == 4)
            fifthChance += 10;

        // 限制100%
        secondChance = std::min(secondChance, 100.0);
        thirdChance = std::min(thirdChance, 100.0);
        fourthChance = std::min(fourthChance, 100.0);
        fifthChance = std::min(fifthChance, 100.0);

        //=========================
        // 从当前锁定数量继续追加
        //=========================
        // 已锁1条，判断是否增加第2条
        if (targetCount < 2 && rand_chance() < secondChance)
            targetCount++;

        // 已经2条，判断第3条
        if (targetCount < 3 && rand_chance() < thirdChance)
            targetCount++;

        // 已经3条，判断第4条
        if (targetCount < 4 && rand_chance() < fourthChance)
            targetCount++;

        // 已经4条，判断第5条
        if (targetCount < 5 && rand_chance() < fifthChance)
            targetCount++;

        // 最少保留锁定数量
        if (targetCount < lockCount)
            targetCount = lockCount;

        if (targetCount > 5)
            targetCount = 5;

        // 生成未锁定属性
        int newCount = targetCount - lockCount;
        std::vector<int> newEnchants;
        int retry = 0;

        while (newCount > 0 && retry < 50)
        {
            int ench =
                getRandEnchantment(item);

            if (ench > 0)
            {
                newEnchants.push_back(ench);
                newCount--;
            }
            retry++;
        }

        // 重新组合属性
        std::vector<int> finalEnchants;
        // 加入锁定属性
        for (uint32 ench : lockedEnchants)
        {
            finalEnchants.push_back(ench);
        }

        // 加入新属性
        for (int ench : newEnchants)
        {
            finalEnchants.push_back(ench);
        }
        // 重新写入槽位
        for (int i = 0;i < 5;i++)
        {
            player->ApplyEnchantment(
                item,
                EnchantmentSlot(slotEnch[i]),
                false
            );
            item->SetEnchantment(EnchantmentSlot(slotEnch[i]),0,0,0);
        }

        for (size_t i = 0; i < finalEnchants.size() && i < 5; i++)
        {
            item->SetEnchantment(EnchantmentSlot(slotEnch[i]),finalEnchants[i],0,0);
            player->ApplyEnchantment(item,EnchantmentSlot(slotEnch[i]),true);
        }

        //=========================
        // 统计
        //=========================
        int enchantCount = 0;
        int lockedCount = 0;

        for (int i = 0; i < 5; ++i)
        {
            if (item->GetEnchantmentId(EnchantmentSlot(slotEnch[i])) != 0)
                ++enchantCount;

            if (lockedSlots[i])
                ++lockedCount;
        }

        std::string itemName = item->GetTemplate()->Name1;

        if (const ItemLocale* locale = sObjectMgr->GetItemLocale(item->GetEntry()))
        {
            if (locale->Name.size() > 4 && !locale->Name[4].empty())
                itemName = locale->Name[4];
        }

        ChatHandler(player->GetSession()).PSendSysMessage(
            "装备 |cffFF0000{}|r 洗练完成，获得 |cff00FF00{}|r 条随机属性（洗前已锁定 {} 条）",
            itemName,
            enchantCount,
            lockedCount);
    }

    int getRandEnchantment(Item* item)
    {
        uint32 Quality = item->GetTemplate()->Quality;
        uint32 ItemLevel = item->GetTemplate()->ItemLevel;

        // 处理查询结果
        QueryResult qr = WorldDatabase.Query("SELECT enchantID FROM item_enchantment_random_tiers WHERE tier={} ORDER BY RAND() LIMIT 1", 5);
        if (qr)
            return qr->Fetch()[0].Get<uint32>();
        return -1;
    }
};

// ========== 修改点4：在类外部实现Gossip函数 ==========
void ShowLockSelectionMenu(Player* player, Item* targetItem)
{
    if (!player || !targetItem)
        return;

    ObjectGuid const& guid = player->GetGUID();

    auto& selections = g_playerLockSelections[guid];

    if (selections.size() != 5)
        selections.assign(5, false);

    player->PlayerTalkClass->ClearMenus();

    for (uint32 i = 0; i < 5; ++i)
    {
        uint32 enchantId = targetItem->GetEnchantmentId(EnchantmentSlot(7 + i));

        std::string buttonText;

        if (enchantId)
        {
            buttonText =
                "第" + std::to_string(i + 1) + "条 ==> "
                "[" + std::string(selections[i] ? "|cffff0000已锁定|r" : "|cff0000ff未锁定|r") + "]";
        }
        else
        {
            buttonText =
                "[无随机附魔] 第" + std::to_string(i + 1) + "条";
        }

        player->PlayerTalkClass->GetGossipMenu().AddMenuItem(i + 1, GOSSIP_ICON_CHAT, buttonText, GOSSIP_SENDER_MAIN, 100 + i, "", 0, false);
    }

    player->PlayerTalkClass->GetGossipMenu().AddMenuItem(
        6,
        GOSSIP_ICON_CHAT,
        "确认洗练",
        GOSSIP_SENDER_MAIN,
        200,
        "",
        0,
        false);

    player->PlayerTalkClass->GetGossipMenu().AddMenuItem(
        7,
        GOSSIP_ICON_CHAT,
        "取消",
        GOSSIP_SENDER_MAIN,
        201,
        "",
        0,
        false);

    player->PlayerTalkClass->SendGossipMenu(
        DEFAULT_GOSSIP_MESSAGE,
        player->GetGUID());
}

// ========== 修改点5：添加Gossip选择处理函数（在全局） ==========
void OnGossipSelect(Player* player, uint32 sender, uint32 action)
{
    if (!player)
        return;

    auto itemItr = g_playerSelectedItem.find(player->GetGUID());
    if (itemItr == g_playerSelectedItem.end())
    {
        player->PlayerTalkClass->SendCloseGossip();
        return;
    }

    Item* targetItem = player->GetItemByGuid(itemItr->second);
    if (!targetItem)
    {
        g_playerSelectedItem.erase(player->GetGUID());
        g_playerLockSelections.erase(player->GetGUID());

        player->PlayerTalkClass->SendCloseGossip();
        return;
    }

    if (action >= 100 && action <= 104)
    {
        uint32 slot = action - 100;

        uint32 enchantId = targetItem->GetEnchantmentId(EnchantmentSlot(7 + slot));

        if (!enchantId)
        {
            player->GetSession()->SendAreaTriggerMessage("该槽位没有随机属性");
            ShowLockSelectionMenu(player, targetItem);
            return;
        }

        auto& selections = g_playerLockSelections[player->GetGUID()];

        if (selections.size() != 5)
            selections.assign(5, false);

        selections[slot] = !selections[slot];

        std::string msg =
            "第" +
            std::to_string(slot + 1) +
            "条属性";

        msg += selections[slot] ? " 已锁定" : " 已解锁";

        ChatHandler(player->GetSession()).SendSysMessage(msg.c_str());

        ShowLockSelectionMenu(player, targetItem);
        return;
    }

    if (action == 200)
    {
        auto& selections = g_playerLockSelections[player->GetGUID()];

        if (selections.size() != 5)
            selections.assign(5, false);

        XilianRandomEnchantItem script;
        script.ExecuteReforge(player, targetItem, selections);

        g_playerLockSelections.erase(player->GetGUID());
        g_playerSelectedItem.erase(player->GetGUID());

        player->PlayerTalkClass->SendCloseGossip();
        return;
    }

    if (action == 201)
    {
        g_playerLockSelections.erase(player->GetGUID());
        g_playerSelectedItem.erase(player->GetGUID());

        player->PlayerTalkClass->SendCloseGossip();
        return;
    }
}

// ========== 修改点6：需要在WorldScript或Script中注册Gossip选择 ==========
// 添加一个新的脚本来注册Gossip选择处理
class GossipSelectScript : public PlayerScript
{
public:
    GossipSelectScript() : PlayerScript("GossipSelectScript") {}

    void OnPlayerGossipSelect(
        Player* player,
        uint32 menuId,
        uint32 sender,
        uint32 action) override
    {
        if (!player)
            return;

        if (sender != GOSSIP_SENDER_MAIN)
            return;

        switch (action)
        {
        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
        case 200:
        case 201:
            OnGossipSelect(player, sender, action);
            break;

        default:
            break;
        }
    }
};

void AddjiandingScripts() {
    new XilianRandomEnchantItem();
    new GossipSelectScript(); // 添加Gossip处理脚本
}
