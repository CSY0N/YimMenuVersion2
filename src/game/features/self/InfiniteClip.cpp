#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"

namespace YimMenu::Features
{
	class InfiniteClip : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			if (Self::GetPed())
				Self::GetPed().SetInfiniteClip(true);
		}

		virtual void OnDisable() override
		{
			if (Self::GetPed())
				Self::GetPed().SetInfiniteClip(false);
		}
	};

	static InfiniteClip _InfiniteClip{"infiniteclip", "Infinite Clip", "Have an endless clip in your gun"};
}