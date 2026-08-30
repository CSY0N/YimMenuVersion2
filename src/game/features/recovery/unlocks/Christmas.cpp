#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/gta/Natives.hpp"
#include "core/util/Joaat.hpp"

namespace YimMenu::Features
{
	static BOOL bLocal_1810 = 0;
	static BOOL bLocal_1813 = 0;
	inline bool IsXmasWeatherApplied()
	{
		return (*ScriptGlobal(2685153).As<uint32_t*>() & (1 << 21)) != 0; // Bit = 21
	}
	void Sync_Weather()
	{
		if (*ScriptGlobal(4516914).As<int*>())
		{
			if (MISC::GET_NEXT_WEATHER_TYPE_HASH_NAME() != Joaat("XMAS"))
			{
				if (!IsXmasWeatherApplied())
				{
					MISC::SET_OVERRIDE_WEATHER("XMAS");
					bLocal_1810 = true;
					bLocal_1813 = true;
				}
			}
		}
		else
		{
			if (MISC::GET_NEXT_WEATHER_TYPE_HASH_NAME() == Joaat("XMAS"))
				MISC::CLEAR_OVERRIDE_WEATHER();
			bLocal_1810= false;
			bLocal_1813 = false;
		}
		//can add SNOW_FOOTSTEPS if want to
	}
	class EnableChristmasDLC : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				FiberPool::Push([] {
					*ScriptGlobal(262145).At(4413).As<int*>() = 1; // Toggle Snow
					*ScriptGlobal(262145).At(9522).As<int*>() = 0; // Disable Snowballs
					*ScriptGlobal(262145).At(4424).As<int*>() = 1; // Main Xmas Unlock
					*ScriptGlobal(262145).At(9525).As<int*>() = 0; // Disable Xmas Clothing
					*ScriptGlobal(262145).At(9526).As<int*>() = 0; // Disable Xmas Masks
					*ScriptGlobal(262145).At(12785).As<int*>() = 1; // Xmas 2015 Masks
					*ScriptGlobal(262145).At(12786).As<int*>() = 1; // Xmas 2015 Costumes
					*ScriptGlobal(262145).At(12787).As<int*>() = 1; // Xmas 2015 Pyjamas
					*ScriptGlobal(262145).At(13224).As<int*>() = 1; // Xmas 2015 Horns
					*ScriptGlobal(262145).At(13227).As<int*>() = 1; // Xmas 2015 Beast Mask
					*ScriptGlobal(262145).At(19112).As<int*>() = 1; // Xmas 2016 Masks
					*ScriptGlobal(262145).At(19113).As<int*>() = 1; // Xmas 2016 Clothing
					*ScriptGlobal(262145).At(22797).As<int*>() = 1; // Xmas 2017 Masks
					*ScriptGlobal(262145).At(22798).As<int*>() = 1; // Xmas 2017 Clothing
					*ScriptGlobal(262145).At(25592).As<int*>() = 1; // Xmas 2018 Clothing
					Sync_Weather();                                 //Local
				});
			}
			else
			{
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
			}
		}
	};

	static EnableChristmasDLC _EnableChristmasDLC{
	    "enable_christmas_dlc",
	    "Enable Christmas DLC",
	    "Unlock all Christmas content from 2015-2018"};
}
