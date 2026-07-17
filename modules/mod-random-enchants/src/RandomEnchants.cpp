/*
* Converted from the original LUA script to a module for Azerothcore(Sunwell) :D
*/
#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Chat.h"
#include "Spell.h"
#include "item.h"

// Enum for item qualities
enum ItemQuality {
    GREY = 0,
    WHITE,
    GREEN,
    BLUE,
    PURPLE,
    ORANGE
};

class RandomEnchantsPlayer : public PlayerScript {
public:
    RandomEnchantsPlayer() : PlayerScript("RandomEnchantsPlayer") { }

    void OnPlayerLogin(Player* player) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.AnnounceOnLogin", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            ChatHandler(player->GetSession()).SendSysMessage(sConfigMgr->GetOption<std::string>("RandomEnchants.OnLoginMessage", "This server is running a RandomEnchants Module.").c_str());
    }

    void OnPlayerGroupRollRewardItem(Player* player, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnGroupRoll", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
        {
            if (item->IsEnchanted())
                return;
            else
                RollPossibleEnchant(player, item);
        }
    }

    void OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnLoot", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
        {
            if (item->IsEnchanted())
                return;
            else
                RollPossibleEnchant(player, item);
        }
    }

    //void OnItemCreate(Item* item, ItemTemplate const* itemProto, Player* player) {
    void OnPlayerCreateItem(Player* player, Item* item, uint32 /*count*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnCreate", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
        {
            if (item->IsEnchanted())
                return;
            else
                RollPossibleEnchant(player, item);
        }
    }

    void OnPlayerQuestRewardItem(Player* player, Item* item, uint32 /*count*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnQuestReward", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
        {
            if (item->IsEnchanted())
                return;
            else
                RollPossibleEnchant(player, item);
        }
    }

    void OnPlayerStoreNewItem(Player* player, Item* item, uint32 /*count*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnCreate", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
        {
            if (item->IsEnchanted())
                return;
            else
                RollPossibleEnchant(player, item);
        }
    }

    void OnPlayerAfterStoreOrEquipNewItem(Player* player, uint32 /*vendorslot*/, Item* item, uint8 /*count*/, uint8 /*bag*/, uint8 /*slot*/, ItemTemplate const* /*pProto*/, Creature* /*pVendor*/, VendorItem const* /*crItem*/, bool /*bStore*/)  override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.BuyfromNpc", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
        {
            if (item->IsEnchanted())
                return;
            else
                RollPossibleEnchant(player, item);
        }
    }

    void RollPossibleEnchant(Player* player, Item* item)
    {
        if (!sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            return;

        if (!player || !item || !item->GetTemplate())
            return;

        ItemTemplate const* proto = item->GetTemplate();

        uint32 Quality = proto->Quality;
        uint32 Class = proto->Class;

        // 只处理 武器 / 护甲
        if (Quality > 5 || (Class != ITEM_CLASS_WEAPON && Class != ITEM_CLASS_ARMOR))
            return;

        // 根据品质决定最大附魔数量
        int maxEnchants = 0;
        switch (Quality)
        {
        case 5: // 橙色(传说)
            maxEnchants = 5;
            break;
        case 4: // 紫色(史诗)
            maxEnchants = 4;
            break;
        case 3: // 蓝色(精良)
        case 2: // 绿色(优秀)
        case 1: // 白色(普通)
        case 0: // 灰色(粗糙)
        default:
            maxEnchants = 3;
            break;
        }

        // 扩展槽位数组到5个，只使用7-11槽位
        int slotRand[5] = { -1, -1, -1, -1, -1 };
        uint32 slotEnch[5] = { 7, 8, 9, 10, 11 };

        // 配置每条附魔的触发几率
        float enchantChance[5];
        enchantChance[0] = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance1", 70.0f);
        enchantChance[1] = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance2", 65.0f);
        enchantChance[2] = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance3", 60.0f);
        enchantChance[3] = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance4", 55.0f);
        enchantChance[4] = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance5", 50.0f);

        // 循环生成附魔，最多生成maxEnchants条
        int enchantCount = 0;
        for (int i = 0; i < maxEnchants; i++)
        {
            // 第一条附魔不需要前置条件，后续的附魔需要前一条成功
            if (i == 0)
            {
                if (rand_chance() < enchantChance[i])
                {
                    slotRand[i] = getRandEnchantment(item);
                    if (slotRand[i] != -1)
                        enchantCount++;
                }
                else
                    break; // 第一条失败就直接退出
            }
            else
            {
                // 前一条附魔成功才尝试生成当前附魔
                if (slotRand[i - 1] != -1 && rand_chance() < enchantChance[i])
                {
                    slotRand[i] = getRandEnchantment(item);
                    if (slotRand[i] != -1)
                        enchantCount++;
                    else
                        break; // 获取附魔失败就停止
                }
                else
                    break; // 前一条不存在或几率未触发就停止
            }
        }

        // 应用附魔到装备
        for (int i = 0; i < 5; i++)
        {
            if (slotRand[i] != -1)
            {
                if (sSpellItemEnchantmentStore.LookupEntry(slotRand[i]))
                {
                    player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), false);
                    item->SetEnchantment(EnchantmentSlot(slotEnch[i]), slotRand[i], 0, 0);
                    player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), true);
                }
            }
        }

        // 如果获得了随机附魔，发送提示消息
        if (enchantCount > 0)
        {
            ChatHandler chathandle(player->GetSession());
            // 获取物品模板
            const ItemTemplate* itemTemplate = sObjectMgr->GetItemTemplate(item->GetEntry());
            if (!itemTemplate)
                return;

            // 构建物品链接
            std::ostringstream oss;
            oss << "|c";
            oss << std::hex << ItemQualityColors[itemTemplate->Quality] << std::dec;
            oss << "|Hitem:";
            oss << itemTemplate->ItemId;
            oss << ":0:0:0:0:0:0:0:0:0|h[";

            // 获取本地化名称（带兜底）
            std::string itemName;
            const ItemLocale* itemLocale = sObjectMgr->GetItemLocale(item->GetEntry());
            if (itemLocale && !itemLocale->Name.empty())
            {
                int32 locIndex = player->GetSession()->GetSessionDbLocaleIndex();
                if (locIndex >= 0 && locIndex < static_cast<int32>(itemLocale->Name.size()) && !itemLocale->Name[locIndex].empty())
                    itemName = itemLocale->Name[locIndex];
                else
                    itemName = itemLocale->Name[0];  // 兜底：英文
            }
            if (itemName.empty())
                itemName = itemTemplate->Name1;  // 最终兜底：物品模板的默认名字

            oss << itemName;
            oss << "]|h|r";

            std::string itemLink = oss.str();

            chathandle.PSendSysMessage(
                "拾取{}时获得|cffFF0000{}项|r随机附魔!",
                itemLink, enchantCount);
        }
    }

    uint32 getRandEnchantment(Item* item) {
        if (!item->GetTemplate())
            return -1;

        uint32 Class = item->GetTemplate()->Class;
        if (Class == 0)
            return -1;
        //std::string ClassQueryString = "";//已无效作废直接用class
        //switch (Class) {
        //case 2:
        //    ClassQueryString = "WEAPON";
        //    break;
        //case 4:
        //    ClassQueryString = "ARMOR";
        //    break;
        //}
        //if (ClassQueryString == "")
        //    return -1;
        //if (Class == 0)
        //    return -1;
        //}
        /*uint32 Quality = item->GetTemplate()->Quality;
        uint32 ItemLevel = item->GetTemplate()->ItemLevel;
        int rarityRoll = -1;
        switch (Quality) {
        case GREY:
            rarityRoll = rand_norm() * 25;
            break;
        case WHITE:
            rarityRoll = rand_norm() * 50;
            break;
        case GREEN:
            rarityRoll = 45 + (rand_norm() * 20);
            break;
        case BLUE:
            rarityRoll = 65 + (rand_norm() * 15);
            break;
        case PURPLE:
            rarityRoll = 80 + (rand_norm() * 14);
            break;
        case ORANGE:
            rarityRoll = 93;
            break;
        }
        if (rarityRoll < 0)
            return -1;
        int tier = 0;
        if (rarityRoll <= 44)
            tier = 1;
        else if (rarityRoll <= 64)
            tier = 2;
        else if (rarityRoll <= 79)
            tier = 3;
        else if (rarityRoll <= 92)
            tier = 4;
        else
            tier = 5;
        // 根据装等强制限制tier
        if (ItemLevel < 50)
        {
            tier = 1;  // 装等1-49只能获得tier1
        }
        else if(ItemLevel < 100) {
            // 装等50-99只能获得tier1或tier2
                tier = 2;
        }*/
        QueryResult qr = WorldDatabase.Query("SELECT enchantID FROM item_enchantment_random_tiers WHERE tier='{}' AND (class='0' OR class='{}') ORDER BY RAND() LIMIT 1", 5, Class);
        if (!qr)
            return -1;
        return qr->Fetch()[0].Get<uint32>();
    }
};

void AddRandomEnchantsScripts() {
    new RandomEnchantsPlayer();
}
