/*
 *  mod-random-enchant-tooltip
 *  功能：为装备 Tooltip 提供随机附魔查询（5槽：SLOT_0~4）
 *  通信：Addon 消息 RE_CMD
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Item.h"
#include "Bag.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Chat.h"
#include "DBCStores.h"
#include "ObjectAccessor.h"
#include <unordered_map>
#include <ctime>
#include <sstream>
#include <vector>

 /* ---------- 附魔槽位定义 ---------- */
static const EnchantmentSlot RE_SLOTS[5] = {
    PROP_ENCHANTMENT_SLOT_0,  // 随机属性/后缀
    PROP_ENCHANTMENT_SLOT_1,  // 强化祝福
    PROP_ENCHANTMENT_SLOT_2,  // 随机附魔 I
    PROP_ENCHANTMENT_SLOT_3,  // 随机附魔 II
    PROP_ENCHANTMENT_SLOT_4   // 随机附魔 III
};

/* ---------- 辅助：读取附魔描述 ---------- */
static std::string GetEnchantDesc(const SpellItemEnchantmentEntry* entry)
{
    if (!entry) return "无";

    for (int i = 0; i < 16; ++i)
    {
        if (entry->description[i] && entry->description[i][0] != '\0')
        {
            std::string desc = entry->description[i];
            size_t pos = 0;
            while ((pos = desc.find('|', pos)) != std::string::npos)
            {
                desc.replace(pos, 1, "%7C");
                pos += 3;
            }
            std::replace(desc.begin(), desc.end(), '\n', ' ');
            std::replace(desc.begin(), desc.end(), '\r', ' ');
            return desc.empty() ? "无" : desc;
        }
    }
    return "无";
}

/* ---------- Addon 数据发送 ---------- */
static void SendAddonData(Player* player, const std::string& prefix, const std::string& payload)
{
    std::string fullMsg = prefix + "\t" + payload;
    uint32 msgLen = static_cast<uint32>(fullMsg.length() + 1);

    WorldPacket data(SMSG_MESSAGECHAT, 100);
    data << uint8(CHAT_MSG_WHISPER);
    data << uint32(LANG_ADDON);
    data << uint64(player->GetGUID().GetRawValue());
    data << uint32(0);
    data << uint64(player->GetGUID().GetRawValue());
    data << uint32(msgLen);
    data.append(fullMsg.c_str(), msgLen);
    data << uint8(0);

    player->GetSession()->SendPacket(&data);
}

/* ===================== 查询缓存 ===================== */
struct EnchantQueryCache {
    int32  ench[5];
    std::string desc[5];
    time_t timestamp;
};
static std::unordered_map<std::string, EnchantQueryCache> g_enchantQueryCache;

static void CleanupEnchantQueryCache()
{
    time_t now = time(nullptr);
    for (auto it = g_enchantQueryCache.begin(); it != g_enchantQueryCache.end();)
    {
        if (now - it->second.timestamp > 30)
            it = g_enchantQueryCache.erase(it);
        else
            ++it;
    }
}

static std::string BuildCacheKey(const std::string& type, const std::string& param)
{
    return type + ":" + param;
}

static void FillCache(EnchantQueryCache& cache, Item* item)
{
    cache.timestamp = time(nullptr);
    for (uint8 i = 0; i < 5; ++i)
    {
        uint32 eid = item->GetEnchantmentId(RE_SLOTS[i]);
        cache.ench[i] = static_cast<int32>(eid);
        cache.desc[i] = "无";
        if (eid)
        {
            const SpellItemEnchantmentEntry* entry = sSpellItemEnchantmentStore.LookupEntry(eid);
            if (entry) cache.desc[i] = GetEnchantDesc(entry);
        }
    }
}

static void SendEnchantQueryResult(Player* player, Item* item)
{
    if (!player || !item) return;
    std::ostringstream oss;
    oss << "QUERY_RESULT|1";
    for (uint8 i = 0; i < 5; ++i)
    {
        uint32 eid = item->GetEnchantmentId(RE_SLOTS[i]);
        std::string desc = "无";
        if (eid)
        {
            const SpellItemEnchantmentEntry* entry = sSpellItemEnchantmentStore.LookupEntry(eid);
            if (entry) desc = GetEnchantDesc(entry);
        }
        size_t p = 0;
        while ((p = desc.find('|', p)) != std::string::npos)
        {
            desc.replace(p, 1, "%7C");
            p += 3;
        }
        oss << "|" << eid << "|" << desc;
    }
    SendAddonData(player, "RE_CMD", oss.str());
}

