#include "core/commands/Command.hpp"
#include "game/gta/ScriptLocal.hpp"
#include "game/gta/ScriptGlobal.hpp"

namespace YimMenu::Features
{
	class Bunkeri_Sell : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptLocal("gb_gunrunning"_J, 1268).At(774).As<int*>() = 0;
			// High Demand Bonus
			*ScriptGlobal(262145).At(21232).As<float*>() = 2.5f;
			*ScriptGlobal(262145).At(21233).As<float*>() = 20.0f;
		}
	};
	static Bunkeri_Sell _Bunkeri_Sell{"Bunker_Instant-Sell","Instant-Finish Bunker","Instant-Finish"};
}
