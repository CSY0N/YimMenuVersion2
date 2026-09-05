#include "core/commands/LoopedCommand.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class ThermalVision : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			GRAPHICS::SET_SEETHROUGH(true);
		}

		virtual void OnDisable() override
		{
			GRAPHICS::SET_SEETHROUGH(false);
		}
	};

	static ThermalVision _ThermalVision{"thermalvision", "Thermal Vision", "Enables thermal vision to highlight players and objects."};
}