static Item* FindItemByLinkInPlayer(Player* player, uint32 entry, int32 suffix)
{
    if (!player) return nullptr;

    for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (item && item->GetEntry() == entry && item->GetItemRandomPropertyId() == suffix)
            return item;
    }
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* pBag = player->GetBagByPos(bag);
        if (!pBag) continue;
        for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
        {
            Item* item = pBag->GetItemByPos(j);
            if (item && item->GetEntry() == entry && item->GetItemRandomPropertyId() == suffix)
                return item;
        }
    }
    return nullptr;
}

/* ===================== 消息处理器 ===================== */
class RETooltipHandler : public PlayerScript
{
public:
    RETooltipHandler() : PlayerScript("RETooltipHandler") {}

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Player* /*receiver*/) override
    {
        if (type != CHAT_MSG_WHISPER || lang != LANG_ADDON)
            return true;

        std::string payload;
        if (msg.find("RE_CMD\t") == 0)
            payload = msg.substr(7);
        else if (msg.find("RE_CMD|") == 0)
            payload = msg.substr(7);
        else
            return true;

        std::vector<std::string> parts;
        std::stringstream ss(payload);
        std::string part;
        while (std::getline(ss, part, '|'))
            parts.push_back(part);

        if (parts.empty()) return true;
        std::string cmd = parts[0];

        /* ---------- RE_QUERY_BAG ---------- */
        if (cmd == "RE_QUERY_BAG")
        {
            SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|背包物品无需查询");
            return false;
        }

        /* ---------- RE_QUERY_SELF ---------- */
        if (cmd == "RE_QUERY_SELF")
        {
            SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|自身装备无需查询");
            return false;
        }

        /* ---------- RE_QUERY_INSPECT ---------- */
        if (cmd == "RE_QUERY_INSPECT")
        {
            std::string targetName = parts.size() > 1 ? parts[1] : "";
            uint8 slot = parts.size() > 2 ? std::stoi(parts[2]) : 0;

            if (targetName.empty() || slot >= EQUIPMENT_SLOT_END)
            {
                SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|参数错误"); return false;
            }

            Player* target = ObjectAccessor::FindPlayerByName(targetName);
            if (!target) { SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|目标不在线"); return false; }

            Item* item = target->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item) { SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|该槽位无装备"); return false; }

            // ====== 关键修复：缓存 key 包含装备 entry 和 suffix，确保换装备后缓存失效 ======
            uint32 entry = item->GetEntry();
            int32 suffix = item->GetItemRandomPropertyId();
            CleanupEnchantQueryCache();
            std::string cacheKey = BuildCacheKey("INSPECT", targetName + ":" + std::to_string(slot) + ":" + std::to_string(entry) + ":" + std::to_string(suffix));

            auto cit = g_enchantQueryCache.find(cacheKey);
            if (cit != g_enchantQueryCache.end())
            {
                std::ostringstream oss; oss << "QUERY_RESULT|1";
                for (uint8 i = 0; i < 5; ++i) oss << "|" << cit->second.ench[i] << "|" << cit->second.desc[i];
                SendAddonData(player, "RE_CMD", oss.str());
                return false;
            }

            EnchantQueryCache cache;
            FillCache(cache, item);
            g_enchantQueryCache[cacheKey] = cache;
            SendEnchantQueryResult(player, item);
            return false;
        }

        /* ---------- RE_QUERY_TRADE ---------- */
                /* ---------- RE_QUERY_TRADE ---------- */
        if (cmd == "RE_QUERY_TRADE")
        {
            if (parts.size() < 2)
            {
                SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|参数错误");
                return false;
            }

            std::string slotArg = parts[1];  // parts[0] 是 "RE_QUERY_TRADE"
            bool isRecipient = (slotArg.find("R:") == 0);
            int slotIndex = atoi(slotArg.substr(2).c_str()) - 1; // 转为 0-based

            if (slotIndex < 0 || slotIndex >= TRADE_SLOT_COUNT)
            {
                SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|交易槽位错误");
                return false;
            }

            Item* item = nullptr;
            TradeSlots tradeSlot = static_cast < TradeSlots > (slotIndex);

            if (isRecipient)
            {
                // 查询对方的交易物品
                if (Player* trader = player->GetTrader())
                {
                    if (TradeData* tradeData = trader->GetTradeData())
                    {
                        item = tradeData->GetItem(tradeSlot);
                    }
                }
            }
            else
            {
                // 查询自己的交易物品
                if (TradeData* tradeData = player->GetTradeData())
                {
                    item = tradeData->GetItem(tradeSlot);
                }
            }

            if (!item)
            {
                SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|交易物品不存在");
                return false;
            }

            // 复用你现有的缓存+发送逻辑
            CleanupEnchantQueryCache();
            uint32 entry = item->GetEntry();
            int32 suffix = item->GetItemRandomPropertyId();
            std::string cacheKey = BuildCacheKey("TRADE", std::to_string(slotIndex) + ":" + std::to_string(entry) + ":" + std::to_string(suffix));

            auto cit = g_enchantQueryCache.find(cacheKey);
            if (cit != g_enchantQueryCache.end())
            {
                std::ostringstream oss;
                oss << "QUERY_RESULT|1";
                for (uint8 i = 0; i < 5; ++i)
                    oss << "|" << cit->second.ench[i] << "|" << cit->second.desc[i];
                SendAddonData(player, "RE_CMD", oss.str());
                return false;
            }

            EnchantQueryCache cache;
            FillCache(cache, item);
            g_enchantQueryCache[cacheKey] = cache;
            SendEnchantQueryResult(player, item);
            return false;
        }

        /* ---------- RE_QUERY_LINK ---------- */
        if (cmd == "RE_QUERY_LINK")
        {
            uint32 entry = parts.size() > 1 ? std::stoul(parts[1]) : 0;
            int32  suffix = parts.size() > 2 ? std::stoi(parts[2]) : 0;
            uint32 unique = parts.size() > 3 ? std::stoul(parts[3]) : 0;

            if (entry == 0) { SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|参数错误"); return false; }

            CleanupEnchantQueryCache();
            std::string cacheKey = BuildCacheKey("LINK", std::to_string(entry) + ":" + std::to_string(suffix) + ":" + std::to_string(unique));
            auto cit = g_enchantQueryCache.find(cacheKey);
            if (cit != g_enchantQueryCache.end())
            {
                std::ostringstream oss; oss << "QUERY_RESULT|1";
                for (uint8 i = 0; i < 5; ++i) oss << "|" << cit->second.ench[i] << "|" << cit->second.desc[i];
                SendAddonData(player, "RE_CMD", oss.str());
                return false;
            }

            Item* item = FindItemByLinkInPlayer(player, entry, suffix);
            if (!item && player->GetTarget())
            {
                if (Player* t = ObjectAccessor::FindPlayer(player->GetTarget()))
                    item = FindItemByLinkInPlayer(t, entry, suffix);
            }
            if (!item)
            {
                Map* map = player->GetMap();
                if (map)
                {
                    uint32 checkCount = 0;
                    Map::PlayerList const& players = map->GetPlayers();
                    for (auto itr = players.begin(); itr != players.end(); ++itr)
                    {
                        if (checkCount++ >= 50) break;
                        Player* p = itr->GetSource();
                        if (!p || p == player) continue;
                        item = FindItemByLinkInPlayer(p, entry, suffix);
                        if (item) break;
                    }
                }
            }

            if (!item) { SendAddonData(player, "RE_CMD", "QUERY_RESULT|0|未找到该装备"); return false; }

            EnchantQueryCache cache;
            FillCache(cache, item);
            g_enchantQueryCache[cacheKey] = cache;
            SendEnchantQueryResult(player, item);
            return false;
        }

        return true;
    }
};

/* ===================== 注册 ===================== */
void Addmod_RandomEnchantTooltipScripts()
{
    new RETooltipHandler();
}
