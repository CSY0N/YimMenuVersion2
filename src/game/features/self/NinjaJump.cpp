#include "core/commands/LoopedCommand.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class NinjaJump : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			MISC::SET_SUPER_JUMP_THIS_FRAME(PLAYER::PLAYER_ID());
			if (PED::IS_PED_JUMPING(PLAYER::PLAYER_PED_ID()))
			{
				if (ENTITY::IS_ENTITY_IN_AIR(PLAYER::PLAYER_PED_ID()))
				{
					ENTITY::SET_ENTITY_ROTATION(PLAYER::PLAYER_PED_ID(), 10, 0, ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID()), 0, 0);
				}
			}
		}
	};
	static NinjaJump _NinjaJump{
	    "ninjajump",
	    "Ninja Jump",
	    "Performs fast double flips with extra launch power"};
}
