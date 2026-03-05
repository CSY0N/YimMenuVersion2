#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"

namespace YimMenu::Features
{
	class FirstdoseH : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptGlobal(262145).At(33923).As<int*>() = 1;
		}
	};
	static FirstdoseH _FirstdoseH{"First_hardmode","First Dose Hard Mode","Forces First Dose Hard Mode"};
}
