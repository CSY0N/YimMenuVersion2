#include "Vehicle.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/commands/Commands.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/frontend/submenus/Vehicle/SpawnVehicle.hpp"
#include "Vehicle/VehicleEditor.hpp"
#include "Vehicle/SavedVehicles.hpp"

namespace YimMenu::Submenus
{
	Vehicle::Vehicle() :
		#define ICON_FA_CAR "\xef\x86\xb9"
		Submenu::Submenu("Vehicle", ICON_FA_CAR)
	{
		auto main = std::make_shared<Category>("Main");

		auto globals = std::make_shared<Group>("Globals");
		auto tools = std::make_shared<Group>("Tools", 2);
		auto misc = std::make_shared<Group>("Misc");

		globals->AddItem(std::make_shared<BoolCommandItem>("vehiclegodmode"_J, "Godmode"));
		globals->AddItem(std::make_shared<BoolCommandItem>("keepfixed"_J, "Keep Fixed"));
		globals->AddItem(std::make_shared<BoolCommandItem>("hornboost"_J));
        globals->AddItem(std::make_shared<ConditionalItem>("hornboost"_J, std::make_shared<FloatCommandItem>("hornboostspeed"_J)));
        globals->AddItem(std::make_shared<BoolCommandItem>("camerashake"_J));
		globals->AddItem(std::make_shared<ConditionalItem>("camerashake"_J, std::make_shared<FloatCommandItem>("camerashakeintensity"_J)));
		globals->AddItem(std::make_shared<BoolCommandItem>("speedblur"_J));
        globals->AddItem(std::make_shared<ConditionalItem>("speedblur"_J, std::make_shared<FloatCommandItem>("speedblurintensity"_J)));
		globals->AddItem(std::make_shared<BoolCommandItem>("tornadomode"_J));
        globals->AddItem(std::make_shared<ConditionalItem>("tornadomode"_J, std::make_shared<FloatCommandItem>("tornadostrength"_J)));
		globals->AddItem(std::make_shared<BoolCommandItem>("wheeliemode"_J));
		globals->AddItem(std::make_shared<ConditionalItem>("wheeliemode"_J, std::make_shared<FloatCommandItem>("wheeliestrength"_J)));
		globals->AddItem(std::make_shared<BoolCommandItem>("modifyboostbehavior"_J));
		globals->AddItem(std::make_shared<ConditionalItem>("modifyboostbehavior"_J, std::make_shared<ListCommandItem>("boostbehavior"_J)));
		globals->AddItem(std::make_shared<BoolCommandItem>("autodrive"_J));
		globals->AddItem(std::make_shared<BoolCommandItem>("npcautodrive"_J));
		globals->AddItem(std::make_shared<BoolCommandItem>("autodrivehud"_J));
		auto isAutoDriveEnabled = [] {
			const auto playerAutoDrive = Commands::GetCommand<BoolCommand>("autodrive"_J);
			const auto npcAutoDrive = Commands::GetCommand<BoolCommand>("npcautodrive"_J);
			return (playerAutoDrive && playerAutoDrive->GetState())
			    || (npcAutoDrive && npcAutoDrive->GetState());
		};
		globals->AddItem(std::make_shared<ConditionalItem>(isAutoDriveEnabled, std::make_shared<IntCommandItem>("autodrivespeed"_J)));
		globals->AddItem(std::make_shared<ConditionalItem>(isAutoDriveEnabled, std::make_shared<ListCommandItem>("autodrivestyle"_J)));
		
		tools->AddItem(std::make_shared<CommandItem>("enterlastvehicle"_J));
		tools->AddItem(std::make_shared<CommandItem>("enterpvvehicle"_J));
		tools->AddItem(std::make_shared<CommandItem>("repairvehicle"_J));
		tools->AddItem(std::make_shared<CommandItem>("fixallvehicles"_J));
		tools->AddItem(std::make_shared<CommandItem>("callmechanic"_J));
		tools->AddItem(std::make_shared<CommandItem>("requestpv"_J));
		tools->AddItem(std::make_shared<CommandItem>("despawnpv"_J));
		tools->AddItem(std::make_shared<CommandItem>("deletePV"_J));
		tools->AddItem(std::make_shared<CommandItem>("savepersonalvehicle"_J));

		// Rainbow Paint feature with options
		misc->AddItem(std::make_shared<BoolCommandItem>("rainbowpaint"_J, "Rainbow Paint"));
		misc->AddItem(std::make_shared<ConditionalItem>("rainbowpaint"_J, std::make_shared<ListCommandItem>("rainbowtype"_J, "Paint Type")));
		misc->AddItem(std::make_shared<ConditionalItem>("rainbowpaint"_J, std::make_shared<BoolCommandItem>("rainbowpri"_J, "Primary")));
		misc->AddItem(std::make_shared<ConditionalItem>("rainbowpaint"_J, std::make_shared<BoolCommandItem>("rainbowsec"_J, "Secondary")));
		misc->AddItem(std::make_shared<ConditionalItem>("rainbowpaint"_J, std::make_shared<IntCommandItem>("rainbowspeed"_J, "Speed")));

		// Vehicle Jump feature
		misc->AddItem(std::make_shared<BoolCommandItem>("vehjump"_J, "Vehicle Jump"));

		misc->AddItem(std::make_shared<BoolCommandItem>("speedometer"_J));
		misc->AddItem(std::make_shared<BoolCommandItem>("seatbelt"_J));
		misc->AddItem(std::make_shared<BoolCommandItem>("loudsubwoffer"_J));
		misc->AddItem(std::make_shared<BoolCommandItem>("lowervehiclestance"_J, "Lower Stance"));
		misc->AddItem(std::make_shared<BoolCommandItem>("allowhatsinvehicles"_J));
		misc->AddItem(std::make_shared<BoolCommandItem>("lsccustomsbypass"_J));
		misc->AddItem(std::make_shared<BoolCommandItem>("dlcvehicles"_J));
		misc->AddItem(std::make_shared<BoolCommandItem>("vehicledrift"_J));
		misc->AddItem(std::make_shared<ListCommandItem>("driftvariant"_J,"Drift Variant"));

		main->AddItem(globals);
		main->AddItem(tools);
		main->AddItem(misc);

		AddCategory(std::move(main));
		AddCategory(BuildSpawnVehicleMenu());
		AddCategory(BuildVehicleEditorMenu());
		AddCategory(BuildSavedVehiclesMenu());
	}
}
