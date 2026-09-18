#include "game/commands/PlayerCommand.hpp"
#include "game/gta/Scripts.hpp"

namespace YimMenu::Features
{
    class SendPizza : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        void OnCall(Player player) override
        {
            if (!player.IsValid())
                return;

            const int id = player.GetId();
            if (id < 0 || id >= 32)
                return;

            Scripts::TriggerScriptEvent(1, std::uint32_t{1} << id, {1450115979LL, -1, -1, 340});
        }
    };

    static SendPizza _SendPizza{"sendpizza", "Send to Pizza", "Sends the selected player to the pizza delivery activity", 0, false};
}
