#include "CayoPericoHeist.hpp"
#include "game/gta/Natives.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/util/utils.hpp"
#include "core/frontend/Notifications.hpp"

namespace YimMenu::Submenus
{
	static int g_SelectedTeleport = 0;

	static const char* g_TeleportItems[] = {
		"Kosatka",
		"Drainage Pipe",
		"Drainage Pipe Checkpoint",
		"El Rubio's Office",
		"Front Gate Exit",
		"Ocean (Exit)",
	};

	static bool OwnsKosatka()
	{
		/*freemode.c*/
		/*func_5530*/
		const int player = PLAYER::PLAYER_ID();
		auto value = ScriptGlobal(1845347).At(player, 884).At(260).At(489).As<int*>();
		if (!value)
			return false;

		return (*value & (1 << 0)) != 0;
	}

	std::shared_ptr<TabItem> RenderCayoPericoHeistMenu()
	{
		auto tab = std::make_shared<TabItem>("Cayo Perico Heist");

		auto cuts = std::make_shared<Group>("Heist Cuts", 2);
		auto setups = std::make_shared<Group>("Heist Setups");
		auto loots = std::make_shared<Group>("Loots", 2);
		auto misc = std::make_shared<Group>("Misc", 1);
		auto cayo_tp = std::make_shared<Group>("Teleport");

		cuts->AddItem(std::make_shared<IntCommandItem>("cayopericoheistcut1"_J));
		cuts->AddItem(std::make_shared<IntCommandItem>("cayopericoheistcut3"_J));
		cuts->AddItem(std::make_shared<IntCommandItem>("cayopericoheistcut2"_J));
		cuts->AddItem(std::make_shared<IntCommandItem>("cayopericoheistcut4"_J));
		cuts->AddItem(std::make_shared<CommandItem>("cayopericoheistforceready"_J));
		cuts->AddItem(std::make_shared<CommandItem>("cayopericoheistsetcuts"_J));

		setups->AddItem(std::make_shared<ListCommandItem>("cayopericoheistdifficulty"_J));
		setups->AddItem(std::make_shared<ListCommandItem>("cayopericoheistprimarytarget"_J));
		setups->AddItem(std::make_shared<ListCommandItem>("cayopericoheistweapon"_J));
		setups->AddItem(std::make_shared<CommandItem>("cayopericoheistsetup"_J));

		loots->AddItem(std::make_shared<IntCommandItem>("cayopericoheistprimarytargetvalue"_J));
		loots->AddItem(std::make_shared<IntCommandItem>("cayopericoheistsecondarytakevalue"_J));
		loots->AddItem(std::make_shared<CommandItem>("cayopericoheistsetprimarytargetvalue"_J, "Set##primarytargetvalue"));
		loots->AddItem(std::make_shared<CommandItem>("cayopericoheistsetsecondarytakevalue"_J, "Set##secondarytakevalue"));

		misc->AddItem(std::make_shared<CommandItem>("cayopericoheistskiphacking"_J));
		misc->AddItem(std::make_shared<CommandItem>("cayopericoheistcutsewer"_J));
		misc->AddItem(std::make_shared<CommandItem>("cayopericoheistcutglass"_J));
		misc->AddItem(std::make_shared<CommandItem>("cayopericoheisttakeprimarytarget"_J));
		misc->AddItem(std::make_shared<CommandItem>("cayopericoheistinstantfinish"_J));

		cayo_tp->AddItem(std::make_unique<ImGuiItem>([] {
			ImGui::SetNextItemWidth(140.f);
			ImGui::Combo("Teleport To", &g_SelectedTeleport, g_TeleportItems, IM_ARRAYSIZE(g_TeleportItems));

			if (ImGui::Button("Teleport", ImVec2(150, 30)))
			{
				if (g_SelectedTeleport == 0 && !OwnsKosatka())
				{
					Notifications::Show("Cayo Perico Heist", "You must own the Kosatka to use this teleport.", NotificationType::Error);

					return;
				}

				Vector3 pos{};

				switch (g_SelectedTeleport)
				{
				case 0: // Kosatka
					pos = {1561.2369f, 385.8831f, -49.689915f};
					Utils::SetHeading(175.f);
					break;

				case 1: // Drainage Pipe
					pos = {5044.001f, -5815.6426f, -11.808871f};
					break;

				case 2: // Drainage Pipe Checkpoint
					pos = {5053.773f, -5773.2266f, -5.40778f};
					break;

				case 3: // El Rubio's Office
					pos = {5010.12f, -5750.1353f, 28.84334f};
					Utils::SetHeading(325.f);
					break;

				case 4: // Front Gate Exit
					pos = {4990.0386f, -5717.6895f, 19.880217f};
					Utils::SetHeading(50.f);
					break;

				case 5: // Ocean (Exit)
					pos = {4771.479f, -6165.737f, -39.079613f};
					break;
				}

				Ped ped = PLAYER::PLAYER_PED_ID();

				if (PED::IS_PED_IN_ANY_VEHICLE(ped, false))
				{
					Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(ped, false);

					ENTITY::SET_ENTITY_COORDS(veh, pos.x, pos.y, pos.z, false, false, false, true);
				}
				else
				{
					ENTITY::SET_ENTITY_COORDS(ped, pos.x, pos.y, pos.z, false, false, false, true);
				}
			}
		}));

		tab->AddItem(cuts);
		tab->AddItem(setups);
		tab->AddItem(loots);
		tab->AddItem(misc);
		tab->AddItem(cayo_tp);

		return tab;
	}
}
