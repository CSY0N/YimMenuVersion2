#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class EnableHiddenLiveries : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				FiberPool::Push([] {
					*ScriptGlobal(262145).At(37402).As<int*>() = 0; // Annis Hardy
					*ScriptGlobal(262145).At(37403).As<int*>() = 0; // Ubermacht Sentinel GTS
					*ScriptGlobal(262145).At(37404).As<int*>() = 0; // Declasse Walton L35 Stock
					*ScriptGlobal(262145).At(37405).As<int*>() = 0; // Declasse Drift Walton L35
					*ScriptGlobal(262145).At(37406).As<int*>() = 0; // Karin Everon RS
					*ScriptGlobal(262145).At(37407).As<int*>() = 0; // Declasse Tampa GT
					*ScriptGlobal(262145).At(37408).As<int*>() = 0; // Annis Minimus
					*ScriptGlobal(262145).At(37409).As<int*>() = 0; // Karin Woodlander
					*ScriptGlobal(262145).At(37410).As<int*>() = 0; // Overflod Suzume
					*ScriptGlobal(262145).At(37411).As<int*>() = 0; // Dewbauchee Rapid GT X
				});
			}
			else
			{
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
			}
		}
	};
	static EnableHiddenLiveries _EnableHiddenLiveries{
	    "enable_hidden_liveries",
	    "Enable Hidden DLC Liveries",
	    "Make Safehouse in the Hills DLC liveries available for purchase"};

}
