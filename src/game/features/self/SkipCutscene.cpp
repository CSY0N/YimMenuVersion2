#include "core/commands/Command.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class SkipCutscene : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			CUTSCENE::STOP_CUTSCENE_IMMEDIATELY();
		}
	};

	static SkipCutscene _SkipCutscene{"skipcutscene", "Skip Cutscene", "Skips any currently playing cutscene"};
}
