#include "core/commands/Command.hpp"
#include "game/gta/Natives.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/gta/Stats.hpp"

namespace YimMenu::Features
{
	class Titan_job : public Command
	{
		using Command::Command;
		virtual void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				Stats::SetInt("MPX_HACKER24_INST_BS", -1);
				Stats::SetPackedBool(51272, true);
				Stats::SetPackedBool(51265, true);
				Stats::SetPackedBool(51266, true);
				Stats::SetPackedBool(51264, true);
				Stats::SetPackedBool(51268, true);
				Stats::SetPackedBool(51269, true);
				Stats::SetPackedBool(51270, true);
				Stats::SetPackedBool(51271, true);
				Stats::SetPackedBool(51260, true);
				Stats::SetPackedBool(51261, true);
				Stats::SetPackedBool(51262, true);
				Stats::SetPackedBool(51263, true);
				Stats::SaveStats();
			}
			else
			{
				Notifications::Show("YimMenuV2", "You Must be Online", NotificationType::Error);
			}
		}
	};
	static Titan_job _Titan_job{"Skiptitanjob", "Skip titan job setup", "Skip titan job setup."};
}
