#include "core/commands/LoopedCommand.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class NoFallDamage : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;
		virtual void OnTick() override
		{
			Ped ped = PLAYER::PLAYER_PED_ID();
			PED::SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(ped, false);
			PED::SET_PED_CONFIG_FLAG(ped, 32, false);
		}
		virtual void OnDisable() override
		{
			Ped ped = PLAYER::PLAYER_PED_ID();

			PED::SET_PED_CAN_RAGDOLL_FROM_PLAYER_IMPACT(ped, true);
			PED::SET_PED_CONFIG_FLAG(ped, 32, true);
		}
	};
	static NoFallDamage _NoFallDamage{
	    "nofalldamage",
	    "No Fall Damage",
	    "Prevents all damage from high-impact landings"};
}

