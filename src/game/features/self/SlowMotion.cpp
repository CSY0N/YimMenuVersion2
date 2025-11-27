#include "core/commands/LoopedCommand.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class SlowMotion : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			MISC::SET_TIME_SCALE(0.3f);
		}
		virtual void OnDisable() override
		{
			MISC::SET_TIME_SCALE(1.0f);
		}
	};
	static SlowMotion _SlowMotion{
	    "slowmotion",
	    "Slow Motion",
	    "Slows down the entire game world for cinematic effects"};
}

