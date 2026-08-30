#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class EnableHalloweenDLC : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				FiberPool::Push([] {
					*ScriptGlobal(262145).At(12113).As<int*>() = 1; // Halloween Heist Masks
					*ScriptGlobal(262145).At(12118).As<int*>() = 1; // Halloween Vehicles
					*ScriptGlobal(262145).At(12119).As<int*>() = 1; // Halloween Masks
					*ScriptGlobal(262145).At(12120).As<int*>() = 1; // Halloween Facepaint
					*ScriptGlobal(262145).At(12122).As<int*>() = 1; // Halloween Bobbleheads
					*ScriptGlobal(262145).At(12128).As<int*>() = 1; // Halloween Clothing
					*ScriptGlobal(262145).At(12130).As<int*>() = 1; // Halloween Horns
					*ScriptGlobal(262145).At(17459).As<int*>() = 1; // Enable Sanctus
				});
			}
			else
			{
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
			}
		}
	};

	static EnableHalloweenDLC _EnableHalloweenDLC{
	    "enable_halloween_dlc",
	    "Enable Halloween DLC",
	    "Unlock Halloween"};
}

namespace YimMenu::Features
{
	class EnableJackOLantern : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				FiberPool::Push([] {
					*ScriptGlobal(262145).At(32518).As<int*>() = 1; // Enable Jack O' Lantern
				});
			}
			else
			{
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
			}
		}
	};

	static EnableJackOLantern _EnableJackOLantern{
	    "enable_jack_o_latern",
	    "Enable Jack O' Lantern",
	    "Jack O' Lantern"};
}
