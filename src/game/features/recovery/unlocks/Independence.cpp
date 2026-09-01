#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class EnableIndependenceDLC : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				FiberPool::Push([] {
					*ScriptGlobal(262145).At(8506).As<int*>() = 1;  // Enable Independence Pack
					*ScriptGlobal(262145).At(8515).As<int*>() = 0;  // Disable Firework Launcher
					*ScriptGlobal(262145).At(12134).As<int*>() = 1; // Independence Day Masks
					*ScriptGlobal(262145).At(8521).As<int*>() = 1;  // Enable Monster Truck
					*ScriptGlobal(262145).At(8520).As<int*>() = 1;  // Enable Western Sovereign
					*ScriptGlobal(262145).At(8544).As<int*>() = 1;  // P1sswasser Hat
					*ScriptGlobal(262145).At(8545).As<int*>() = 1;  // Benedict
					*ScriptGlobal(262145).At(8546).As<int*>() = 1;  // Jlager
					*ScriptGlobal(262145).At(8547).As<int*>() = 1;  // Patriot
					*ScriptGlobal(262145).At(8548).As<int*>() = 1;  // Blarneys
					*ScriptGlobal(262145).At(8549).As<int*>() = 1;  // Supa Wat Hat
				});
			}
			else
			{
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
			}
		}
	};

	static EnableIndependenceDLC _EnableIndependenceDLC{
	    "enable_independence_dlc",
	    "Enable Independence DLC",
	    "Unlock Independence"};
}
