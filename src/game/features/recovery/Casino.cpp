#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"
#include "game/gta/ScriptLocal.hpp"
#include "game/gta/Stats.hpp"
#include "core/commands/ListCommand.hpp"
#include "game/backend/Tunables.hpp"
#include "core/backend/ScriptMgr.hpp"

#include <set>


namespace YimMenu::Features
{
	class CasinoManipulateRigSlotMachines : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		int slots_random_results_table = 1357; // Static_1381.f_1[][]
		std::set<int> slots_blacklist = {9, 21, 22, 87, 152};
		int spin_state_var = 1675;
		std::set<int> spin_state_whitelist = {8, 14};

		virtual void OnTick() override
		{
			if (Scripts::SafeToModifyFreemodeBroadcastGlobals() && SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH("casino_slots"_J))
			{
				Player casinoSlotsScriptHostPlayer = NETWORK::NETWORK_GET_HOST_OF_SCRIPT("casino_slots", -1, 0);
				auto casinoSlotsScriptHostPlayerId = casinoSlotsScriptHostPlayer.GetId();
				auto selfPlayerId = Self::GetPlayer().GetId();
				if (casinoSlotsScriptHostPlayerId != selfPlayerId)
				{
					Scripts::ForceScriptHost(Scripts::FindScriptThread("casino_slots"_J));
				}
				int* spin_state = ScriptLocal("casino_slots"_J, spin_state_var).As<int*>();


				bool needs_run = false;
				for (int slots_iter = 3; slots_iter <= 196; ++slots_iter)
				{
					if (!slots_blacklist.contains(slots_iter))
					{
						if (*ScriptLocal("casino_slots"_J, slots_random_results_table + slots_iter).As<int*>() != 6)
						{
							needs_run = true;
							break;
						}
					}
				}
				if (needs_run)
				{
					for (int slots_iter = 3; slots_iter <= 196; ++slots_iter)
					{
						if (!slots_blacklist.contains(slots_iter) && spin_state_whitelist.contains(*spin_state))
						{
							int slot_result = 6;
							*ScriptLocal("casino_slots"_J, slots_random_results_table + slots_iter).As<int*>() = slot_result;
						}
					}
				}
			}
		}

virtual void OnDisable() override
		{
			if (Scripts::SafeToModifyFreemodeBroadcastGlobals() && SCRIPT::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH("casino_slots"_J))
			{
				Player casinoSlotsScriptHostPlayer = NETWORK::NETWORK_GET_HOST_OF_SCRIPT("casino_slots", -1, 0);
				auto casinoSlotsScriptHostPlayerId = casinoSlotsScriptHostPlayer.GetId();
				auto selfPlayerId = Self::GetPlayer().GetId();
				if (casinoSlotsScriptHostPlayerId != selfPlayerId)
				{
					Scripts::ForceScriptHost(Scripts::FindScriptThread("casino_slots"_J));
				}

				int* spin_state = ScriptLocal("casino_slots"_J, spin_state_var).As<int*>();
				// waiting for reset until next time using
				while (!spin_state_whitelist.contains(*spin_state))
				{
					ScriptMgr::Yield();
					spin_state = ScriptLocal("casino_slots"_J, spin_state_var).As<int*>();
					if (spin_state == nullptr){
						return;
					}
				}

				for (int slots_iter = 3; slots_iter <= 196; ++slots_iter)
				{
					if (!slots_blacklist.contains(slots_iter))
					{
						int slot_result = 6;
						std::srand(static_cast<unsigned int>(std::time(0)) + slots_iter);
						slot_result = 3 + std ::rand() % 7; // Generates a pseudo random number [3,9] 
						*ScriptLocal("casino_slots"_J, slots_random_results_table + slots_iter).As<int*>() = slot_result;
					}
				}
			}
		}
	};
	static std::vector<std::pair<int, const char*>> wheelPrizes = {
	    {0, "Clothing 1"},
	    {1, "2,500 RP"},
	    {2, "$20,000"},
	    {3, "10,000 Chips"},
	    {4, "Discount"},
	    {5, "5,000 RP"},
	    {6, "$30,000"},
	    {7, "15,000 Chips"},
	    {8, "Clothing 2"},
	    {9, "7,500 RP"},
	    {10, "20,000 Chips"},
	    {11, "Mystery Prize"},
	    {12, "Clothing 3"},
	    {13, "10,000 RP"},
	    {14, "$40,000"},
	    {15, "25,000 Chips"},
	    {17, "15,000 RP"},
	    {18, "Vehicle"},
	    {19, "$50,000"}};

	static ListCommand _CasinoWheelPrize{"casinowheelprize", "Lucky Wheel Prize", "Select prize for lucky wheel", wheelPrizes, 18};

	class CasinoSlotsWin : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			int randomResultTable = 1374; 
			for (int i = 3; i <= 196; i++)
			{
				if (i != 67 && i != 132)
				{
					*ScriptLocal("casino_slots"_J, randomResultTable + i).As<int*>() = 6;
				}
			}
		}
	};

	class CasinoSlotsLose : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			int randomResultTable = 1374; 
			for (int i = 3; i <= 196; i++)
			{
				if (i != 67 && i != 132)
				{
					*ScriptLocal("casino_slots"_J, randomResultTable + i).As<int*>() = 0;
				}
			}
		}
	};

	class CasinoGiveWheelPrize : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			int prize = _CasinoWheelPrize.GetState();
			*ScriptLocal("casino_lucky_wheel"_J, 318).As<int*>() = prize;
			*ScriptLocal("casino_lucky_wheel"_J, 349).As<int*>() = 11; 
		}
	};

	class CasinoBypass : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			Stats::SetInt("MPPLY_CASINO_CHIPS_WON_GD", 0);
			Stats::SetInt("MPPLY_CASINO_CHIPS_WONTIM", 0);
			Stats::SetInt("MPPLY_CASINO_GMBLNG_GD", 0);
			Stats::SetInt("MPPLY_CASINO_BAN_TIME", 0);
			Stats::SetInt("MPPLY_CASINO_CHIPS_PURTIM", 0);
			Stats::SetInt("MPPLY_CASINO_CHIPS_PUR_GD", 0);

			*ScriptGlobal(1972794).As<BOOL*>() = true;

			ScriptMgr::Yield(5000ms);
		}
	};

	static std::vector<std::pair<int, const char*>> chipLimits = {
	    {20000, "20,000 Chips (Standard)"},
	    {50000, "50,000 Chips (Penthouse)"}};

	static ListCommand _CasinoChipLimit{"casinochiplimit", "Chip Purchase Limit", "Select chip purchase limit", chipLimits, 0};

	class CasinoAcquireChips : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			int limit = _CasinoChipLimit.GetState();
			int tunableHash = (limit == 20000) ? 0x7E4B8C1A : 0x7E4B8C1B;
			Tunable buyLimit(tunableHash);
			if (buyLimit.IsReady())
				buyLimit.Set(limit);
		}
	};

	class CasinoTradeChips : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			Tunable tradeLimit(0x2C9F4D8C);
			if (tradeLimit.IsReady())
				tradeLimit.Set(10000000);
		}
	};

	class CasinoMembershipBonus : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptGlobal(1973325).As<int*>() = 1;
		}
	};


	static CasinoSlotsWin _CasinoSlotsWin{"casinoslotswin", "Rig Slot Machines", "Forces slot machines to give jackpot"};
	static CasinoSlotsLose _CasinoSlotsLose{"casinoslotslose", "Lose Slot Machines", "Forces slot machines to lose"};
	static CasinoGiveWheelPrize _CasinoGiveWheelPrize{"casinogivewheelprize", "Give Lucky Wheel Prize", "Gives selected lucky wheel prize"};
	static CasinoBypass _CasinoBypass{"casinobypass", "Bypass Casino Limits", "Bypasses casino purchase and time limits"};
	static CasinoAcquireChips _CasinoAcquireChips{"casinoacquirechips", "Set Chip Purchase Limit", "Sets maximum chips you can buy"};
	static CasinoTradeChips _CasinoTradeChips{"casinotradechips", "Set Chip Trade Limit", "Sets maximum chips you can trade in"};
	static CasinoManipulateRigSlotMachines _CasinoManipulateRigSlotMachines{"casinomanipulaterigslotmachines", "Manipulate Rig Slot Machines", "Lets you win the Rig Slot Machines every time"};
	static CasinoMembershipBonus _CasinoMembershipBonus{"casino_membership_bonus", "Casino Membership Bonus", "Triggers the Casino Membership Bonus."};
}
