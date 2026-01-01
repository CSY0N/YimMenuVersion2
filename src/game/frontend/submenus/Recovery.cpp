#include "Recovery.hpp"
#include "Recovery/HeistModifier.hpp"
#include "Recovery/StatEditor.hpp"
#include "Recovery/Transactions.hpp"
#include "Recovery/DailyActivities.hpp"
#include "game/frontend/items/Items.hpp"

namespace YimMenu::Submenus
{
	Recovery::Recovery() :
		#define ICON_FA_SACK_DOLLAR "\xef\x93\x80"
	    Submenu::Submenu("Recovery", ICON_FA_SACK_DOLLAR)
	{
		auto main = std::make_shared("Main");
		auto businesses = std::make_shared("Businesses");
		auto casino = std::make_shared("Casino");
		auto unlocks = std::make_shared("Unlocks");
		auto generalUnlocks = std::make_shared("General");
		auto trophies = std::make_shared("Trophies");
		auto skip = std::make_shared("Skipper");

		auto generalGroup = std::make_shared("General");
		auto gunvanGroup = std::make_shared("Gun Van Accessories");
		auto businessGroup = std::make_shared("General");
		auto casinoSlots = std::make_shared("Slot Machines");
		//auto casinoWheel = std::make_shared("Lucky Wheel");
		//auto casinoBlackJack = std::make_shared("Blackjack");
		//auto casinoRoulette = std::make_shared("Roulette");

		generalGroup->AddItem(std::make_shared("playallmissionssolo"_J));
		generalGroup->AddItem(std::make_shared("unlockgtaplus"_J));
		generalGroup->AddItem(std::make_shared("overriderpmultiplier"_J));
		generalGroup->AddItem(std::make_shared("overriderpmultiplier"_J, std::make_shared("rpmultiplierinput"_J)));
		generalGroup->AddItem(std::make_shared("maxskills"_J));
		generalGroup->AddItem(std::make_shared("rpslider"_J, "Character Level"));
		generalGroup->AddItem(std::make_shared("applyrp"_J));
		generalGroup->AddItem(std::make_shared("lscarmeetrank"_J, "LS Car Meet Rank"));
		generalGroup->AddItem(std::make_shared("applylscarmeetrank"_J));

		generalGroup->AddItem(std::make_shared("freechangeappearance"_J));
		generalGroup->AddItem(std::make_shared("nochangeappearancecooldown"_J));
		generalGroup->AddItem(std::make_shared("allowgenderchange"_J));
		generalGroup->AddItem(std::make_shared("peddropcash"_J));
		generalGroup->AddItem(std::make_shared("vehicledropcash"_J));

		gunvanGroup->AddItem(std::make_shared("gunvanslot"_J));

		auto gunvanWeaponRow = std::make_shared("", 3);
		gunvanWeaponRow->AddItem(std::make_shared("gunvanweapon"_J));
		gunvanWeaponRow->AddItem(std::make_shared("setgunvanweapon"_J));
		gunvanWeaponRow->AddItem(std::make_shared("removegunvanweapon"_J));
		gunvanGroup->AddItem(std::move(gunvanWeaponRow));

		auto gunvanLocationRow = std::make_shared("", 2);
		gunvanLocationRow->AddItem(std::make_shared("gunvanlocation"_J));
		gunvanLocationRow->AddItem(std::make_shared("setgunvanlocation"_J));
		gunvanGroup->AddItem(std::move(gunvanLocationRow));

		gunvanGroup->AddItem(std::make_shared("teleportgunvan"_J));

		gunvanGroup->AddItem(std::make_shared("gunvanskins"_J));
		gunvanGroup->AddItem(std::make_shared("gunvanblip"_J));

		businessGroup->AddItem(std::make_shared("businesssafe"_J));
		businessGroup->AddItem(std::make_shared("claimsafeearnings"_J));

		casinoSlots->AddItem(std::make_shared("casinomanipulaterigslotmachines"_J));

		generalUnlocks->AddItem(std::make_shared("unlockallcareerprogress"_J));
		generalUnlocks->AddItem(std::make_shared("unlockalltattoos"_J));
		generalUnlocks->AddItem(std::make_shared("BunkerUnlocks"_J));
		generalUnlocks->AddItem(std::make_shared("unlockallAwards"_J));
		generalUnlocks->AddItem(std::make_shared("unlock_paints"_J));
		generalUnlocks->AddItem(std::make_shared("unlock_packed_bools"_J));
		generalUnlocks->AddItem(std::make_shared("unlock_misc"_J));

		skip->AddItem(std::make_shared("SkipDrDreeContract"_J));
		skip->AddItem(std::make_shared("SkipJenetteTheMutette"_J));
		skip->AddItem(std::make_shared("SkipCluckinBellRaid"_J));

		trophies->AddItem(std::make_shared("unlockallmansiontrophies"_J));

		main->AddItem(generalGroup);
		main->AddItem(gunvanGroup);
		businesses->AddItem(businessGroup);
		casino->AddItem(casinoSlots);
		unlocks->AddItem(generalUnlocks);
		unlocks->AddItem(trophies);
		unlocks->AddItem(skip);

		AddCategory(std::move(main));
		AddCategory(std::move(businesses));
		AddCategory(std::move(casino));
		AddCategory(std::move(unlocks));
		AddCategory(BuildStatEditorMenu());
		AddCategory(BuildTransactionsMenu());
		AddCategory(BuildHeistModifierMenu());
		AddCategory(BuildDailyActivitiesMenu());
	}
}
