#include "core/commands/Command.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"

namespace YimMenu::Features
{
	class Heal : Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			int maxHealth = Self::GetPed().GetMaxHealth();
			int maxArmour = Self::GetPlayer().GetMaxArmour();

			Self::GetPed().SetHealth(maxHealth);
			Self::GetPed().SetArmour(maxArmour);
		}
	};

	class AutoHeal : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			if (!Self::GetPed())
				return;

			if (Self::GetPed().GetHealth() < Self::GetPed().GetMaxHealth())
				Self::GetPed().SetHealth(Self::GetPed().GetMaxHealth());
		}
	};

	static Heal _Heal{"heal", "Heal", "Restores your health and armour."};
	static AutoHeal _AutoHeal{"autoheal", "Auto Heal", "Automatically restores your health when it decreases."};
}
