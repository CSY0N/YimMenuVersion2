#include "core/commands/Command.hpp"
#include "game/gta/Stats.hpp"

namespace YimMenu::Features
{
	class FillVehicleWarehouse : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			for (int i = 32359; i <= 32363; i++)
			{
				Stats::SetPackedBool(i, true);
			}

			Stats::SaveStats();
		}
	};

	static FillVehicleWarehouse _FillVehicleWarehouse{
	    "fillvehwarehouse",
	    "Fill Vehicle Warehouse",
	    "Instantly fills all 5 Import/Export vehicle warehouses."};
}
