#ifndef MOD_ITEM_UPGRADE_H
#define MOD_ITEM_UPGRADE_H

#include "DatabaseEnvFwd.h"
#include "ItemTemplate.h"
#include "PlayerScript.h"

class ItemUpgradePlayerScript final : public PlayerScript
{
public:
    ItemUpgradePlayerScript()
        : PlayerScript("ItemUpgradePlayerScript")
    {
    }

    virtual void OnAfterApplyItemMods(Player* player, Item* item, uint8 slot, bool apply) override;
    //void SendItemPacket(Player* player, Item* item) const;
    virtual void OnPlayerLogin(Player* player) override;

private:
    static ItemTemplate GetUpgradeItemInstance(Item* item, QueryResult result);
    static void ApplyUpgradeItemBonuses(Player* player, ItemTemplate const* proto, uint8 slot, bool apply, bool onlyLevelScale = false);
    static void SendItemPacket(Player* player, Item* item);
};

void Addmod_item_upgradeScripts();

#endif // MOD_ITEM_UPGRADE_H
