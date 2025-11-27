#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class SwimInAir : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			PED::SET_PED_CONFIG_FLAG(PLAYER::PLAYER_PED_ID(), 65, true);
		}
		virtual void OnDisable() override
		{
			PED::SET_PED_CONFIG_FLAG(PLAYER::PLAYER_PED_ID(), 65, false);
		}
	};
	static SwimInAir _SwimInAir{
	    "swiminair",
	    "Swim in Air",
	    "Allows swimming and movement while in the air"};
}
