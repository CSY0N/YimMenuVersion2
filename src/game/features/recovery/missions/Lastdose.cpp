#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"

namespace YimMenu::Features
{
	class LastdoseH : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptGlobal(262145).At(33924).As<int*>() = 1;
		}
	};
	static LastdoseH _LastdoseH{"Last_hardmode","Last Dose Hard Mode","Forces Last Dose Hard Mode"};
}
