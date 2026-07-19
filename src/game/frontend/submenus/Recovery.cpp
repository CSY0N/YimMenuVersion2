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
		auto main = std::make_shared<Category>("Main");
		auto businesses = std::make_shared<Category>("Businesses");
		auto casino = std::make_shared<Category>("Casino");
		auto unlocks = std::make_shared<Category>("Unlocks");
		auto generalUnlocks = std::make_shared<Group>("General");
		auto LSCMGroupe = std::make_shared<Group>("LS Car Meet");
		auto trophies = std::make_shared<Group>("Trophies");
		auto skip = std::make_shared<Group>("Skipper");
		auto Unreleased = std::make_shared<Group>("Unreleased");
		auto requestServices = std::make_shared<Group>("Request Services");
		auto missions = std::make_shared<Group>("Missions");
		auto achievements = std::make_shared<Group>("Unlock Achievements");
		auto cooldown = std::make_shared<Group>("Cooldown");
		auto Safehouseinhills = std::make_shared<Group>(" Safehouse in the Hills");

		auto generalGroup = std::make_shared<Group>("General");
		auto gunvanGroup = std::make_shared<Group>("Gun Van");
		auto businessGroup = std::make_shared<Group>("Business");
		auto Bailloffice = std::make_shared<Group>("Bail Office");
		auto Misc_business = std::make_shared<Group>("Misc");
		auto over_lay = std::make_shared<Group>("Overlay");
		auto instantfinish = std::make_shared<Group>("Instant Finish Missions");

		auto casinoSlots = std::make_shared<Group>("Slot Machines");
		auto casinoWheel = std::make_shared<Group>("Lucky Wheel");
		auto casinoMisc = std::make_shared<Group>("Misc");
		//auto casinoBlackJack = std::make_shared<Group>("Blackjack");
		//auto casinoRoulette = std::make_shared<Group>("Roulette");

		auto free = std::make_shared<Group>("Free");
		auto exhibitGroup = std::make_shared<Group>("Exhibit Loot");


		generalGroup->AddItem(std::make_shared<BoolCommandItem>("playallmissionssolo"_J));
		generalGroup->AddItem(std::make_shared<BoolCommandItem>("unlockgtaplus"_J));
		generalGroup->AddItem(std::make_shared<BoolCommandItem>("overriderpmultiplier"_J));
		generalGroup->AddItem(std::make_shared<ConditionalItem>("overriderpmultiplier"_J, std::make_shared<FloatCommandItem>("rpmultiplierinput"_J)));
		generalGroup->AddItem(std::make_shared<CommandItem>("maxskills"_J));
		generalGroup->AddItem(std::make_shared<IntCommandItem>("rpslider"_J, "Character Level"));
		generalGroup->AddItem(std::make_shared<CommandItem>("applyrp"_J));
		generalGroup->AddItem(std::make_shared<CommandItem>("Rockstar_dev_tshirt"_J));
		generalGroup->AddItem(std::make_shared<CommandItem>("Good_Behavior_Bonus"_J));

		generalGroup->AddItem(std::make_shared<BoolCommandItem>("freechangeappearance"_J));
		generalGroup->AddItem(std::make_shared<BoolCommandItem>("nochangeappearancecooldown"_J));
		generalGroup->AddItem(std::make_shared<BoolCommandItem>("allowgenderchange"_J));
		generalGroup->AddItem(std::make_shared<BoolCommandItem>("autocollecttargets"_J));
		generalGroup->AddItem(std::make_shared<BoolCommandItem>("peddropcash"_J));
		generalGroup->AddItem(std::make_shared<BoolCommandItem>("vehicledropcash"_J));

		gunvanGroup->AddItem(std::make_shared<ListCommandItem>("gunvanslot"_J));
		gunvanGroup->AddItem(std::make_shared<ListCommandItem>("gunvanweapon"_J));
		gunvanGroup->AddItem(std::make_shared<CommandItem>("setgunvanweapon"_J));
		gunvanGroup->AddItem(std::make_shared<CommandItem>("removegunvanweapon"_J));
		gunvanGroup->AddItem(std::make_shared<ListCommandItem>("gunvanlocation"_J));
		gunvanGroup->AddItem(std::make_shared<CommandItem>("setgunvanlocation"_J));
		gunvanGroup->AddItem(std::make_shared<CommandItem>("teleportgunvan"_J));
		gunvanGroup->AddItem(std::make_shared<BoolCommandItem>("gunvanskins"_J));
		gunvanGroup->AddItem(std::make_shared<BoolCommandItem>("gunvanblip"_J));

		businessGroup->AddItem(std::make_shared<ListCommandItem>("businesssafe"_J));
		businessGroup->AddItem(std::make_shared<CommandItem>("claimsafeearnings"_J));

	    businessGroup->AddItem(std::make_shared<BoolCommandItem>("businessoverlay"_J));
		businessGroup->AddItem(std::make_shared<ConditionalItem>("businessoverlay"_J, std::make_shared<BoolCommandItem>("showwarehouse"_J)));
		businessGroup->AddItem(std::make_shared<ConditionalItem>("businessoverlay"_J, std::make_shared<BoolCommandItem>("showhangar"_J)));
		businessGroup->AddItem(std::make_shared<ConditionalItem>("businessoverlay"_J, std::make_shared<BoolCommandItem>("showbusinesses"_J)));
		businessGroup->AddItem(std::make_shared<ConditionalItem>("businessoverlay"_J, std::make_shared<BoolCommandItem>("shownightclub"_J)));

		businessGroup->AddItem(std::make_shared<CommandItem>("resupplybusiness"_J));
		businessGroup->AddItem(std::make_shared<CommandItem>("hangerresupply"_J));
		businessGroup->AddItem(std::make_shared<CommandItem>("warehouseresupply"_J));
		businessGroup->AddItem(std::make_shared<CommandItem>("nightclubpopularity"_J));
		businessGroup->AddItem(std::make_shared<CommandItem>("salvageyardpopularity"_J));
		businessGroup->AddItem(std::make_shared<CommandItem>("moneyfrontheatremove"_J));
		businessGroup->AddItem(std::make_shared<BoolCommandItem>("removeresupplycooldown"_J));
		businessGroup->AddItem(std::make_shared<BoolCommandItem>("enforceeasiestmission"_J));

		over_lay->AddItem(std::make_shared<BoolCommandItem>("businessoverlay"_J));
		over_lay->AddItem(std::make_shared<ConditionalItem>("businessoverlay"_J, std::make_shared<BoolCommandItem>("showwarehouse"_J)));
		over_lay->AddItem(std::make_shared<ConditionalItem>("businessoverlay"_J, std::make_shared<BoolCommandItem>("showhangar"_J)));
		over_lay->AddItem(std::make_shared<ConditionalItem>("businessoverlay"_J, std::make_shared<BoolCommandItem>("showbusinesses"_J)));
		over_lay->AddItem(std::make_shared<ConditionalItem>("businessoverlay"_J, std::make_shared<BoolCommandItem>("shownightclub"_J)));

		Bailloffice->AddItem(std::make_shared<ListCommandItem>("bailtarget"_J));
		Bailloffice->AddItem(std::make_shared<CommandItem>("baildeliver"_J));
		Bailloffice->AddItem(std::make_shared<CommandItem>("bailsecure"_J));
		Bailloffice->AddItem(std::make_shared<CommandItem>("baildeliver_mw"_J));
		Bailloffice->AddItem(std::make_shared<CommandItem>("bailsecure_mw"_J));
		Bailloffice->AddItem(std::make_shared<CommandItem>("Skip_Office_Dialogues"_J));

		Misc_business->AddItem(std::make_shared<BoolCommandItem>("mansion_business_boost"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("club_popularity"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("club_payout"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("bail_agent1"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("bail_agent2"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("carwash"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("smokewater"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("helitours"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("salvageyard"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("auto_source"_J));
		Misc_business->AddItem(std::make_shared<CommandItem>("fillvehwarehouse"_J));

		casinoSlots->AddItem(std::make_shared<BoolCommandItem>("casinomanipulaterigslotmachines"_J));
		casinoSlots->AddItem(std::make_shared<CommandItem>("casinoslotswin"_J));
		casinoSlots->AddItem(std::make_shared<CommandItem>("casinoslotslose"_J));

		casinoWheel->AddItem(std::make_shared<ListCommandItem>("casinowheelprize"_J));
		casinoWheel->AddItem(std::make_shared<CommandItem>("casinogivewheelprize"_J));


		casinoMisc->AddItem(std::make_shared<BoolCommandItem>("casinobypass"_J));
		casinoMisc->AddItem(std::make_shared<ListCommandItem>("casinochiplimit"_J));
		casinoMisc->AddItem(std::make_shared<CommandItem>("casinoacquirechips"_J));
		casinoMisc->AddItem(std::make_shared<CommandItem>("casinotradechips"_J));



		generalUnlocks->AddItem(std::make_shared<CommandItem>("unlockallcareerprogress"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("unlockalltattoos"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("BunkerUnlocks"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("unlockallAwards"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("unlock_paints"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("unlock_packed_bools"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("unlock_misc"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("all_Collectibles"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("Unlock_Gunrunning"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("unlockalltradeprices"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("Unlock_Festive_Gifts"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("Collectibles_Unlocks"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("CompleteWSameCrewheist"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("New_Unlock"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("enable_jack_o_latern"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("Lester_services"_J));
		generalUnlocks->AddItem(std::make_shared<CommandItem>("mk2_skins"_J));


		skip->AddItem(std::make_shared<CommandItem>("SkipDrDreeContract"_J));
		skip->AddItem(std::make_shared<CommandItem>("SkipJenetteTheMutette"_J));
		skip->AddItem(std::make_shared<CommandItem>("SkipCluckinBellRaid"_J));
		skip->AddItem(std::make_shared<CommandItem>("SkipOscargunzmanflies"_J));
		skip->AddItem(std::make_shared<CommandItem>("Skiptitanjob"_J));




		trophies->AddItem(std::make_shared<CommandItem>("unlockallmansiontrophies"_J));

		requestServices->AddItem(std::make_shared<CommandItem>("requestmoc"_J));
		requestServices->AddItem(std::make_shared<CommandItem>("requestavenger"_J));
		requestServices->AddItem(std::make_shared<CommandItem>("requestterrorbyte"_J));
		requestServices->AddItem(std::make_shared<CommandItem>("requestkosatka"_J));
		requestServices->AddItem(std::make_shared<CommandItem>("requestacidlab"_J));
		requestServices->AddItem(std::make_shared<CommandItem>("requestacidbike"_J));
		requestServices->AddItem(std::make_shared<CommandItem>("requestammodrop"_J));
		requestServices->AddItem(std::make_shared<CommandItem>("requestbullshark"_J));
		requestServices->AddItem(std::make_shared<CommandItem>("requestairstrike"_J));

		Unreleased->AddItem(std::make_shared<CommandItem>("Unreleased_content"_J));
		Unreleased->AddItem(std::make_shared<ListCommandItem>("luckyclover_location"_J));
		Unreleased->AddItem(std::make_shared<CommandItem>("teleport_luckyclover"_J));

		Safehouseinhills->AddItem(std::make_shared<CommandItem>("enable_hidden_liveries"_J));
		Safehouseinhills->AddItem(std::make_shared<CommandItem>("enable_halloween_dlc"_J));
		Safehouseinhills->AddItem(std::make_shared<CommandItem>("enable_independence_dlc"_J));
		Safehouseinhills->AddItem(std::make_shared<CommandItem>("enable_christmas_dlc"_J));
		Safehouseinhills->AddItem(std::make_shared<CommandItem>("enable_new_years_gifts"_J));
		Safehouseinhills->AddItem(std::make_shared<CommandItem>("enable_valentines_day"_J));
		Safehouseinhills->AddItem(std::make_shared<CommandItem>("enable_limited_items"_J));
		Safehouseinhills->AddItem(std::make_shared<CommandItem>("TongvaM_Unlock"_J));
		Safehouseinhills->AddItem(std::make_shared<CommandItem>("RichmanM_Unlock"_J));
		Safehouseinhills->AddItem(std::make_shared<CommandItem>("VinewoodM_Unlock"_J));

		missions->AddItem(std::make_shared<CommandItem>("alien_egg_mission"_J));

		achievements->AddItem(std::make_shared<CommandItem>("unlock_achievements"_J));

		LSCMGroupe->AddItem(std::make_shared<IntCommandItem>("lscarmeetrank"_J, "LS Car Meet Rank"));
		LSCMGroupe->AddItem(std::make_shared<CommandItem>("applylscarmeetrank"_J));
		LSCMGroupe->AddItem(std::make_shared<CommandItem>("Complete_Challenge"_J));
		LSCMGroupe->AddItem(std::make_shared<CommandItem>("tp_lscm"_J));


		cooldown->AddItem(std::make_shared<CommandItem>("reset_casino_cd"_J));
		cooldown->AddItem(std::make_shared<CommandItem>("reset_vehicle_sell_cd"_J));
		cooldown->AddItem(std::make_shared<CommandItem>("Reset_vehicle_Delivery_cd"_J));

		free->AddItem(std::make_shared<CommandItem>("FKarin_S95"_J));


		instantfinish->AddItem(std::make_shared<CommandItem>("InstantFinishTowTruckServiceMission"_J));
		instantfinish->AddItem(std::make_shared<CommandItem>("instantfinishvehiclerobberies"_J));
		instantfinish->AddItem(std::make_shared<CommandItem>("InstantSellBunkerStuff"_J));
		instantfinish->AddItem(std::make_shared<CommandItem>("sellandbuyspecialcargo"_J));
		instantfinish->AddItem(std::make_shared<CommandItem>("instantaircargosell"_J));
		instantfinish->AddItem(std::make_shared<CommandItem>("mcbusinesssellmissions"_J));


		exhibitGroup->AddItem(std::make_shared<ListCommandItem>("soloexhibittarget"_J, "Target"));
		exhibitGroup->AddItem(std::make_shared<BoolCommandItem>("soloexhibitloot"_J));



		main->AddItem(generalGroup);
		main->AddItem(gunvanGroup);
		main->AddItem(requestServices);
		main->AddItem(missions);
		main->AddItem(achievements);
		main->AddItem(LSCMGroupe);
		main->AddItem(cooldown);
		main->AddItem(free);
		main->AddItem(exhibitGroup);


		businesses->AddItem(businessGroup);
		businesses->AddItem(Bailloffice);
		businesses->AddItem(Misc_business);
		businesses->AddItem(over_lay);

	

		casino->AddItem(casinoSlots);
		casino->AddItem(casinoWheel);
		casino->AddItem(casinoMisc);



		unlocks->AddItem(generalUnlocks);
		unlocks->AddItem(trophies);
		unlocks->AddItem(skip);
		unlocks->AddItem(Unreleased);
		unlocks->AddItem(Safehouseinhills);
		unlocks->AddItem(instantfinish);

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
