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
        "Ocean (Exit)"
    };

    static bool IsKosatkaInOcean()
    {
        const auto status = ScriptGlobal(2658296).At(PLAYER::PLAYER_ID(), 468).At(325).At(4).As<int*>();
        return status && (static_cast<uint32_t>(*status) & (1u << 31)) != 0;
    }

    static bool RequestKosatka()
    {
        auto request = ScriptGlobal(2733326).At(613).As<int*>();

        if (!request)
            return false;

        *request = 1;
        return true;
    }

    static bool OwnsKosatka()
	{
		auto value = ScriptGlobal(1845347).At(PLAYER::PLAYER_ID(), 884).At(260).At(489).As<int*>();
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
        auto misc = std::make_shared<Group>("Misc", 4);
        auto modifiers = std::make_shared<Group>("Modifiers", 1);
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
        misc->AddItem(std::make_shared<BoolCommandItem>("infiniteplasmacutterheat"_J));
        misc->AddItem(std::make_shared<CommandItem>("Reset_Cayo_Perico_cd"_J));
        misc->AddItem(std::make_shared<BoolCommandItem>("removefencingfee"_J));
        misc->AddItem(std::make_shared<BoolCommandItem>("removepavelscut"_J));

        modifiers->AddItem(std::make_shared<IntCommandItem>("bagcapacity"_J));
        modifiers->AddItem(std::make_shared<BoolCommandItem>("bagcapacitymodifier"_J));

        cayo_tp->AddItem(std::make_unique<ImGuiItem>([] {
            ImGui::SetNextItemWidth(140.f);
            ImGui::Combo("Teleport To", &g_SelectedTeleport, g_TeleportItems, IM_ARRAYSIZE(g_TeleportItems));

            if (!ImGui::Button("Teleport", ImVec2(150, 30)))
                return;

            if (g_SelectedTeleport == 0)
            {
                if (!OwnsKosatka())
                {
                    Notifications::Show("Cayo Perico Heist", "You must own the Kosatka to use this teleport.", NotificationType::Error);
                    return;
                }

                if (!IsKosatkaInOcean())
                {
                    if (RequestKosatka())
                        Notifications::Show("Cayo Perico Heist", "Kosatka requested. Wait for it to spawn, then teleport again.", NotificationType::Success);
                    else
                        Notifications::Show("Cayo Perico Heist", "Failed to request Kosatka.", NotificationType::Error);

                    return;
                }
            }

            Vector3 pos{};
            float heading = 0.f;
            bool setHeading = false;

            switch (g_SelectedTeleport)
            {
            case 0:
                pos = {1561.2369f, 385.8831f, -49.689915f};
                heading = 175.f;
                setHeading = true;
                break;

            case 1:
                pos = {5044.001f, -5815.6426f, -11.808871f};
                break;

            case 2:
                pos = {5053.773f, -5773.2266f, -5.40778f};
                break;

            case 3:
                pos = {5010.12f, -5750.1353f, 28.84334f};
                heading = 325.f;
                setHeading = true;
                break;

            case 4:
                pos = {4990.0386f, -5717.6895f, 19.880217f};
                heading = 50.f;
                setHeading = true;
                break;

            case 5:
                pos = {4771.479f, -6165.737f, -39.079613f};
                break;

            default:
                return;
            }

            if (!ENTITY::DOES_ENTITY_EXIST(PLAYER::PLAYER_PED_ID()))
                return;

            Entity entity = PLAYER::PLAYER_PED_ID();

            if (PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), false))
                entity = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);

            if (!ENTITY::DOES_ENTITY_EXIST(entity))
                return;

            ENTITY::SET_ENTITY_COORDS(entity, pos.x, pos.y, pos.z, false, false, false, true);

            if (setHeading)
                Utils::SetHeading(heading);
        }));

        tab->AddItem(cuts);
        tab->AddItem(setups);
        tab->AddItem(loots);
        tab->AddItem(misc);
        tab->AddItem(modifiers);
        tab->AddItem(cayo_tp);

        return tab;
    }
}
