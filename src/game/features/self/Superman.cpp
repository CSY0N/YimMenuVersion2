#include "core/commands/LoopedCommand.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class Superman : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;
		virtual void OnTick() override
		{
			Ped ped = PLAYER::PLAYER_PED_ID();
			int left_right = PAD::GET_CONTROL_VALUE(2, 188);
			int up_down = PAD::GET_CONTROL_VALUE(2, 189);
			PED::SET_PED_CAN_RAGDOLL(ped, false);
			PED::SET_PED_CONFIG_FLAG(ped, 60, true);
			Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(ped);
			if (vel.z < 0.0f)
			{
				ENTITY::SET_ENTITY_VELOCITY(
				    ped,
				    vel.x,
				    vel.y,
				    0.0f);
			}
			if (ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(ped) < 3.0f)
			{
				Vector3 coords = ENTITY::GET_ENTITY_COORDS(ped, true);
				coords.z += 80.0f;
				ENTITY::SET_ENTITY_COORDS(ped, coords, false, false, false, false, false, false);
			}
			else
			{
				if (up_down == 254 || (GetAsyncKeyState('S') & 0x8000))
				{
					ENTITY::APPLY_FORCE_TO_ENTITY(
					    ped,
					    1,
					    0.0f,
					    4.0f,
					    9.0f,
					    0.0f,
					    0.0f,
					    0.0f,
					    0,
					    TRUE,
					    TRUE,
					    TRUE,
					    FALSE,
					    TRUE);
				}
				if (up_down == 0 || (GetAsyncKeyState('W') & 0x8000))
				{
					ENTITY::APPLY_FORCE_TO_ENTITY(
					    ped,
					    1,
					    0.0f,
					    4.0f,
					    0.0f,
					    0.0f,
					    0.0f,
					    0.0f,
					    0,
					    TRUE,
					    TRUE,
					    TRUE,
					    FALSE,
					    TRUE);
				}
				if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) || PAD::IS_CONTROL_PRESSED(0, 25))
				{
					ENTITY::APPLY_FORCE_TO_ENTITY(
					    ped,
					    1,
					    0.0f,
					    20.0f,
					    0.5f,
					    0.0f,
					    0.0f,
					    0.0f,
					    0,
					    TRUE,
					    TRUE,
					    TRUE,
					    FALSE,
					    TRUE);
				}
				if (left_right == 254 || (GetAsyncKeyState('D') & 0x8000))
				{
					ENTITY::APPLY_FORCE_TO_ENTITY(
					    ped,
					    1,
					    1.5f,
					    0.0f,
					    0.0f,
					    0.0f,
					    0.1f,
					    0.0f,
					    0,
					    TRUE,
					    TRUE,
					    TRUE,
					    FALSE,
					    TRUE);
				}
				if (left_right == 0 || (GetAsyncKeyState('A') & 0x8000))
				{
					ENTITY::APPLY_FORCE_TO_ENTITY(
					    ped,
					    1,
					    -1.5f,
					    0.0f,
					    0.0f,
					    0.0f,
					    0.1f,
					    0.0f,
					    0,
					    TRUE,
					    TRUE,
					    TRUE,
					    FALSE,
					    TRUE);
				}
			}
		}
		virtual void OnDisable() override
		{
			Ped ped = PLAYER::PLAYER_PED_ID();
			PED::SET_PED_CAN_RAGDOLL(ped, true);
			PED::SET_PED_CONFIG_FLAG(ped, 60, false);
		}
	};
	static Superman _Superman{
	    "superman",
	    "Superman Mode",
	    "Superman Flight"};
}

