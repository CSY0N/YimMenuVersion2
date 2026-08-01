#include "core/commands/Command.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "game/gta/ScriptLocal.hpp"
#include "game/gta/Scripts.hpp"

#include <cstdint>

namespace YimMenu::Features
{
	static constexpr int FIB_SAFE_CAPACITY             = 36743;
	static constexpr int BLACK_BOX_NORMAL_PAYOUT       = 36662;
	static constexpr int BLACK_BOX_PRIORITY_PAYOUT     = 36663;
	static constexpr int BRUTE_FORCE_NORMAL_PAYOUT     = 36664;
	static constexpr int BRUTE_FORCE_PRIORITY_PAYOUT   = 36665;
	static constexpr int FINE_ART_NORMAL_PAYOUT        = 36666;
	static constexpr int FINE_ART_PRIORITY_PAYOUT      = 36667;
	static constexpr int BREAKAWAY_NORMAL_PAYOUT       = 36668;
	static constexpr int BREAKAWAY_PRIORITY_PAYOUT     = 36669;

	static void SetGlobalInt(int offset, int value)
	{
		*ScriptGlobal(262145).At(offset).As<int*>() = value;
	}

	static bool FinishFibStage(std::uint32_t scriptHash, int flagsOffset, int finishOffset, const char* stageName)
	{
	
		const auto script = Scripts::FindScriptThread(scriptHash);
		if (!script)
		{
			Notifications::Show("FIB Files", "Start the selected mission first.", NotificationType::Error);
			return false;
		}

		int* flags = ScriptLocal(script, flagsOffset).As<int*>();
		int* finish = ScriptLocal(script, finishOffset).As<int*>();

		if (!flags || !finish)
		{
			Notifications::Show("FIB Files", "Failed to access the mission locals.", NotificationType::Error);
			return false;
		}
		*flags |= (1 << 11);
		*finish = 1;
		Notifications::Show("FIB Files", stageName, NotificationType::Success);

		return true;
	}

	class SetFibFilePayouts : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			SetGlobalInt(FIB_SAFE_CAPACITY, 100000);
			SetGlobalInt(BLACK_BOX_NORMAL_PAYOUT, 155000);
			SetGlobalInt(BLACK_BOX_PRIORITY_PAYOUT, 310000);
			SetGlobalInt(BRUTE_FORCE_NORMAL_PAYOUT, 158000);
			SetGlobalInt(BRUTE_FORCE_PRIORITY_PAYOUT, 316000);
			SetGlobalInt(FINE_ART_NORMAL_PAYOUT, 153000);
			SetGlobalInt(FINE_ART_PRIORITY_PAYOUT, 306000);
			SetGlobalInt(BREAKAWAY_NORMAL_PAYOUT, 150000);
			SetGlobalInt(BREAKAWAY_PRIORITY_PAYOUT, 300000);
			Notifications::Show("FIB Files", "Payout values have been applied.", NotificationType::Success);
		}
	};

	class FinishBlackBoxPrep : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			FinishFibStage("fm_content_hacker_cargo_prep"_J, 5330 + 1, 5436 + 1116, "Black Box preparation finished.");
		}
	};

	class FinishBlackBoxFinale : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			FinishFibStage("fm_content_hacker_cargo_finale"_J, 7657 + 1, 7823 + 1319, "Black Box finale finished.");
		}
	};

	class FinishFineArtPrep : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			FinishFibStage("fm_content_hacker_house_prep"_J, 6960 + 1, 7122 + 1313, "Fine Art preparation finished.");
		}
	};

	class FinishFineArtFinale : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			FinishFibStage("fm_content_hacker_house_finale"_J, 8123 + 1, 8267 + 1192, "Fine Art finale finished.");
		}
	};

	class FinishBreakawayPrep : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			FinishFibStage("fm_content_hacker_whistle_prep"_J, 6508 + 1, 6629 + 999, "Project Breakaway preparation finished.");
		}
	};

	class FinishBreakawayFinale : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			FinishFibStage("fm_content_hacker_whistle_fin"_J, 6616 + 1, 6782 + 1176, "Project Breakaway finale finished.");
		}
	};

	class FinishBruteForcePrep : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			FinishFibStage("fm_content_hacker_zancudo_prep"_J, 5269 + 1, 5387 + 1001, "Brute Force preparation finished.");
		}
	};

	class FinishBruteForceFinale : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			FinishFibStage("fm_content_hacker_zancudo_fin"_J, 8537 + 1, 8657 + 1182, "Brute Force finale finished.");
		}
	};

	static SetFibFilePayouts _SetFibFilePayouts{
	    "setfibfilepayouts",
	    "Set FIB File Payouts",
	    "Applies the default and priority FIB File payouts"};

	static FinishBlackBoxPrep _FinishBlackBoxPrep{
	    "finishblackboxprep",
	    "Finish Black Box Prep",
	    "Instantly finishes the active Black Box preparation"};

	static FinishBlackBoxFinale _FinishBlackBoxFinale{
	    "finishblackboxfinale",
	    "Finish Black Box Finale",
	    "Instantly finishes the active Black Box finale"};

	static FinishFineArtPrep _FinishFineArtPrep{
	    "finishfineartprep",
	    "Finish Fine Art Prep",
	    "Instantly finishes the active Fine Art preparation"};

	static FinishFineArtFinale _FinishFineArtFinale{
	    "finishfineartfinale",
	    "Finish Fine Art Finale",
	    "Instantly finishes the active Fine Art finale"};

	static FinishBreakawayPrep _FinishBreakawayPrep{
	    "finishbreakawayprep",
	    "Finish Breakaway Prep",
	    "Instantly finishes the active Project Breakaway preparation"};

	static FinishBreakawayFinale _FinishBreakawayFinale{
	    "finishbreakawayfinale",
	    "Finish Breakaway Finale",
	    "Instantly finishes the active Project Breakaway finale"};

	static FinishBruteForcePrep _FinishBruteForcePrep{
	    "finishbruteforceprep",
	    "Finish Brute Force Prep",
	    "Instantly finishes the active Brute Force preparation"};

	static FinishBruteForceFinale _FinishBruteForceFinale{
	    "finishbruteforcefinale",
	    "Finish Brute Force Finale",
	    "Instantly finishes the active Brute Force finale"};
}
