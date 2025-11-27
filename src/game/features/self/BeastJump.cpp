#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class BeastJump : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			if (PAD::IS_CONTROL_JUST_PRESSED(0, 22) && !PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), NULL) && !ENTITY::IS_ENTITY_IN_AIR(PLAYER::PLAYER_PED_ID()))
			{
				TASK::TASK_JUMP(PLAYER::PLAYER_PED_ID(), true, true, true);
			}
		}
	};
	static BeastJump _BeastJump{"beastjump", "Beast Jump", "Extreme forward + vertical jump force"};
}

