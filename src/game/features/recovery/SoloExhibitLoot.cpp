#include "core/commands/ListCommand.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/gta/ScriptGlobal.hpp"

namespace YimMenu::Features
{
	static ListCommand _SoloExhibitTarget{
	    "soloexhibittarget",
	    "Exhibit Target",
	    "Select which Level 2 exhibit targets are enabled for solo play",
	    {
	        {0, "All Targets"},
	        {1, "Tall Glass Cases"},
	        {2, "Horizontal Glass Cases"},
	        {3, "Artwork"},
	    },
	    0};

	class SoloExhibitLoot : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		static void EnableTarget(int index)
		{
			const auto entry = ScriptGlobal(4980736).At(29174).At(index, 333);
			entry.At(68).As<int&>()  = 0;
			entry.At(143).As<int&>() = 0;
		}

		static void EnableTallGlassCases()
		{
			EnableTarget(0);
			EnableTarget(1);
		}

		static void EnableHorizontalGlassCases()
		{
			EnableTarget(5);
			EnableTarget(6);
			EnableTarget(7);
		}

		static void EnableArtwork()
		{
			EnableTarget(20);
			EnableTarget(21);
		}

		virtual void OnTick() override
		{
			switch (_SoloExhibitTarget.GetState())
			{
			case 0:
				EnableTallGlassCases();
				EnableHorizontalGlassCases();
				EnableArtwork();
				break;

			case 1:
				EnableTallGlassCases();
				break;

			case 2:
				EnableHorizontalGlassCases();
				break;

			case 3:
				EnableArtwork();
				break;
			}
		}
	};

	static SoloExhibitLoot _SoloExhibitLoot{
	    "soloexhibitloot",
	    "Solo Exhibit Loot",
	    "Enables selected Level 2 exhibit secondary targets for solo play"};
}
