#include "core/commands/ListCommand.hpp"
#include "game/commands/PlayerCommand.hpp"
#include "types/script/ScriptEvent.hpp"

#include <vector>

namespace YimMenu::Features
{
    static std::vector<std::pair<int, const char*>> g_Notifications{
        {1964206081, "Armored Truck"},
        {1269083963, "Easy Way Out"},
        {-1369501940, "Smuggler Plane"},
        {1478734661, "Delivered a Bag of Cash"},
        {2073500011, "Added to Wanted Level"},
        {-1496350145, "Off the Radar"},
        {-376947579, "The King"},
        {-1853142904, "Revealed All Players"}
    };

    static ListCommand _NotificationType{
        "notificationtype",
        "Notification Type",
        "Select the notification to send",
        g_Notifications,
        1964206081
    };

    class SendNotification : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        void OnCall(Player player) override
        {
            if (!player.IsValid())
                return;

            SCRIPT_EVENT_NOTIFICATION message;

            message.NotificationType = _NotificationType.GetState();
            message.SetPlayer(player.GetId());
            message.Send();
        }
    };

    static SendNotification _SendNotification{"sendnotification", "Send Notification", "Sends the selected notification to the selected player", 0, false};
}
