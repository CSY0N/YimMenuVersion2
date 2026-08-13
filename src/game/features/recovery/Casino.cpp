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

	class CasinoMembershipBonus : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptGlobal(1973325).As<int*>() = 1;
		}
	};
	static ListCommand _LuckyWheelPrize{
	    "luckywheelprize",
	    "Lucky Wheel Prize",
	    "Select the outcome for the Lucky Wheel",
	    {
	        {0, "Clothing"},
	        {1, "2,500 RP"},
	        {2, "$20,000"},
	        {3, "10,000 Chips"},
	        {4, "10% Discount Voucher"},
	        {5, "5,000 RP"},
	        {6, "$30,000"},
	        {7, "15,000 Chips"},
	        {8, "Clothing"},
	        {9, "7,500 RP"},
	        {10, "20,000 Chips"},
	        {11, "Mystery Prize"},
	        {12, "Clothing"},
	        {13, "10,000 RP"},
	        {14, "$40,000"},
	        {15, "25,000 Chips"},
	        {16, "Clothing"},
	        {17, "15,000 RP"},
	        {18, "Podium Vehicle"},
	        {19, "$50,000"},
	    }};
	class SetLuckyWheelPrize : public Command
	{

		using Command::Command;

		virtual void OnCall() override
		{
			if (auto thread = Scripts::FindScriptThread("casino_lucky_wheel"_J))
			{
				const auto player = PLAYER::PLAYER_ID();
				*ScriptGlobal(262145).At(26856).As<bool*>() = true; // Enable Additional Spins
				*ScriptGlobal(262145).At(37458).As<int*>() = 2; // Max. Spins Per Day w/ GTA+
				*ScriptLocal(thread, 150).At(player, 5).As<int*>() = _LuckyWheelPrize.GetState(); // Results
			}
		}
	};


	static SetLuckyWheelPrize _SetLuckyWheelPrize{"setluckywheelprize", "Set Lucky Wheel Prize", "Sets the Lucky Wheel outcome"};
	static CasinoMembershipBonus _CasinoMembershipBonus{"casino_membership_bonus", "Casino Membership Bonus", "Triggers the Casino Membership Bonus."};
	static CasinoManipulateRigSlotMachines _CasinoManipulateRigSlotMachines{"casinomanipulaterigslotmachines", "Manipulate Rig Slot Machines", "Lets you win the Rig Slot Machines every time"};
}
