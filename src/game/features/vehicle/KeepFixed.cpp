#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"

namespace YimMenu::Features
{
	class KeepFixed : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			auto veh = Self::GetVehicle();
			if (veh)
				veh.Fix();
		}
	};

	static KeepFixed _KeepFixed{"keepfixed", "Keep Vehicle Fixed", "Keeps your vehicle clean and fixed"};
}