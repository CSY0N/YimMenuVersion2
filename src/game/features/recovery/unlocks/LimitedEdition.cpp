#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/pointers/Pointers.hpp"

namespace YimMenu::Features
{
	class EnableLimitedEditionItems : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				for (int i = 0; i < 10; i++)
				{
					*ScriptGlobal(262145).At(2408).At(i).As<float*>() = 1.0f; // amount
					*ScriptGlobal(262145).At(2419).At(i).As<int*>() = 1;     // item id
				}
			}
			else
			{
				Notifications::Show("YimMenuV2","You must be online",NotificationType::Error);
			}
		}
	};

	static EnableLimitedEditionItems _EnableLimitedEditionItems{
	    "enable_limited_items",
	    "Enable Limited Edition Items",
	    "Enable Limited Edition Items"};
}
