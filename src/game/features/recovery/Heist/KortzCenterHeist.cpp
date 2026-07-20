#include "core/commands/Command.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/commands/ListCommand.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/gta/Stats.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "game/gta/ScriptLocal.hpp"



namespace YimMenu::Features
{
	namespace KortzCenterHeist
	{
		// --- Primary Target ---
		static std::vector<std::pair<int, const char*>> kortzCenterTargets = {
		    {0, "La Dernière Débauche"},
		    {1, "Hare Oneself Think"},
		    {2, "The Downfall of Rome"},
		    {3, "Brother Brother"},
		    {4, "A Cast of Characters"},
		    {5, "Gone To Seed"},
		    {6, "True Love"},
		    {7, "Breathless"},
		    {8, "Consumato"},
		    {9, "I Hear Voices"},
		    {10, "Winter, Nowhere in Particular"},
		    {11, "The Girl With the Pearl Necklace"},
		    {12, "Chat on Fruit"},
		    {13, "Pumpkin"},
		    {14, "Twindifference"},
		    {15, "Stacks Study V"},
		    {16, "I, Fruit"},
		    {17, "To Beat About the Bush"},
		    {18, "In Excess of Success"},
		    {19, "Juiced"},
		    {20, "A Winding Road Home"},
		    {21, "Teckels"},
		    {22, "Trust"},
		    {23, "Until Death"},
		    {24, "What Are Melons?"},
		    {25, "The Outcome of Endeavour"},
		    {26, "Mi O Melee"}
		};
		static ListCommand _KortzCenterPrimaryTarget{"kortzcenterheistprimarytarget", "Primary Target", "Primary target", kortzCenterTargets, 0};

		// --- General Purchases (MPX_K26_GENERAL_BS bits 5-8) ---
		static BoolCommand _KortzCenterGuardRoutes{"kortzcenterheistguardroutes", "Guard Routes", "Guard routes purchased", true};
		static BoolCommand _KortzCenterGlassCutter{"kortzcenterheistglasscutter", "Glass Cutter", "Glass cutter purchased", true};
		static BoolCommand _KortzCenterPowerDrills{"kortzcenterheistpowerdrills", "Power Drills", "Power drills purchased", true};
		static BoolCommand _KortzCenterEMPCharges{"kortzcenterheistempcharges", "EMP Charges", "EMP charges purchased", true};

		// --- Prep Work (MPX_K26_ROBBERY_PROG bits 0-15) ---
		static BoolCommand _KortzCenterScopeOut{"kortzcenterheistscopeout", "Scope Out", "Scope out Kortz Center", true};
		static BoolCommand _KortzCenterAlphaMail{"kortzcenterheistalphamail", "Alpha Mail Disguise", "Alpha mail disguise", true};
		static BoolCommand _KortzCenterHazmat{"kortzcenterheisthazmat", "Hazmat Suit", "Hazmat suit", true};
		static BoolCommand _KortzCenterStaffKeycard{"kortzcenterheiststaffkeycard", "Staff Key Card", "Staff key card", true};
		static BoolCommand _KortzCenterTacticalEquip{"kortzcenterheisttacticalequip", "Tactical Equipment", "Tactical equipment", true};
		static BoolCommand _KortzCenterHackingDevice{"kortzcenterheisthackingdevice", "Hacking Device", "Hacking device", true};
		static BoolCommand _KortzCenterAccessCode{"kortzcenterheistaccesscode", "Access Code", "Access code", true};
		static BoolCommand _KortzCenterUnmarkedWeapons{"kortzcenterheistunmarkedweapons", "Unmarked Weapons", "Unmarked weapons", true};
		static BoolCommand _KortzCenterCaracara{"kortzcenterheistcaracara", "Armored Caracara", "Armored Caracara", true};
		static BoolCommand _KortzCenterAnnihilator{"kortzcenterheistannihilator", "Annihilator Stealth", "Annihilator stealth", true};
		static BoolCommand _KortzCenterManchez{"kortzcenterheistmanchez", "Manchez", "Manchez", true};
		static BoolCommand _KortzCenterPrepEMP{"kortzcenterheistprepemp", "EMP Charges (Prep)", "EMP charges prep", true};
		static BoolCommand _KortzCenterGuardShipments{"kortzcenterheistguardshipments", "Guard Shipments", "Guard shipments", true};
		static BoolCommand _KortzCenterGuardRoutesPrep{"kortzcenterheistguardroutesprep", "Guard Routes (Prep)", "Guard routes prep", true};
		static BoolCommand _KortzCenterGlassCutterPrep{"kortzcenterheistglasscutterprep", "Glass Cutter (Prep)", "Glass cutter prep", true};
		static BoolCommand _KortzCenterPowerDrillsPrep{"kortzcenterheistpowerdrillsprep", "Power Drills (Prep)", "Power drills prep", true};

