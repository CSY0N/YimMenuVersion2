#include "core/commands/Command.hpp"
#include "game/gta/Natives.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/gta/Stats.hpp"

namespace YimMenu::Features
{
	class Unlockfastrun : public Command
	{
		using Command::Command;
		virtual void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				Stats::SetInt("MPX_CHAR_FM_ABILITY_1_UNLCK", -1);
				Stats::SetInt("MPX_CHAR_FM_ABILITY_2_UNLCK ", -1);
				Stats::SetInt("MPX_CHAR_FM_ABILITY_3_UNLCK ", -1);
				Stats::SetInt("MPX_CHAR_ABILITY_1_UNLCK", -1);
				Stats::SetInt("MPX_CHAR_ABILITY_2_UNLCK", -1);
				Stats::SetInt("MPX_CHAR_ABILITY_3_UNLCK", -1);
				STATS::STAT_SAVE(0, 0, 3, 0);
			}
			else
			{
				Notifications::Show("YimMenuV2", "You Must be Online", NotificationType::Error);
			}
		}
	};
	static Unlockfastrun _Unlockfastrun{
	    "unlockfastrun",
	    "Unlock Fast Run",
	    "Unlocks fast run"};
}
