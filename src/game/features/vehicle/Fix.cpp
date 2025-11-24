#include "core/commands/Command.hpp"
#include "game/backend/Self.hpp"

namespace YimMenu::Features
{
	class RepairVehicle : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			if (Self::GetVehicle())
				Self::GetVehicle().Fix();
		}
	};

	static RepairVehicle _RepairVehicle{"repairvehicle", "Repair Vehicle", "Fixes any damage to your current vehicle"};
}