		// --- Scoping ---
		static BoolCommand _KortzCenterScopeSecondary{"kortzcenterheistscopesecondary", "Secondary Targets", "Scope secondary targets", true};
		static BoolCommand _KortzCenterScopePOI{"kortzcenterheistscopepoi", "Points of Interest", "Scope points of interest", true};

		// --- Setup ---
		class Setup : public Command
		{
			using Command::Command;

			virtual void OnCall() override
			{
				// GENERAL_BS: start at -1 (all bits set), clear unchecked bits
				int generalBits = -1;
				if (!_KortzCenterGuardRoutes.GetState())   generalBits &= ~(1 << 5);
				if (!_KortzCenterGlassCutter.GetState())   generalBits &= ~(1 << 6);
				if (!_KortzCenterPowerDrills.GetState())   generalBits &= ~(1 << 7);
				if (!_KortzCenterEMPCharges.GetState())    generalBits &= ~(1 << 8);

				// ROBBERY_PROG: start at -1, clear unchecked bits
				int robberyProg = -1;
				if (!_KortzCenterScopeOut.GetState())        robberyProg &= ~(1 << 0);
				if (!_KortzCenterAlphaMail.GetState())       robberyProg &= ~(1 << 1);
				if (!_KortzCenterHazmat.GetState())          robberyProg &= ~(1 << 2);
				if (!_KortzCenterStaffKeycard.GetState())    robberyProg &= ~(1 << 3);
				if (!_KortzCenterTacticalEquip.GetState())   robberyProg &= ~(1 << 4);
				if (!_KortzCenterHackingDevice.GetState())   robberyProg &= ~(1 << 5);
				if (!_KortzCenterAccessCode.GetState())      robberyProg &= ~(1 << 6);
				if (!_KortzCenterUnmarkedWeapons.GetState()) robberyProg &= ~(1 << 7);
				if (!_KortzCenterCaracara.GetState())        robberyProg &= ~(1 << 8);
				if (!_KortzCenterAnnihilator.GetState())     robberyProg &= ~(1 << 9);
				if (!_KortzCenterManchez.GetState())         robberyProg &= ~(1 << 10);
				if (!_KortzCenterPrepEMP.GetState())         robberyProg &= ~(1 << 11);
				if (!_KortzCenterGuardShipments.GetState())  robberyProg &= ~(1 << 12);
				if (!_KortzCenterGuardRoutesPrep.GetState()) robberyProg &= ~(1 << 13);
				if (!_KortzCenterGlassCutterPrep.GetState()) robberyProg &= ~(1 << 14);
				if (!_KortzCenterPowerDrillsPrep.GetState()) robberyProg &= ~(1 << 15);

				// Scoping
				int scopingBS = _KortzCenterScopeSecondary.GetState() ? -1 : 0;
				int poiBS     = _KortzCenterScopePOI.GetState() ? -1 : 0;

				// Write all stats
				Stats::SetInt("MPX_K26_GENERAL_BS", generalBits);
				Stats::SetInt("MPX_K26_GENERAL_BS2", -1);
				Stats::SetInt("MPX_K26_ROBBERY_PROG", robberyProg);
				Stats::SetInt("MPX_K26_HEIST_TARGET", _KortzCenterPrimaryTarget.GetState());
				Stats::SetInt("MPX_K26_SCOPING_BS", scopingBS);
				Stats::SetInt("MPX_K26_POI_BS", poiBS);
			}
		};

