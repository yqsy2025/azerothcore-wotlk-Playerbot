local json = require("json")
local enchantSlot = 11
local extractSlot = 10 --读取附魔
local ItemUpgrade = {

}
local NPC_Entry = 1700207


local EQUIPMENT_SLOT_START              = 0
local EQUIPMENT_SLOT_HEAD               = 0
local EQUIPMENT_SLOT_NECK               = 1
local EQUIPMENT_SLOT_SHOULDERS          = 2
local EQUIPMENT_SLOT_BODY               = 3
local EQUIPMENT_SLOT_CHEST              = 4
local EQUIPMENT_SLOT_WAIST              = 5
local EQUIPMENT_SLOT_LEGS               = 6
local EQUIPMENT_SLOT_FEET               = 7
local EQUIPMENT_SLOT_WRISTS             = 8
local EQUIPMENT_SLOT_HANDS              = 9
local EQUIPMENT_SLOT_FINGER1            = 10
local EQUIPMENT_SLOT_FINGER2            = 11
local EQUIPMENT_SLOT_TRINKET1           = 12
local EQUIPMENT_SLOT_TRINKET2           = 13
local EQUIPMENT_SLOT_BACK               = 14
local EQUIPMENT_SLOT_MAINHAND           = 15
local EQUIPMENT_SLOT_OFFHAND            = 16
local EQUIPMENT_SLOT_RANGED             = 17
local EQUIPMENT_SLOT_TABARD             = 18
local EQUIPMENT_SLOT_END                = 19

local INVENTORY_SLOT_BAG_START          = 19
local INVENTORY_SLOT_BAG_END            = 23

local INVENTORY_SLOT_ITEM_START         = 23
local INVENTORY_SLOT_ITEM_END           = 39

local INVTYPE_CHEST                     = 5
local INVTYPE_WEAPON                    = 13
local INVTYPE_ROBE                      = 20
local INVTYPE_WEAPONMAINHAND            = 21
local INVTYPE_WEAPONOFFHAND             = 22

local ITEM_CLASS_WEAPON                 = 2
local ITEM_CLASS_ARMOR                  = 4

local ITEM_SUBCLASS_WEAPON_BOW          = 2
local ITEM_SUBCLASS_WEAPON_GUN          = 3
local ITEM_SUBCLASS_WEAPON_CROSSBOW     = 18
local ITEM_SUBCLASS_WEAPON_FISHING_POLE = 20

local EXPANSION_WOTLK                   = 2
local EXPANSION_TBC                     = 1
local PLAYER_VISIBLE_ITEM_1_ENTRYID
local ITEM_SLOT_MULTIPLIER
if GetCoreExpansion() < EXPANSION_TBC then
    PLAYER_VISIBLE_ITEM_1_ENTRYID = 260
    ITEM_SLOT_MULTIPLIER = 12
elseif GetCoreExpansion() < EXPANSION_WOTLK then
    PLAYER_VISIBLE_ITEM_1_ENTRYID = 346
    ITEM_SLOT_MULTIPLIER = 16
else
    PLAYER_VISIBLE_ITEM_1_ENTRYID = 283
    ITEM_SLOT_MULTIPLIER = 2
end

local INVENTORY_SLOT_BAG_0 = 255

