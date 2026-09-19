#include "core/commands/ListCommand.hpp"
#include "game/commands/PlayerCommand.hpp"
#include "types/script/ScriptEvent.hpp"

namespace YimMenu::Features
{
    static std::vector<std::pair<int, const char*>> g_Missions{
        {0, "Weather Mission"},
        {2, "Exit Strat Mission"},
        {4, "Cover Blown Mission"},
        {5, "Mole Hunt Mission"},
        {6, "Data Breach Mission"},
        {7, "Work Dispute Mission"}
    };

    static ListCommand _MissionType{"forcemissiontype", "Mission", "Select the mission", g_Missions, 0};

    class ForceMission : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        void OnCall(Player player) override
        {
            if (!player.IsValid())
                return;

            SCRIPT_EVENT_FORCE_MISSION message;

            message.Mission = _MissionType.GetState();
            message.SetPlayer(player.GetId());
            message.Send();
        }
    };

    static ForceMission _ForceMission{"forcemission", "Force Into Mission", "Forces the selected player into the selected mission", 0, false};
}
