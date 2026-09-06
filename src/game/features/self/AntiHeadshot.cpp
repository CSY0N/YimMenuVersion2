#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class AntiHeadshot : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{

			if (!Self::GetPed())
				return;

			PED::SET_PED_SUFFERS_CRITICAL_HITS(Self::GetPed().GetHandle(), false);
		}

		virtual void OnDisable() override
		{
			if (Self::GetPed())
				PED::SET_PED_SUFFERS_CRITICAL_HITS(Self::GetPed().GetHandle(), true);
		}
	};

	static AntiHeadshot _AntiHeadshot{"antiheadshot", "Anti-Headshot", "Prevents critical headshot damage"};
}
