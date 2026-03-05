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
	static Towtruck_Mission _Towtruck_Mission{"tow_InstantFinish","Instant-Finish","Instant-Finish Tow Truck Service Mission"};
}
