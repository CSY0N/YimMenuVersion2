#include "core/commands/ListCommand.hpp"
#include "game/commands/PlayerCommand.hpp"
#include "game/gta/Scripts.hpp"
#include <cstdint>
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

    static ListCommand _NotificationType{"notificationtype", "Notification Type", "Select the notification to send", g_Notifications, 1964206081};

    class SendNotification : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        void OnCall(Player player) override
        {
            if (!player.IsValid())
                return;

            const int id = player.GetId();
            if (id < 0 || id >= 32)
                return;

            std::vector<std::int64_t> args(15, 0);
            args[0] = -642704387LL;
            args[3] = _NotificationType.GetState();
            Scripts::TriggerScriptEvent(1, std::uint32_t{1} << id, args);
        }
    };

    static SendNotification _SendNotification{"sendnotification", "Send Notification", "Sends the selected notification to the selected player", 0, false};
}
