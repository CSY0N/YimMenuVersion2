#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/pointers/Pointers.hpp"

namespace YimMenu::Features
{
	class EnableValentinesDay : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				*ScriptGlobal(262145).At(12107).As<bool*>() = true; // Weapons
				*ScriptGlobal(262145).At(12108).As<bool*>() = true; // Vehicles
				*ScriptGlobal(262145).At(12109).As<bool*>() = true; // Masks
				*ScriptGlobal(262145).At(12110).As<bool*>() = true; // Hair
				*ScriptGlobal(262145).At(12111).As<bool*>() = true; // Clothing
			}
			else
			{
				Notifications::Show("YimMenuV2","You must be online",NotificationType::Error);
			}
		}
	};
	static EnableValentinesDay _EnableValentinesDay{
	    "enable_valentines_day",
	    "Enable Valentine's Day",
	    "Enable Valentine's Day"};
}
