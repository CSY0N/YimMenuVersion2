#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"

namespace YimMenu::Features
{
    class ClearWorld : public Command
    {
        using Command::Command;

        virtual void OnCall() override
        {
            *ScriptGlobal(1690468).As<int*>() = 1; // Peds
            *ScriptGlobal(1690469).As<int*>() = 1; // Vehicles
            *ScriptGlobal(1690470).As<int*>() = 1; // Objects
        }
    };

    static ClearWorld _ClearWorld{
        // Keep the one-shot action separate from TinkerScripts' reversible toggle.
        "clearworldall",
        "Clear World",
        "Clear World (Remove Ped, Vehicles & Objects)"
    };
}
