#include "core/commands/FloatCommand.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	static FloatCommand _SpeedBlurIntensity{"speedblurintensity", "Speed Blur Intensity", "Controls the strength of the speed blur effect", 0.1f, 3.0f, 1.0f};

	class SpeedBlur : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			if (!Self::GetVehicle() || Self::GetVehicle().GetSpeed() < 10.0f)
			{
				GRAPHICS::SET_TIMECYCLE_MODIFIER_STRENGTH(0.0f);
				return;
			}

			const float maxIntensity = _SpeedBlurIntensity.GetState();
			const float intensity = std::min(((Self::GetVehicle().GetSpeed() - 10.0f) / 50.0f) * maxIntensity, maxIntensity);

			GRAPHICS::SET_TIMECYCLE_MODIFIER("rply_motionblur");
			GRAPHICS::SET_TIMECYCLE_MODIFIER_STRENGTH(intensity);
		}

		virtual void OnDisable() override
		{
			GRAPHICS::CLEAR_TIMECYCLE_MODIFIER();
		}
	};

	static SpeedBlur _SpeedBlur{"speedblur", "Speed Blur", "Enhances the sense of speed with dynamic motion blur"};
}
