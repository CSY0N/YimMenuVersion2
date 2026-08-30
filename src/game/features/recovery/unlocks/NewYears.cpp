#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/pointers/Pointers.hpp"
#include "core/backend/FiberPool.hpp"

namespace YimMenu::Features
{
	class EnableNewYearsGifts : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				*ScriptGlobal(262145).At(33940).As<bool*>() = true; // 2022
				*ScriptGlobal(262145).At(35826).As<bool*>() = true; // 2023
			}
			else
			{
				Notifications::Show("YimMenuV2","You must be online",NotificationType::Error);
			}
		}
	};

	static EnableNewYearsGifts _EnableNewYearsGifts{"enable_new_years_gifts", "Enable New Years Gifts", "Enables 2022 & 2023 New Years gift tunables"};
}
namespace YimMenu::Features
{
	class EnableLunarNewYear : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				FiberPool::Push([] {
					*ScriptGlobal(262145).At(36741).As<int*>() = 1;
					*ScriptGlobal(262145).At(34048).As<int*>() = 1;
				});
			}
			else
			{
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
			}
		}
	};

	static EnableLunarNewYear _EnableLunarNewYear{"enable_lunar_new_year", "Enable Lunar New Year", "Enables the Lunar New Year Yuanbao collectible event"};
}
