#include "core/commands/LoopedCommand.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class HideRadar : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			HUD::DISPLAY_RADAR(false);
		}

		virtual void OnDisable() override
		{
			HUD::DISPLAY_RADAR(true);
		}
	};

	static HideRadar _HideRadar{"hideradar", "Hide Radar", "Hides the radar and minimap."};
}
