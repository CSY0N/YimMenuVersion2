#include "core/commands/Command.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/gta/ScriptLocal.hpp"
#include "game/gta/Scripts.hpp"

#include <array>
#include <cstddef>
#include <format>

namespace YimMenu::Features
{
	struct StashHouseCombination
	{
		int First;
		int Second;
		int Third;
	};

	static constexpr std::array<StashHouseCombination, 10> g_StashHouseCodes{{
	    {5, 2, 91},
	    {28, 3, 98},
	    {24, 10, 81},
	    {2, 12, 87},
	    {1, 23, 45},
	    {28, 11, 97},
	    {77, 79, 73},
	    {73, 27, 38},
	    {44, 23, 37},
	    {72, 68, 83},
	}};

	static constexpr int STASH_CODE_1 = 153 + 22 + 1 + 1;
	static constexpr int STASH_CODE_2 = 153 + 22 + 3 + 1;
	static constexpr int STASH_CODE_3 = 153 + 22 + 5 + 1;
	static constexpr int STASH_SCREEN_1 = 153 + 22 + 1;
	static constexpr int STASH_SCREEN_2 = 153 + 22 + 3;
	static constexpr int STASH_SCREEN_3 = 153 + 22 + 5;

	class GetStashHouseCode : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			const auto script = Scripts::FindScriptThread("fm_content_stash_house"_J);
			if (!script)
			{
				Notifications::Show("Stash House", "Start a stash house mission first.", NotificationType::Error);
				return;
			}

			const int first = *ScriptLocal(script, STASH_CODE_1).As<int*>();
			const int second = *ScriptLocal(script, STASH_CODE_2).As<int*>();
			const int third = *ScriptLocal(script, STASH_CODE_3).As<int*>();

			int combinationIndex = -1;
			for (std::size_t i = 0; i < g_StashHouseCodes.size(); ++i)
			{
				const auto& code = g_StashHouseCodes[i];

				if (code.First == first && code.Second == second && code.Third == third)
				{
					combinationIndex = static_cast<int>(i);
					break;
				}
			}

			if (combinationIndex == -1)
			{
				Notifications::Show("Stash House", std::format("Unknown combination: {:02}, {:02}, {:02}", first, second, third), NotificationType::Warning);
				return;
			}
			Notifications::Show("Stash House", std::format("Code {}: {:02}, {:02}, {:02}", combinationIndex + 1, first, second, third), NotificationType::Success);
		}
	};

	static GetStashHouseCode _GetStashHouseCode{
	    "getstashhousecode",
	    "Get Stash House Code",
	    "Displays the current stash house safe combination"};
}
