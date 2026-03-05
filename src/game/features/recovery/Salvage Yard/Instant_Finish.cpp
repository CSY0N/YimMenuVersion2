#include "core/commands/LoopedCommand.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "game/gta/ScriptLocal.hpp"

namespace YimMenu::Features
{
	class Towtruck_Mission : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			*ScriptLocal("fm_content_tow_truck_work"_J, 1795).At(1).As<int*>() = -1071628608;
			*ScriptLocal("fm_content_tow_truck_work"_J, 1852).At(93).As<int*>() = 3;
		}
	};
	static Towtruck_Mission _Towtruck_Mission{"tow_InstantFinish","Instant-Finish Tow Truck Service Mission","Instant-Finish"};
	class Vehicle_robberies : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			// The Duggan
			*ScriptLocal("fm_content_vehrob_arena"_J, 7914).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_arena"_J, 8034).At(1315).As<int*>() = 1;
			// The Cargo Ship
			*ScriptLocal("fm_content_vehrob_cargo_ship"_J, 7187).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_cargo_ship"_J, 7332).At(1250).As<int*>() = 1;
			// The Podium
			*ScriptLocal("fm_content_vehrob_casino_prize"_J, 9193).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_casino_prize"_J, 9330).At(1259).As<int*>() = 1;
			// The [removed]er
			*ScriptLocal("fm_content_vehrob_police"_J, 9013).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_police"_J, 9146).At(1306).As<int*>() = 1;
			// The McTony
			*ScriptLocal("fm_content_vehrob_submarine"_J, 6220).At(1).As<int*>() = -2145370424;
			*ScriptLocal("fm_content_vehrob_submarine"_J, 6358).At(1160).As<int*>() = 1;

		}
	};
	static Vehicle_robberies _Vehicle_robberies{"vehrob_InstantFinish","Vehicle Robberies Instant-Finish","Instant-Finish"};
}
