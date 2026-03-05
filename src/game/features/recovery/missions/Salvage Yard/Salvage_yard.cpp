#include "core/commands/LoopedCommand.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/gta/Stats.hpp"
#include "game/gta/Natives.hpp"
namespace YimMenu::Features
{
	class SkipPreps : public Command
	{
		using Command::Command;
		virtual void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				Stats::SetInt("MPX_SALV23_FM_PROG", -1);
				STATS::STAT_SAVE(0, 0, 3, 0);
			}
		}
	};
	static SkipPreps _SkipPreps{"Sy_SkipPreps", "Skip Preps", "Skip Preps"};
}
