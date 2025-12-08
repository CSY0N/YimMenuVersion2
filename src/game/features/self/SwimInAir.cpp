#include "core/commands/LoopedCommand.hpp"
#include "core/commands/FloatCommand.hpp"
#include "game/gta/Natives.hpp"
#include "game/backend/Self.hpp"

namespace YimMenu::Features
{
	class SwimInAir : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;
		//Basic needs improved
		virtual void OnTick() override
		{
			PED::SET_PED_CONFIG_FLAG(PLAYER::PLAYER_PED_ID(), 65, true);
		}
		virtual void OnDisable() override
		{
			PED::SET_PED_CONFIG_FLAG(PLAYER::PLAYER_PED_ID(), 65, true);
		}
	};
	static SwimInAir _SwimInAir{
	    "swiminair",
	    "Swim In Air",
	    "Freely move around in the air like you're swimming"};
}