		static Setup _KortzCenterSetup{"kortzcenterheistsetup", "Setup", "Sets up Kortz Center heist"};
	}
	class SkipFingerprint : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			if (auto thread = Scripts::FindScriptThread("fm_mission_controller_v3"_J))*ScriptLocal(thread, 26866).As<int*>() = 5;
		}
	};
	static SkipFingerprint _SkipFingerprint{"kortz_skip_fingerprint_hack", "Skip Fingerprint Hacking", "(Computer Room)"};

	class SkipSignalNode : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			if (auto thread = Scripts::FindScriptThread("fm_mission_controller_v3"_J))*ScriptLocal(thread, 27914).As<int*>() = 5;
		}
	};
	static SkipSignalNode _SkipSignalNode{"kortz_skip_Signal_Nodes_hack", "Skip Signal Nodes Hacking", "(Vault Keypad)"};

	class Primarytarget : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			if (auto thread = Scripts::FindScriptThread("fm_mission_controller_v3"_J))
			{
				*ScriptLocal(thread, 29355).At(11).As<int*>() = 10;
				*ScriptLocal(thread, 29355).At(11).As<int*>() = 17;
			}

		}
	};
	static Primarytarget _Primarytarget{"kortz_take_primary", "Take primary target", "Take primary target"};

	class Secondarytarget : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			if (auto thread = Scripts::FindScriptThread("fm_mission_controller_v3"_J))
			{
				*ScriptLocal(thread, 29355).At(11).As<int*>() = 3;
			}
		}
	};
	static Secondarytarget _Secondarytarget{"kortz_take_secondary", "Take secondary target", "Take secondary target"};

	class DisableLaserGrid : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			if (auto thread = Scripts::FindScriptThread("fm_mission_controller_v3"_J))
			{
				if (auto local = ScriptLocal(thread, 70416).As<int*>())
					*local = 4294784;
			}
			if (auto global = ScriptGlobal(1935711).As<int*>())
				*global |= (1 << 0);
		}
	};
	static DisableLaserGrid _disableLaserGrid{"kortz_disablelasergrid", "Disable Laser Grid", "Disables the mission laser grid."};

	class SkipDataCrack : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			if (auto thread = Scripts::FindScriptThread("fm_mission_controller_v3"_J))
			{
				for (int i = 0; i < 8; ++i)
				{
					*ScriptLocal(thread, 1388).At(i, 4).As<int*>() = 1;
				}
			}
		}
	};
	static SkipDataCrack _skipDataCrack{"kortz_skipdatacrack", "Skip Data Crack", "Skips the data crack minigame."};

	class CutGlass : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			if (auto thread = Scripts::FindScriptThread("fm_mission_controller_v3"_J))
			{
				*ScriptLocal(thread, 32855).At(4, 13).At(3).As<float*>() = 100.0f;
			}
		}
	};
	static CutGlass _cutGlass{"kortz_cutglass", "Cut Glass", "Completes the glass cutting stage."};

	class KortzCenterCooldowns : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			*ScriptGlobal(262145).At(38102).As<int*>() = 600;
			*ScriptGlobal(262145).At(38103).As<int*>() = 2880;
		}
	};
	static KortzCenterCooldowns _kortzCenterCooldowns{"kortz_centercooldowns", "KC Cooldowns", "Sets the Kortz Center Heist cooldowns."};

	class KortzCenterPayouts : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			static constexpr int payouts[] = {
			    481250, // 1  - La Dernière Débauche
			    304500, // 2  - Hare Oneself Think
			    305000, // 3  - The Downfall of Rome
			    305500, // 4  - Brother Brother
			    306000, // 5  - A Cast of Characters
			    306500, // 6  - Gone To Seed
			    307000, // 7  - True Love
			    307500, // 8  - Breathless
			    308000, // 9  - Consumato
			    308500, // 10 - I Hear Voices
			    309000, // 11 - Winter, Nowhere in Particular
			    309500, // 12 - The Girl With the Pearl Necklace
			    310000, // 13 - Chat on Fruit
			    310500, // 14 - Pumpkin
			    311000, // 15 - Twindifference
			    311500, // 16 - Stacks Study V
			    312000, // 17 - I, Fruit
			    312500, // 18 - To Beat About the Bush
			    313000, // 19 - In Excess of Success
			    313500, // 20 - Juiced
			    314000, // 21 - A Winding Road Home
			    314500, // 22 - Teckels
			    315000, // 23 - Trust
			    315500, // 24 - Until Death
			    316000, // 25 - What Are Melons?
			    365000, // 26 - The Outcome of Endeavour
			    317000  // 27 - Mi O Melee
			};

			for (std::size_t i = 0; i < std::size(payouts); ++i)
			{
				if (auto payoutGlobal = ScriptGlobal(262145).At(38004).At(static_cast<std::ptrdiff_t>(i + 1)).As<int*>())
				{
					*payoutGlobal = payouts[i];
				}
			}
			if (auto multiplier = ScriptGlobal(262145).At(38199).As<float*>())
				*multiplier = 4.0f;
		}
	};
	static KortzCenterPayouts _KortzCenterPayouts{"kortz_centerpayouts", "Kortz Center Payouts", "Changes all Kortz Center primary target payouts and sets the first sale multiplier to 4x."};

}
