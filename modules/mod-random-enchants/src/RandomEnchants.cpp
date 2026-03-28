/*
* Converted from the original LUA script to a module for Azerothcore(Sunwell) :D
*/
#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Chat.h"
#include "Spell.h"

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

    void RollPossibleEnchant(Player* player, Item* item) {
        // Check global enable option
        if (!sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true)) {
            return;
        }

        uint32 Quality = item->GetTemplate()->Quality;
        uint32 Class = item->GetTemplate()->Class;

        if (
            (Quality > 5 || Quality < 3) || /* || item->GetTemplate()->ItemLevel < 188 eliminates enchanting anything that isn't a recognized quality */
            (Class != 2 && Class != 4) /* eliminates enchanting anything but weapons/armor */) {
            return;
        }

        // Randomize base stats
        //ItemTemplate const* itemTemplate = item->GetTemplate();
        //for (uint16 i = 0; i < MAX_ITEM_PROTO_STATS; ++i) {
        //    if (itemTemplate->ItemStat[i].ItemStatValue > 0) {
        //        const uint16 ITEM_FIELD_STAT = ITEM_END; // ITEM_END 是基础字段的结束位置
        //        float randomFactor = frand(0.5f, 1.5f); // Random factor between 50% and 150%
        //        int32 newStatValue = static_cast<int32>(itemTemplate->ItemStat[i].ItemStatValue * randomFactor);
        //        //item->SetUInt32Value(i, newStatValue);
        //        //item->ApplyModInt32Value(i, newStatValue, 1);
        //        //player->HandleStatModifier(UnitMods(i), TOTAL_VALUE, randomFactor, 1);
        //        //player->ApplyStatBuffMod(stat(i), randomFactor, 1);
        //        item->SetInt32Value(ITEM_FIELD_STAT + i, newStatValue);
        //    }
        //}
        //item->SetState(ITEM_CHANGED, player);
        //item->SaveToDB(CharacterDatabase.BeginTransaction());
        int slotRand[3] = { -1, -1, -1 };
        uint32 slotEnch[3] = { 7, 8, 10 };//原为5，7，8
        //if (item->HasSocket()) {
        //    slotEnch[0] = 5;
        //    slotEnch[1] = 6;
        //    slotEnch[2] = 7;
        //}

        // Fetching the configuration values as float
        float enchantChance1 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance1", 70.0f);
        float enchantChance2 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance2", 65.0f);
        float enchantChance3 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance3", 60.0f);

        if (rand_chance() < enchantChance1)
            slotRand[0] = getRandEnchantment(item);
        if (slotRand[0] != -1 && rand_chance() < enchantChance2)
            slotRand[1] = getRandEnchantment(item);
        if (slotRand[1] != -1 && rand_chance() < enchantChance3)
            slotRand[2] = getRandEnchantment(item);

        for (int i = 0; i < 3; i++) {
            if (slotRand[i] != -1) {
                if (sSpellItemEnchantmentStore.LookupEntry(slotRand[i])) { //Make sure enchantment id exists
                    player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), false);
                    item->SetEnchantment(EnchantmentSlot(slotEnch[i]), slotRand[i], 0, 0);
                    player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), true);
                }
            }
        }
        ChatHandler chathandle = ChatHandler(player->GetSession());
        std::vector<std::string> localname = sObjectMgr->GetItemLocale(item->GetEntry())->Name;
        std::string chineseName = localname.size() > 4 ? localname[4] : "";
        if (slotRand[2] != -1)
            //chathandle.PSendSysMessage("拾取 |cffFF0000 %s |r时获得|cffFF0000 3项 |r随机附魔!", item->GetTemplate()->Name1.c_str());
            chathandle.PSendSysMessage("拾取|cffFF0000 {} |r时获得|cffFF0000 3项 |r随机附魔!", chineseName.c_str());
        else if (slotRand[1] != -1)
            chathandle.PSendSysMessage("拾取|cffFF0000 {} |r时获得|cffFF0000 2项 |r随机附魔!", chineseName.c_str());
        else if (slotRand[0] != -1)
            chathandle.PSendSysMessage("拾取|cffFF0000 {} |r时获得|cffFF0000 1项 |r随机附魔!", chineseName.c_str());
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
        uint32 Quality = item->GetTemplate()->Quality;
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
        }
        QueryResult qr = WorldDatabase.Query("SELECT enchantID FROM item_enchantment_random_tiers WHERE tier='{}' AND (class='0' OR class='{}') ORDER BY RAND() LIMIT 1", tier, Class);
        if (!qr)
            return -1;
        return qr->Fetch()[0].Get<uint32>();
    }
};

class XilianRandomEnchantItem : public ItemScript {
public:
    XilianRandomEnchantItem() : ItemScript("XilianRandomEnchantItem") {}

    bool OnUse(Player* player, Item* self, SpellCastTargets const& targets) override {
        Item* item = targets.GetItemTarget();

        // 如果目标物品为空，直接返回 true，避免后续操作
        if (!item) {
            ChatHandler(player->GetSession()).PSendSysMessage("目标物品无效，无法洗练。");
            return true;
        }

        // 获取物品的背包槽位和插槽，确保它们有效
        uint8 bagSlot = item->GetBagSlot();
        uint8 slot = item->GetSlot();
        uint8 selfbagSlot = self->GetBagSlot();
        uint8 selfslot = self->GetSlot();

        // 检查物品槽位有效性
        if (bagSlot == INVENTORY_SLOT_BAG_0 || slot == NULL_SLOT) {
            ChatHandler(player->GetSession()).PSendSysMessage("无法识别物品的位置，操作失败。");
            return true;
        }

        // 直接调用 RollPossibleEnchant 方法为物品应用随机附魔
        //RandomEnchantsPlayer().RollPossibleEnchant(player, item);

        // 销毁目标物品
        uint32 selfEntry = self->GetEntry();
        uint32 itemEntry = item->GetEntry();  // 保存物品的Entry
        player->DestroyItem(bagSlot, slot, true);

        // 销毁洗练物品自身
        player->DestroyItem(selfbagSlot, selfslot, true);

        // 发送成功消息
        //ChatHandler(player->GetSession()).PSendSysMessage("随机附魔属性洗练成功");

        // 重新添加被洗练的物品
        if (player->AddItem(itemEntry, 1)) {
            // 成功添加新物品，发送确认消息
            ChatHandler(player->GetSession()).PSendSysMessage("随机附魔属性洗练成功");
        }
        else {
            // 如果添加新物品失败，发送错误消息
            ChatHandler(player->GetSession()).PSendSysMessage("物品洗练失败，可能是背包已满。");
        }

        return true;
    }
};

void AddRandomEnchantsScripts() {
    new RandomEnchantsPlayer();
    new XilianRandomEnchantItem();
}
