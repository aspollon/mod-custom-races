/*
 * mod-custom-races - hide every armor piece of a character for everyone.
 * Weapons stay visible. The choice is saved per character in coa_hide_armor and comes back at login.
 */

#include "Chat.h"
#include "CommandScript.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <string>
#include <unordered_set>

using namespace Acore::ChatCommands;

namespace
{
    std::unordered_set<ObjectGuid::LowType> HiddenArmor;

    bool IsArmorSlot(uint8 slot)
    {
        switch (slot)
        {
            case EQUIPMENT_SLOT_HEAD: case EQUIPMENT_SLOT_SHOULDERS: case EQUIPMENT_SLOT_BODY:
            case EQUIPMENT_SLOT_CHEST: case EQUIPMENT_SLOT_WAIST: case EQUIPMENT_SLOT_LEGS:
            case EQUIPMENT_SLOT_FEET: case EQUIPMENT_SLOT_WRISTS: case EQUIPMENT_SLOT_HANDS:
            case EQUIPMENT_SLOT_BACK: case EQUIPMENT_SLOT_TABARD:
                return true;
            default:
                return false;
        }
    }

    void RefreshArmor(Player* player)
    {
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            if (IsArmorSlot(slot))
                player->SetVisibleItemSlot(slot, player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
    }

    bool IsHidden(ObjectGuid::LowType guid)
    {
        return HiddenArmor.count(guid) != 0;
    }

    void Remember(ObjectGuid::LowType guid, bool hidden)
    {
        if (hidden)
        {
            HiddenArmor.insert(guid);
            CharacterDatabase.Execute("REPLACE INTO coa_hide_armor (guid) VALUES ({})", guid);
        }
        else
        {
            HiddenArmor.erase(guid);
            CharacterDatabase.Execute("DELETE FROM coa_hide_armor WHERE guid = {}", guid);
        }
    }
}

class CustomRacesHideArmorPlayer : public PlayerScript
{
public:
    CustomRacesHideArmorPlayer() : PlayerScript("CustomRacesHideArmorPlayer", {
        PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_ON_AFTER_SET_VISIBLE_ITEM_SLOT }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (!player)
            return;

        if (CharacterDatabase.Query("SELECT 1 FROM coa_hide_armor WHERE guid = {}", player->GetGUID().GetCounter()))
        {
            HiddenArmor.insert(player->GetGUID().GetCounter());
            RefreshArmor(player);
        }
    }

    void OnPlayerLogout(Player* player) override
    {
        if (player)
            HiddenArmor.erase(player->GetGUID().GetCounter());
    }

    void OnPlayerAfterSetVisibleItemSlot(Player* player, uint8 slot, Item* /*item*/) override
    {
        if (!player || !IsArmorSlot(slot) || !IsHidden(player->GetGUID().GetCounter()))
            return;

        player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENTRYID + (slot * 2), 0);
        player->SetUInt32Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (slot * 2), 0);
    }
};

class CustomRacesHideArmorCommand : public CommandScript
{
public:
    CustomRacesHideArmorCommand() : CommandScript("CustomRacesHideArmorCommand") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "hidearmor", HandleHideArmor, SEC_PLAYER, Console::No },
        };
        return commandTable;
    }

    static bool HandleHideArmor(ChatHandler* handler, Optional<std::string> action)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        ObjectGuid::LowType const guid = player->GetGUID().GetCounter();
        if (action)
        {
            std::string const what = *action;
            if (what == "status")
            {
                handler->PSendSysMessage(IsHidden(guid) ? "Armor hidden." : "Armor shown again.");
                return true;
            }
            if (what == "on" || what == "1")
            {
                Remember(guid, true);
                RefreshArmor(player);
                handler->PSendSysMessage("Armor hidden.");
                return true;
            }
            if (what == "off" || what == "0")
            {
                Remember(guid, false);
                RefreshArmor(player);
                handler->PSendSysMessage("Armor shown again.");
                return true;
            }
        }

        bool const hide = !IsHidden(guid);
        Remember(guid, hide);
        RefreshArmor(player);
        handler->PSendSysMessage(hide ? "Armor hidden." : "Armor shown again.");
        return true;
    }
};

void AddSC_hide_armor()
{
    new CustomRacesHideArmorPlayer();
    new CustomRacesHideArmorCommand();
}
