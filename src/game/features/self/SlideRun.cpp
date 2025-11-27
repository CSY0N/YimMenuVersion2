#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class SlideRun : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			if (TASK::IS_PED_RUNNING(PLAYER::PLAYER_PED_ID()) || TASK::IS_PED_SPRINTING(PLAYER::PLAYER_PED_ID()))
			{
				ENTITY::APPLY_FORCE_TO_ENTITY(
				    PLAYER::PLAYER_PED_ID(),
				    1,
				    0.0f,
				    1.5f,
				    0.0f,
				    0.0f,
				    0.0f,
				    0.0f,
				    0,
				    TRUE,
				    TRUE,
				    TRUE,
				    FALSE,
				    TRUE
				);
			}
		}
	};
	static SlideRun _SlideRun{
	    "sliderun",
	    "Slide Run",
	    "Break the Speed Limit"};
}

