#include "core/commands/Command.hpp"
#include "game/backend/Self.hpp"
#include "core/commands/Commands.hpp"

//https://github.com/Tessio/YimMenuV2/tree/fix-unload-crash
namespace YimMenu::Features
{
	class UnloadMenu : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			Commands::Shutdown();
			g_Running = false;
		}
	};

	static UnloadMenu _UnloadMenu{"unloadmenu", "Unload", "Unload YimMenuV2."};
}
