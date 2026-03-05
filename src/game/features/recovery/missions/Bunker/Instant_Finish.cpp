#include "core/commands/Command.hpp"
#include "game/gta/ScriptLocal.hpp"

namespace YimMenu::Features
{
	class Bunkeri_Sell : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptLocal("gb_gunrunning"_J, 1268).At(774).As<int*>() = 0;
		}
	};
	static Bunkeri_Sell _Bunkeri_Sell{"Bunker_Instant-Sell","Instant-Finish Bunker","Instant-Finish"};
}