local SlotNames            = {
    [EQUIPMENT_SLOT_HEAD] = { "头盔·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_NECK] = { "项链·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_SHOULDERS] = { "肩膀·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_BODY] = { "衬衫·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_CHEST] = { "胸部·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_WAIST] = { "腰部·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_LEGS] = { "腿部·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_FEET] = { "脚部·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_WRISTS] = { "手腕·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_HANDS] = { "手套·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_FINGER1] = { "戒指·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_FINGER2] = { "戒指·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_TRINKET1] = { "饰品·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_TRINKET2] = { "饰品·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_BACK] = { "披风·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_MAINHAND] = { "主手武器·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_OFFHAND] = { "副手武器·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_RANGED] = { "远程武器·", nil, nil, nil, nil, nil, nil, nil, nil },
    [EQUIPMENT_SLOT_TABARD] = { "战袍·", nil, nil, nil, nil, nil, nil, nil, nil },
}
local Locales              = {
    { "更新菜单", nil, nil, nil, nil, nil, nil, nil, nil },
    { "移除所有幻化", nil, nil, nil, nil, nil, nil, nil, nil },
    { "移除所有已装备物品的幻化吗？", nil, nil, nil, nil, nil, nil, nil, nil },
    { "升级装备将消耗以下材料且无法退还,是否确定升级？", nil, nil, nil, nil, nil, nil, nil, nil },
    { "移除%s的幻化?", nil, nil, nil, nil, nil, nil, nil, nil },
    { "返回", nil, nil, nil, nil, nil, nil, nil, nil },
    { "移除幻化", nil, nil, nil, nil, nil, nil, nil, nil },
    { "已从已装备的物品中移除幻化", nil, nil, nil, nil, nil, nil, nil, nil },
    { "您当前没有已幻化的已装备物品", nil, nil, nil, nil, nil, nil, nil, nil },
    { "%s 幻化已移除", nil, nil, nil, nil, nil, nil, nil, nil },
    { "%s 插槽上没有进行幻化", nil, nil, nil, nil, nil, nil, nil, nil },
    { "%s 已幻化", nil, nil, nil, nil, nil, nil, nil, nil },
    { "所选物品不适用", nil, nil, nil, nil, nil, nil, nil, nil },
    { "所选的物品不存在", nil, nil, nil, nil, nil, nil, nil, nil },
    { "装备槽为空", nil, nil, nil, nil, nil, nil, nil, nil },
    { "你的%ss不足", nil, nil, nil, nil, nil, nil, nil, nil },
    { "金币不足", nil, nil, nil, nil, nil, nil, nil, nil },
    { "开始升级", nil, nil, nil, nil, nil, nil, nil, nil },
    { "确定升级", nil, nil, nil, nil, nil, nil, nil, nil },
}
local function LocText(id, p) -- "%s":format("test")
    if Locales[id] then
        local s = Locales[id][p:GetDbcLocale() + 1] or Locales[id][1]
        if s then
            return s
        end
    end
    return "Text not found: " .. (id or 0)
end

local function GetSlotName(slot, locale)
    if not SlotNames[slot] then return end
    return locale and SlotNames[slot][locale + 1] or SlotNames[slot][1]
end

math.randomseed(os.time())
local menu_id = math.random(1000)
local UpgradeSetting = {}
local function matchSetting(class, itemLvl, quality, guid, player, EquipItem)
    local levelParams = nil
    for _, row in pairs(UpgradeSetting) do
        if row[2] > quality or class ~= row[5] or itemLvl < row[3] or itemLvl > row[4] then
            goto continue
        end
        --查询当前装备的强化等级
        CurrUpgrade = CharDBQuery(
            "SELECT instance_guid, upgrade_lvl, owner_guid, increase_ratio FROM _item_upgrade WHERE `instance_guid` = '" ..
            guid .. "';");
        CurrLevel = 0
        if (CurrUpgrade ~= nil) then
            CurrLevel = CurrUpgrade:GetUInt32(1)
        end
        --player:SendBroadcastMessage("当前等级:"..CurrLevel)	
        for lvl, value in pairs(row[6]) do
            if CurrLevel < lvl then
                levelParams = value
                levelParams.money = tonumber(levelParams.money)
                levelParams.increase_ratio = tonumber(levelParams.increase_ratio)
                levelParams.success_ratio = tonumber(levelParams.success_ratio)
                levelParams.random_properties_id = tonumber(levelParams.random_properties_id)
                levelParams.level = lvl
                levelParams.currLevel = CurrLevel
                local materials = {}
                -- 读取需求材料
                local popup = "升级将消耗相应材料,是否确定升级？"
                if tonumber(levelParams.material_1.count) > 0 then
                    materials[1] = {
                        entry = tonumber(levelParams.material_1.entry),
                        count = tonumber(levelParams
                            .material_1.count)
                    }
                end
                if tonumber(levelParams.material_2.count) > 0 then
                    materials[2] = {
                        entry = tonumber(levelParams.material_2.entry),
                        count = tonumber(levelParams
                            .material_2.count)
                    }
                end
                if tonumber(levelParams.material_3.count) > 0 then
                    materials[3] = {
                        entry = tonumber(levelParams.material_3.entry),
                        count = tonumber(levelParams
                            .material_3.count)
                    }
                end
                if tonumber(levelParams.material_4.count) > 0 then
                    materials[4] = {
                        entry = tonumber(levelParams.material_4.entry),
                        count = tonumber(levelParams
                            .material_4.count)
                    }
                end
                if tonumber(levelParams.material_5.count) > 0 then
                    materials[5] = {
                        entry = tonumber(levelParams.material_5.entry),
                        count = tonumber(levelParams
                            .material_5.count)
                    }
                end
                levelParams.materials = materials
                break
            end
        end
        if levelParams ~= nil then
            break
        end
        --player:SendBroadcastMessage(EquipItem:GetItemLink(player:GetDbcLocale()) .."[" .. EquipItem:GetGUIDLow() .. "]|cFF33FF33已强化至最高等级")
        ::continue::
    end
    return levelParams
end
--加载已穿戴装备
local function loadEquipItems(event, player, creature)
    for slot = EQUIPMENT_SLOT_START, EQUIPMENT_SLOT_END - 1 do
        local equipment = player:GetItemByPos(INVENTORY_SLOT_BAG_0, slot)
        if equipment then
            --player:SendBroadcastMessage("穿戴装备:" .. equipment:GetEntry())
            local levelParams = matchSetting(equipment:GetClass(), equipment:GetItemLevel(), equipment:GetQuality(),
                equipment:GetGUIDLow(), player, equipment)
            local extMsg = "|cFFFFFF00不可强化"
            local randomProperty = equipment:GetRandomProperty()
            local randomSuffix = equipment:GetRandomSuffix()

            if levelParams ~= nil and ((randomProperty == 0 and randomSuffix == 0) or levelParams.currLevel > 0) then
                extMsg = "|cFF00CC33可强化 |cFFFFFF00" .. levelParams.currLevel .. "|r -> |cFFFF0099" .. levelParams.level
            end
            local slotName = GetSlotName(slot, player:GetDbcLocale())
            if slotName then
                if levelParams ~= nil and ((randomProperty == 0 and randomSuffix == 0) or levelParams.currLevel > 0) then
                    --equipment:GetItemLink(player:GetDbcLocale())
                    player:GossipMenuAddItem(1, slotName .. equipment:GetItemLink(player:GetDbcLocale()) .. " " .. extMsg,
                        slot, 1)
                else
                    player:GossipMenuAddItem(1, slotName .. equipment:GetItemLink(player:GetDbcLocale()) .. " " .. extMsg,
                        slot, 0)
                end
            end
        end
    end
    player:GossipSendMenu(100, creature, menu_id)
end
local npcTextId = 100 --1000002
local function OnGossipHello(event, player, creature)
    player:GossipClearMenu()
    loadEquipItems(event, player, creature)
    player:GossipSendMenu(npcTextId, creature, menu_id)
end
-- 执行强化
-- 重要：先完整检查金币和所有材料，再一次性扣除，避免资源扣一半却无法升级。
-- success_ratio 真正参与随机判定；失败时资源已消耗，但装备等级和属性不变。
local function doUpgrade(event, player, creature, slotid, uiAction)
    local currItem = player:GetItemByPos(INVENTORY_SLOT_BAG_0, slotid)
    if currItem == nil then
        ItemUpgrade.preview(event, player, creature, slotid, 1)
        return
    end

    local params = matchSetting(currItem:GetClass(), currItem:GetItemLevel(), currItem:GetQuality(),
        currItem:GetGUIDLow(), player, currItem)

    if not currItem or not params then
        ItemUpgrade.preview(event, player, creature, slotid, 1)
        return
    end

    local price = tonumber(params.money) or 0
    local materials = params.materials or {}

    -- ============================================================
    -- 第一阶段：只检查，不扣除任何资源
    -- ============================================================
    if player:GetCoinage() < price then
        player:SendNotification(LocText(17, player))
        ItemUpgrade.preview(event, player, creature, slotid, 1)
        return
    end

    for i = 1, #materials do
        local materialEntry = materials[i].entry
        local requiredCount = materials[i].count
        local ownedCount = player:GetItemCount(materialEntry)

        if ownedCount < requiredCount then
            player:SendBroadcastMessage(GetItemLink(materialEntry, 4) .. " 数量不足，升级取消")
            ItemUpgrade.preview(event, player, creature, slotid, 1)
            return
        end
    end

    -- ============================================================
    -- 第二阶段：所有资源都足够，现在一次性扣除
    -- ============================================================
    if price > 0 then
        player:ModifyMoney(-price)
    end

    for i = 1, #materials do
        player:RemoveItem(materials[i].entry, materials[i].count)
    end

    -- ============================================================
    -- 第三阶段：真正执行成功率判定
    -- ============================================================
    local successRatio = tonumber(params.success_ratio) or 100
    if successRatio < 0 then successRatio = 0 end
    if successRatio > 100 then successRatio = 100 end

    local roll = math.random(1, 10000)
    local success = roll <= math.floor(successRatio * 100)

    if not success then
        player:SendBroadcastMessage(
            currItem:GetItemLink(player:GetDbcLocale()) ..
            " 强化失败！成功率 " .. successRatio .. "%，本次消耗的金币和材料无法退还。")
        ItemUpgrade.preview(event, player, creature, slotid, 1)
        return
    end

    local enchantId = currItem:GetEnchantmentId(enchantSlot)
    local extractEnchant = currItem:GetEnchantmentId(extractSlot)

    -- 取消当前强化属性
    currItem:ApplyItemMods(slotid, false)

    local query
    if params.level == 1 then
        query = string.format(
            "INSERT INTO _item_upgrade (`instance_guid`, `upgrade_lvl`, `owner_guid`, `increase_ratio`) " ..
            "VALUES ('%d','%d','%d','%d')",
            currItem:GetGUIDLow(), params.level, player:GetGUIDLow(), params.increase_ratio)
    else
        query = string.format(
            "UPDATE _item_upgrade SET `upgrade_lvl`='%d', `increase_ratio`='%d' " ..
            "WHERE `instance_guid`=%d AND `owner_guid`=%d",
            params.level, params.increase_ratio, currItem:GetGUIDLow(), player:GetGUIDLow())
    end

    CharDBQuery(query)

    if params.random_properties_id > 0 then
        currItem:SetItemRandomProperties(params.random_properties_id)
    end

    -- 重新应用新的强化属性
    currItem:ApplyItemMods(slotid, true)

    if enchantId ~= 0 then
        currItem:SetEnchantment(enchantId, enchantSlot)
    end

    if extractEnchant ~= 0 then
        currItem:SetEnchantment(extractEnchant, extractSlot)
    end

    player:SendBroadcastMessage(
        currItem:GetItemLink(player:GetDbcLocale()) .. " 升级成功 => " .. params.level)

    ItemUpgrade.preview(event, player, creature, slotid, 1)
end

function ItemUpgrade.preview(event, player, creature, slotid, uiAction)
    if slotid >= EQUIPMENT_SLOT_START and slotid < EQUIPMENT_SLOT_END then
        if uiAction == 1 then --强化预览
            player:GossipClearMenu()
            local EquipItem = player:GetItemByPos(INVENTORY_SLOT_BAG_0, slotid)
            if EquipItem then
                local template = EquipItem:GetItemTemplate()


                local levelParams = matchSetting(EquipItem:GetClass(), EquipItem:GetItemLevel(), EquipItem:GetQuality(),
                    EquipItem:GetGUIDLow(), player, EquipItem)
                local randomProperty = EquipItem:GetRandomProperty()
                local randomSuffix = EquipItem:GetRandomSuffix()

                if levelParams == nil or (levelParams.currLevel == 0 and (randomProperty > 0 or randomSuffix > 0)) then
                    player:GossipMenuAddItem(7, "返回", EQUIPMENT_SLOT_END + 1, 0) --返回
                    player:GossipSendMenu(1000001, creature, menu_id)
                else
                    local upgradeAble = true
                    --
                    local upgradeText = EquipItem:GetName() ..
                        "当前强化等级：|cFFFFFF00" ..
                        levelParams.currLevel .. " |r 强化至 |cFFFF0099" .. (levelParams.currLevel + 1) .. "|cffcccccc 级|r"
                    upgradeText = upgradeText .. "\n------------------------------\n"
                    upgradeText = upgradeText ..
                        "属性提升：|cFF66FF00+" ..
                        math.max(0, tonumber(levelParams.increase_ratio) - 100) ..
                        "%|cffcccccc|r 成功几率：|cFFFF0000" .. levelParams.success_ratio .. "%|cffcccccc|r"
                    upgradeText = upgradeText .. "\n-----------材料消耗-----------"

                    player:GossipMenuAddItem(9, upgradeText, slotid, 1)
                    -- 读取需求材料
                    local materials = levelParams.materials
                    local popup = "升级" ..
                        EquipItem:GetItemLink(player:GetDbcLocale()) ..
                        "(Lv" .. EquipItem:GetItemLevel() .. ")将消耗以下材料,是否确定升级？\n---------------------------"
                    for i = 1, #materials do
                        local materialEntry = materials[i].entry
                        local count = player:GetItemCount(materialEntry)
                        local countText = "|cFF00CC33" .. count .. "/" .. materials[i].count;
                        popup = popup .. "\n" .. GetItemLink(materialEntry, 4) .. " x " .. materials[i].count
                        if count < materials[i].count then
                            player:SendBroadcastMessage(GetItemLink(materialEntry, 4) .. "|cFFCC3366数量不足,无法升级")
                            upgradeAble = false
                            countText = "|cFFCC3366" .. count .. "/" .. materials[i].count;
                        end
                        --定义材料点击参数
                        player:GossipMenuAddItem(1, GetItemLink(materialEntry, 4) .. "  " .. countText, slotid,
                            materialEntry)
                    end
                    popup = popup .. "\n---------------------------"
                    if upgradeAble == true then
                        player:GossipMenuAddItem(0, "|TInterface\\ICONS\\120002.blp:30:30:-21:0|t 确定升级|r", slotid, 2,
                            false, popup, levelParams.money)
                    end
                    player:GossipMenuAddItem(7, "返回", EQUIPMENT_SLOT_END + 1, 0) --返回
                    player:GossipSendMenu(1, creature, 1)
                end
            else
                OnGossipHello(event, player, creature)
            end
        elseif uiAction == 2 then --执行强化
            doUpgrade(event, player, creature, slotid, uiAction)
        elseif uiAction > 10 then --点击材料
            player:SendBroadcastMessage(GetItemLink(uiAction, 4) .. " 持有数量:" .. player:GetItemCount(uiAction))
            ItemUpgrade.preview(event, player, creature, slotid, 1)
        else
            OnGossipHello(event, player, creature)
        end
    elseif slotid == EQUIPMENT_SLOT_END + 1 then -- 返回首页
        OnGossipHello(event, player, creature)
    else
        ItemUpgrade.preview(event, player, creature, slotid, uiAction)
    end
end

--[[
local function onEquip(event,player,item)
	player:SendBroadcastMessage(item:GetItemLink(player:GetDbcLocale()).." 属性:" .. item:GetStatsCount())
end
RegisterPlayerEvent(29, onEquip)
]]

--加载数据库强化设置
local function onServerLoaded(event)
    local Query = WorldDBQuery("select * FROM acore_characters.item_upgrade_setting")
    if Query then
        --print(Query:GetRowCount())
        local index = 0
        repeat
            UpgradeSetting[index] = {            -- id
                Query:GetUInt32(0),              --id
                Query:GetUInt32(1),              -- quality
                Query:GetUInt32(2),              -- min_item_lvl
                Query:GetUInt32(3),              -- max_item_lvl
                Query:GetUInt32(4),              -- class
                json.decode(Query:GetString(6)), -- levels
            }
            index = index + 1
        until not Query:NextRow()
    end
    --TextQuery = WorldDBQuery("SELECT value FROM acore_modules.config WHERE `key` = 'eluna_item_upgrade_text';");
    --if (TextQuery ~= nil) then
        --local result = json.decode(TextQuery:GetString(0))
        --if result then
            --npcTextId = result.desc
        --end
    --end
end
local function syncUpgradeItems(event, player)
    local Query = CharDBQuery("SELECT * FROM _item_upgrade WHERE `owner_guid`=" .. player:GetGUIDLow())
    if Query then
        local index = 0
        repeat
            --查询装备是否存在
            local InstanceQuery = CharDBQuery("SELECT itemEntry FROM item_instance WHERE `guid`=" ..
                Query:GetUInt32(0))
            if InstanceQuery == nil then
                PrintInfo(Query:GetUInt32(0) .. " 强化等级:" .. Query:GetUInt32(1) .. " 物品副本不存在,已删除")
                CharDBQuery("DELETE FROM _item_upgrade WHERE `instance_guid`=" ..
                    Query:GetUInt32(0))
            end
            index = index + 1
        until not Query:NextRow()
    end
end
local PLAYER_EVENT_ON_LOGIN  = 3
local PLAYER_EVENT_ON_LOGOUT = 4
RegisterCreatureGossipEvent(NPC_Entry, 1, OnGossipHello)
RegisterCreatureGossipEvent(NPC_Entry, 2, ItemUpgrade.preview)
RegisterServerEvent(33, onServerLoaded)
RegisterPlayerEvent(PLAYER_EVENT_ON_LOGIN, syncUpgradeItems)
RegisterPlayerEvent(PLAYER_EVENT_ON_LOGOUT, syncUpgradeItems)
