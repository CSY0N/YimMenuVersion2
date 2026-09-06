#include "core/commands/FloatCommand.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	static FloatCommand _CameraShakeIntensity{"camerashakeintensity", "Camera Shake Intensity", "Controls the strength of the high speed camera shake", 0.1f, 5.0f, 1.0f};

	class CameraShake : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			auto veh = Self::GetVehicle();

			if (!veh || veh.GetSpeed() < 10.0f)
			{
				CAMERA::STOP_GAMEPLAY_CAM_SHAKING(true);
				return;
			}

			const float maxIntensity = _CameraShakeIntensity.GetState();
			const float intensity = std::min(((veh.GetSpeed() - 10.0f) / 40.0f) * maxIntensity, maxIntensity);

			if (!CAMERA::IS_GAMEPLAY_CAM_SHAKING())
				CAMERA::SHAKE_GAMEPLAY_CAM("HAND_SHAKE", intensity);
			else
				CAMERA::SET_GAMEPLAY_CAM_SHAKE_AMPLITUDE(intensity);
		}

		virtual void OnDisable() override
		{
			CAMERA::STOP_GAMEPLAY_CAM_SHAKING(true);
		}
	};

	static CameraShake _CameraShake{"camerashake", "Camera Shake", "Enhances the sense of speed with dynamic camera shake"};
}
