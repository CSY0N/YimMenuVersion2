#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class VegetableMode : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;
		void ApplyForceToEntity(YimMenu::Entity entity, float x, float y, float z, float offX, float offY, float offZ)
		{
			ENTITY::APPLY_FORCE_TO_ENTITY(entity.GetHandle(), 1, x, y, z, offX, offY, offZ, 0, TRUE, TRUE, TRUE, FALSE, TRUE);
		}
		virtual void OnTick() override
		{
			PED::SET_PED_TO_RAGDOLL(PLAYER::PLAYER_PED_ID(), 10, 10, 0, true, true, true);
			if (GetKeyState(0x57))
				ApplyForceToEntity(PLAYER::PLAYER_PED_ID(), 0, 2, 0, 0, 0, 0);
			if (GetKeyState(0x53))
				ApplyForceToEntity(PLAYER::PLAYER_PED_ID(), 0, -2, 0, 0, 0, 0);
			if (GetKeyState(0x41))
				ApplyForceToEntity(PLAYER::PLAYER_PED_ID(), 2, 0, 0, 0, 0, 0);
			if (GetKeyState(0x44))
				ApplyForceToEntity(PLAYER::PLAYER_PED_ID(), -2, 0, 0, 0, 0, 0);
		}
	};
	static VegetableMode _VegetableMode{
	    "vegetable",
	    "Vegetable Mode",
	    "Uncontrolled airborne tumble mode"};
}

