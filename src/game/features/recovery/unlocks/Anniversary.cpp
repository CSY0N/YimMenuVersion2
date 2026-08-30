#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/pointers/Pointers.hpp"
namespace YimMenu::Features
{
	class EnableAnniversaryDLC : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				FiberPool::Push([] {
					*ScriptGlobal(262145).At(35756).As<int*>() = 1;
					*ScriptGlobal(262145).At(35827).As<int*>() = 1;
					*ScriptGlobal(2673276).At(4).At(17).As<int*>() = 1;
				});
			}
			else
			{
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
			}
		}
	};

	static EnableAnniversaryDLC _EnableAnniversaryDLC{"enable_anniversary_dlc", "Enable Anniversary DLC", "Enables Rockstar 25th Anniversary and West Coast Classics 10th Anniversary content"};
}
