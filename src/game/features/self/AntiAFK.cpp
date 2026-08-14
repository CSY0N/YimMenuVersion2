#include "core/commands/LoopedCommand.hpp"
#include "game/gta/ScriptGlobal.hpp"

namespace YimMenu::Features
{
	class NoIdleKick : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			// Borderless / Windowed
			*ScriptGlobal(1677608).At(1158).As<int*>() = 0;
			*ScriptGlobal(262145).At(87).As<int*>() = INT_MAX;
			*ScriptGlobal(262145).At(84).As<int*>() = INT_MAX;
			*ScriptGlobal(262145).At(85).As<int*>() = INT_MAX;
			*ScriptGlobal(262145).At(86).As<int*>() = INT_MAX;
			// Fullscreen
			*ScriptGlobal(1677608).At(1174).As<int*>() = 0;
			*ScriptGlobal(262145).At(8499).As<int*>() = INT_MAX;
			*ScriptGlobal(262145).At(8496).As<int*>() = INT_MAX;
			*ScriptGlobal(262145).At(8497).As<int*>() = INT_MAX;
			*ScriptGlobal(262145).At(8498).As<int*>() = INT_MAX;
		}

		virtual void OnDisable() override
		{
			*ScriptGlobal(262145).At(87).As<int*>() = 120000;
			*ScriptGlobal(262145).At(84).As<int*>() = 300000;
			*ScriptGlobal(262145).At(85).As<int*>() = 600000;
			*ScriptGlobal(262145).At(86).As<int*>() = 900000;
			*ScriptGlobal(262145).At(8499).As<int*>() = 30000;
			*ScriptGlobal(262145).At(8496).As<int*>() = 60000;
			*ScriptGlobal(262145).At(8497).As<int*>() = 90000;
			*ScriptGlobal(262145).At(8498).As<int*>() = 120000;
		}
	};

	static NoIdleKick _NoIdleKick{"noidlekick", "No Idle Kick", "Prevents the game from kicking you for being idle"};
}
