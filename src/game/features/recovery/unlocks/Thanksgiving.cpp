#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/pointers/Pointers.hpp"
namespace YimMenu::Features
{
    class EnableThanksgiving : public Command
    {
        using Command::Command;

        void OnCall() override
        {
            if (*Pointers.IsSessionStarted)
            {
                FiberPool::Push([] {
                    *ScriptGlobal(262145).At(14787).As<int*>() = 1;
                });
            }
            else
            {
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
            }
        }
    };

    static EnableThanksgiving _EnableThanksgiving{"enable_thanksgiving", "Enable Thanksgiving", "Enables the Thanksgiving event"};
}
