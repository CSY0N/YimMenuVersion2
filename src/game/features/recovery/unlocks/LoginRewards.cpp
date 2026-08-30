#include "core/commands/Command.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/pointers/Pointers.hpp"

namespace YimMenu::Features
{
	class EnableLoginRewards : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				FiberPool::Push([] {
					*ScriptGlobal(262145).At(9315).As<int*>() = 1;
					*ScriptGlobal(262145).At(12890).As<int*>() = 1;
					*ScriptGlobal(262145).At(12891).As<int*>() = 1;
					*ScriptGlobal(262145).At(12892).As<int*>() = 1;
					*ScriptGlobal(262145).At(12893).As<int*>() = 1;
					*ScriptGlobal(262145).At(33981).As<int*>() = 1;
					*ScriptGlobal(262145).At(33982).As<int*>() = 1;
					*ScriptGlobal(262145).At(33983).As<int*>() = 1;
					*ScriptGlobal(262145).At(33984).As<int*>() = 1;
					*ScriptGlobal(262145).At(33985).As<int*>() = 1;
					*ScriptGlobal(262145).At(33986).As<int*>() = 1;
					*ScriptGlobal(262145).At(33987).As<int*>() = 1;
					*ScriptGlobal(262145).At(33988).As<int*>() = 1;
					*ScriptGlobal(262145).At(33989).As<int*>() = 1;
					*ScriptGlobal(262145).At(33990).As<int*>() = 1;
					*ScriptGlobal(262145).At(33991).As<int*>() = 1;
					*ScriptGlobal(262145).At(33992).As<int*>() = 1;
					*ScriptGlobal(262145).At(33993).As<int*>() = 1;
					*ScriptGlobal(262145).At(33994).As<int*>() = 1;
					*ScriptGlobal(262145).At(33995).As<int*>() = 1;
	                *ScriptGlobal(262145).At(33996).As<int*>() = 1;
					*ScriptGlobal(262145).At(33997).As<int*>() = 1;
					*ScriptGlobal(262145).At(33998).As<int*>() = 1;
					*ScriptGlobal(262145).At(33999).As<int*>() = 1;
					*ScriptGlobal(262145).At(34000).As<int*>() = 1;
					*ScriptGlobal(262145).At(34001).As<int*>() = 1;
					*ScriptGlobal(262145).At(34002).As<int*>() = 1;
					*ScriptGlobal(262145).At(34003).As<int*>() = 1;
					*ScriptGlobal(262145).At(34004).As<int*>() = 1;
					*ScriptGlobal(262145).At(34005).As<int*>() = 1;
					*ScriptGlobal(262145).At(34006).As<int*>() = 1;
					*ScriptGlobal(262145).At(34007).As<int*>() = 1;
					*ScriptGlobal(262145).At(34008).As<int*>() = 1;
					*ScriptGlobal(262145).At(34009).As<int*>() = 1;
					*ScriptGlobal(262145).At(34010).As<int*>() = 1;
					*ScriptGlobal(262145).At(34011).As<int*>() = 1;
					*ScriptGlobal(262145).At(34012).As<int*>() = 1;
					*ScriptGlobal(262145).At(34013).As<int*>() = 1;
					*ScriptGlobal(262145).At(34014).As<int*>() = 1;
					*ScriptGlobal(262145).At(34015).As<int*>() = 1;
					*ScriptGlobal(262145).At(34016).As<int*>() = 1;
					*ScriptGlobal(262145).At(34017).As<int*>() = 1;
					*ScriptGlobal(262145).At(34018).As<int*>() = 1;
					*ScriptGlobal(262145).At(34019).As<int*>() = 1;
					*ScriptGlobal(262145).At(34020).As<int*>() = 1;
					*ScriptGlobal(262145).At(34021).As<int*>() = 1;
					*ScriptGlobal(262145).At(34022).As<int*>() = 1;
					*ScriptGlobal(262145).At(34023).As<int*>() = 1;
					*ScriptGlobal(262145).At(34024).As<int*>() = 1;
					*ScriptGlobal(262145).At(34025).As<int*>() = 1;
					*ScriptGlobal(262145).At(34026).As<int*>() = 1;
					*ScriptGlobal(262145).At(34027).As<int*>() = 1;
		            *ScriptGlobal(262145).At(34028).As<int*>() = 1;
					*ScriptGlobal(262145).At(34029).As<int*>() = 1;
					*ScriptGlobal(262145).At(34030).As<int*>() = 1;
					*ScriptGlobal(262145).At(34031).As<int*>() = 1;
					*ScriptGlobal(262145).At(34032).As<int*>() = 1;
		            *ScriptGlobal(262145).At(34033).As<int*>() = 1;
					*ScriptGlobal(262145).At(34034).As<int*>() = 1;
					*ScriptGlobal(262145).At(34035).As<int*>() = 1;
					*ScriptGlobal(262145).At(34036).As<int*>() = 1;
					*ScriptGlobal(262145).At(34037).As<int*>() = 1;
					*ScriptGlobal(262145).At(34038).As<int*>() = 1;
					*ScriptGlobal(262145).At(34039).As<int*>() = 1;
					*ScriptGlobal(262145).At(34040).As<int*>() = 1;
					*ScriptGlobal(262145).At(34041).As<int*>() = 1;
					*ScriptGlobal(262145).At(34042).As<int*>() = 1;
					*ScriptGlobal(262145).At(34043).As<int*>() = 1;
					*ScriptGlobal(262145).At(34044).As<int*>() = 1;
					*ScriptGlobal(262145).At(34045).As<int*>() = 1;
					*ScriptGlobal(262145).At(34046).As<int*>() = 1;
					*ScriptGlobal(262145).At(34047).As<int*>() = 1;
					*ScriptGlobal(262145).At(34048).As<int*>() = 1;
					*ScriptGlobal(262145).At(34049).As<int*>() = 1;
					*ScriptGlobal(262145).At(34050).As<int*>() = 1;
					*ScriptGlobal(262145).At(34051).As<int*>() = 1;
					*ScriptGlobal(262145).At(34052).As<int*>() = 1;
				});
			}
			else
			{
				Notifications::Show("YimMenuV2", "You must be online", NotificationType::Error);
			}
		}
	};

	static EnableLoginRewards _EnableLoginRewards{"enable_login_rewards", "Enable Login Rewards", "Enables legacy Rockstar login gift rewards"};
}
