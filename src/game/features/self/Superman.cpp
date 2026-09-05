#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"
#include "types/pad/ControllerInputs.hpp"

namespace YimMenu::Features
{
	class Superman : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			if (!Self::GetPed() || PED::IS_PED_IN_ANY_VEHICLE(Self::GetPed().GetHandle(), false))
				return;
			if (ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(Self::GetPed().GetHandle()) < 3.0f)
			{
				WEAPON::GIVE_WEAPON_TO_PED(Self::GetPed().GetHandle(), 0xFBAB5776, -1, true, true);

				auto coords = Self::GetPed().GetPosition();
				coords.z += 100.0f;
				Self::GetPed().SetPosition(coords);

				return;
			}

			if (PAD::GET_CONTROL_VALUE(2, 189) == 254 || (GetAsyncKeyState('S') & 0x8000))
				ENTITY::APPLY_FORCE_TO_ENTITY(Self::GetPed().GetHandle(), 1, 0.0f, 3.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0, false, true, true, true, false);

			if (PAD::GET_CONTROL_VALUE(2, 189) == 0 || (GetAsyncKeyState('W') & 0x8000))
				ENTITY::APPLY_FORCE_TO_ENTITY(Self::GetPed().GetHandle(), 1, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, false, true, true, true, false);

			if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) || PAD::IS_CONTROL_PRESSED(0, static_cast<int>(ControllerInputs::INPUT_VEH_ACCELERATE)))
				ENTITY::APPLY_FORCE_TO_ENTITY(Self::GetPed().GetHandle(), 1, 0.0f, 15.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0, false, true, true, true, false);

			if ( PAD::GET_CONTROL_VALUE(2, 188) == 254 || (GetAsyncKeyState('D') & 0x8000))
				ENTITY::APPLY_FORCE_TO_ENTITY(Self::GetPed().GetHandle(), 1, 1.2f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0, false, true, true, true, false);

			if ( PAD::GET_CONTROL_VALUE(2, 188) == 0 || (GetAsyncKeyState('A') & 0x8000))
				ENTITY::APPLY_FORCE_TO_ENTITY(Self::GetPed().GetHandle(), 1, -1.2f, 0.0f, 0.0f, 0.0f, 0.1f, 0.0f, 0, false, true, true, true, false);
		}
	};

	static Superman _Superman{"superman", "Superman", "Allows you to fly through the air with directional controls."};
}
