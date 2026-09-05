#include "core/commands/FloatCommand.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"
#include "types/pad/ControllerInputs.hpp"

namespace YimMenu::Features
{
	static FloatCommand _WheelieStrength{"wheeliestrength", "Wheelie Strength", "Controls how strongly the front of the vehicle lifts", 0.1f, 5.0f, 1.0f};

	class WheelieMode : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			if (!Self::GetVehicle() || !PAD::IS_CONTROL_PRESSED(0, static_cast<int>(ControllerInputs::INPUT_VEH_ACCELERATE)))
				return;

			const float strength = _WheelieStrength.GetState();

			ENTITY::APPLY_FORCE_TO_ENTITY(Self::GetVehicle().GetHandle(), 1, 0.0f, 0.0f, 2.5f * strength, 0.0f, 2.0f, 0.0f, 0, false, true, true, false, true);
		}
	};

	static WheelieMode _WheelieMode{"wheeliemode", "Wheelie Mode", "Allows vehicles to perform wheelies while accelerating"};
}
