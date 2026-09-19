#include "game/commands/PlayerCommand.hpp"
#include "types/script/ScriptEvent.hpp"

namespace YimMenu::Features
{
    class SendPizza : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        void OnCall(Player player) override
        {
            if (!player.IsValid())
                return;

            SCRIPT_EVENT_START_ACTIVITY message;
            message.Activity = 340;
            message.Param = 0;
            message.SetPlayer(player.GetId());
            message.Send();
        }
    };

    static SendPizza _SendPizza{"sendpizza", "Send to Pizza", "Sends the selected player to the pizza delivery activity", 0, false};
}
