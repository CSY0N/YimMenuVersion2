#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/gta/Stats.hpp"

namespace YimMenu::Features
{
	class CollectiblesUnlocks : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				*ScriptGlobal(262145).At(34422).As<int*>() = 1; // Pumpkins Finished
				*ScriptGlobal(262145).At(34393).As<int*>() = 20; // All 20 Ghosts
				*ScriptGlobal(262145).At(35269).As<int*>() = 1; // UFO Event active
				*ScriptGlobal(262145).At(35270).As<int*>() = 1; // Abduction Finished
				Stats::SetInt("XM22_FOOLIGAN_JOBS_COMPLETED", 10);
				Stats::SetBool("XM22_LAB_UNLOCKED", true);
				Stats::SetInt("RADIO_ANTENNA_COLLECTED", 10);
				Stats::SetBool("RADIO_STILL_SLIPPING_UNLOCKED", true);
				Stats::SetBool("AWD_COLLECT_ALL_ACTION_FIGURES", true);
				Stats::SetBool("AWD_COLLECT_ALL_PLAYING_CARDS", true);
				Stats::SetBool("AWD_COLLECT_ALL_SIGNAL_JAMMERS", true);
				Stats::SetBool("AWD_COLLECT_ALL_MOVIE_PROPS", true);
				Stats::SetBool("AWD_COLLECT_ALL_LD_PRODUCTS", true);
				Stats::SetBool("AWD_COLLECT_ALL_SNOWMEN", true);
				Stats::SetInt("CAREER_PROGRESS_TIER_1", 100);
				Stats::SetInt("CAREER_PROGRESS_TIER_2", 100);
				Stats::SetInt("CAREER_PROGRESS_TIER_3", 100);
				Stats::SetInt("CAREER_PROGRESS_TIER_4", 100);
				Stats::SetBool("AWD_GHOSTS_EXPOSED", true);
				Stats::SetBool("AWD_PUMPKIN_COLLECTOR", true);
			}
			else
			{
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
			}
		}
	};

	static CollectiblesUnlocks _CollectiblesUnlocks{
	    "Collectibles_Unlocks",
	    "Collectibles Unlocks",
	    "CollectiblesUnlocks"};
}
