#include "core/commands/IntCommand.hpp"
#include "core/commands/Command.hpp"
#include "game/gta/Stats.hpp"
#include "game/backend/Players.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "game/gta/ScriptLocal.hpp"
#include "core/backend/ScriptMgr.hpp"
#include "types/script/globals/GlobalPlayerBD.hpp"

namespace YimMenu::Features
{
	class InstantFinish_TowTruckServiceMission : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptLocal("fm_content_tow_truck_work"_J, 1828).At(1).As<int*>() = -1071628608;
			*ScriptLocal("fm_content_tow_truck_work"_J, 1885).At(93).As<int*>() = 3;
		}
	};
	static InstantFinish_TowTruckServiceMission _InstantFinish_TowTruckServiceMission{"InstantFinishTowTruckServiceMission", "Finish Tow Truck Service Mission", "finish tow truck services missions"};
}

namespace YimMenu::Features
{
	class InstantFinish_VehicleRobberies : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			// The Duggan
			*ScriptLocal("fm_content_vehrob_arena"_J, 7953).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_arena"_J, 8073).At(1315).As<int*>() = 1;
			// The Cargo Ship
			*ScriptLocal("fm_content_vehrob_cargo_ship"_J, 7227).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_cargo_ship"_J, 7372).At(1250).As<int*>() = 1;
			// The Podium
			*ScriptLocal("fm_content_vehrob_casino_prize"_J, 9235).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_casino_prize"_J, 9377).At(1259).As<int*>() = 1;
			// The [removed]er
			*ScriptLocal("fm_content_vehrob_police"_J, 9050).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_police"_J, 9183).At(1306).As<int*>() = 1;
			// The McTony
			*ScriptLocal("fm_content_vehrob_submarine"_J, 6259).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_submarine"_J, 6420).At(1160).As<int*>() = 1;
		}
	};
	static InstantFinish_VehicleRobberies _InstantFinish_VehicleRobberies{"instantfinishvehiclerobberies", "Finish Vehicle Robberies Mission", "finish vehicle robberies missions"};
}

namespace YimMenu::Features
{
	class Instant_BunkerInstantSell : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptLocal("gb_gunrunning"_J, 1275).At(774).As<int*>() = 0; // Bunker Instant-Sell
		}
	};
	static Instant_BunkerInstantSell _Instant_BunkerInstantSell{"InstantSellBunkerStuff", "Sell your Bunker Stuff instantly", "sell your bunker stuff instantly"};
}

namespace YimMenu::Features
{
	class InstantBuy_SpecialCargo : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			// Special Cargo
			*ScriptGlobal(262145).At(15592).As<int*>() = 300000; // set to 0 to reset // Buy Cooldown
			// Instant Buy
			*ScriptLocal("gb_contraband_buy"_J, 634).At(5).As<int*>() = 1;
			*ScriptLocal("gb_contraband_buy"_J, 634).At(191).As<int*>() = 6;
			*ScriptLocal("gb_contraband_buy"_J, 634).At(192).As<int*>() = 4;
			*ScriptGlobal(262145).At(15593).As<int*>() = 1800000;               //same deal 0 to reset // Sell Cooldown
			*ScriptLocal("gb_contraband_sell"_J, 576).At(1).As<int*>() = 67230; // Instant Sell
		}
	};
	static InstantBuy_SpecialCargo _InstantBuy_SpecialCargo{"sellandbuyspecialcargo", "Sell and buy Instantly Special Cargo (you need to be in the ceo the sell and buy it)", "sell and buy special cargo"};
}

namespace YimMenu::Features
{
	class Instant_AirCargoInstantSell : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			// Air Cargo Instant-Sell (Air Mission Only!)
			*ScriptLocal("gb_smuggler"_J, 1998).At(1035).As<int*>() = 0;
			*ScriptLocal("gb_smuggler"_J, 1998).At(1078).As<int*>() = 1;
		}
	};
	static Instant_AirCargoInstantSell _Instant_AirCargoInstantSell{"instantaircargosell", "Air Cargo Instant-Sell (Air Mission Only!)", "air cargo instant sell"};
}

namespace YimMenu::Features
{
	class Instant_MCBusinessSellMissions : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptGlobal(262145).At(18451).As<int*>() = 1;
			*ScriptGlobal(262145).At(18495).As<int*>() = 1;
			*ScriptGlobal(262145).At(18497).As<int*>() = 0;
			*ScriptGlobal(262145).At(18499).As<int*>() = 1;
			*ScriptGlobal(262145).At(18501).As<int*>() = 0;
			*ScriptGlobal(262145).At(18503).As<int*>() = 1;
		}
	};
	static Instant_MCBusinessSellMissions _Instant_MCBusinessSellMissions{"mcbusinesssellmissions", "MC Business Sell Missions ", "mc business sell missions"};
}
