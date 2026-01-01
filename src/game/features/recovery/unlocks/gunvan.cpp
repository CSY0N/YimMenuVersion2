#include "core/commands/Command.hpp"
#include "core/commands/ListCommand.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/backend/Tunables.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "game/gta/Natives.hpp"
#include "game/gta/ScriptFunction.hpp"

#include 
#include 
#include 

namespace YimMenu::Features
{
	inline constexpr auto allowedGunVanWeapons = std::to_array({
	"WEAPON_KNIFE","WEAPON_NIGHTSTICK","WEAPON_HAMMER","WEAPON_BOTTLE","WEAPON_DAGGER","WEAPON_KNUCKLE","WEAPON_MACHETE","WEAPON_FLASHLIGHT","WEAPON_SWITCHBLADE","WEAPON_BATTLEAXE","WEAPON_POOLCUE","WEAPON_WRENCH","WEAPON_HATCHET","WEAPON_BAT","WEAPON_CROWBAR","WEAPON_STUNROD","WEAPON_PISTOL","WEAPON_PISTOL_MK2","WEAPON_COMBATPISTOL","WEAPON_APPISTOL","WEAPON_PISTOL50","WEAPON_SNSPISTOL","WEAPON_SNSPISTOL_MK2","WEAPON_HEAVYPISTOL","WEAPON_VINTAGEPISTOL","WEAPON_FLAREGUN","WEAPON_MARKSMANPISTOL","WEAPON_REVOLVER","WEAPON_REVOLVER_MK2","WEAPON_DOUBLEACTION","WEAPON_RAYPISTOL","WEAPON_CERAMICPISTOL","WEAPON_NAVYREVOLVER","WEAPON_GADGETPISTOL","WEAPON_STUNGUN_MP",
	"WEAPON_MICROSMG","WEAPON_SMG","WEAPON_SMG_MK2","WEAPON_ASSAULTSMG","WEAPON_COMBATPDW","WEAPON_MACHINEPISTOL","WEAPON_MINISMG","WEAPON_MG","WEAPON_COMBATMG","WEAPON_COMBATMG_MK2","WEAPON_GUSENBERG","WEAPON_RAYCARBINE","WEAPON_ASSAULTRIFLE","WEAPON_ASSAULTRIFLE_MK2","WEAPON_CARBINERIFLE","WEAPON_CARBINERIFLE_MK2","WEAPON_ADVANCEDRIFLE","WEAPON_SPECIALCARBINE","WEAPON_SPECIALCARBINE_MK2","WEAPON_BULLPUPRIFLE","WEAPON_BULLPUPRIFLE_MK2","WEAPON_COMPACTRIFLE","WEAPON_MILITARYRIFLE","WEAPON_HEAVYRIFLE","WEAPON_TACTICALRIFLE",
	"WEAPON_PUMPSHOTGUN","WEAPON_PUMPSHOTGUN_MK2","WEAPON_SAWNOFFSHOTGUN","WEAPON_ASSAULTSHOTGUN","WEAPON_BULLPUPSHOTGUN","WEAPON_MUSKET","WEAPON_HEAVYSHOTGUN","WEAPON_DBSHOTGUN","WEAPON_AUTOSHOTGUN","WEAPON_COMBATSHOTGUN","WEAPON_SNIPERRIFLE","WEAPON_HEAVYSNIPER","WEAPON_HEAVYSNIPER_MK2","WEAPON_MARKSMANRIFLE","WEAPON_MARKSMANRIFLE_MK2","WEAPON_PRECISIONRIFLE","WEAPON_GRENADELAUNCHER","WEAPON_RPG","WEAPON_MINIGUN","WEAPON_HOMINGLAUNCHER","WEAPON_COMPACTLAUNCHER","WEAPON_RAYMINIGUN",
	"WEAPON_EMPLAUNCHER","WEAPON_RAILGUNXM3","WEAPON_TECPISTOL","WEAPON_FIREWORK","WEAPON_BATTLERIFLE","WEAPON_SNOWLAUNCHER"
	});

	inline constexpr auto allowedGunVanSlots =
	    std::to_array({"1","2","3","4","5","6","7","8","9","10"});

	static std::vector> g_SlotList{};
	static std::vector> g_WeaponList{};

	static void BuildLists()
	{
		static bool built = false;
		if (built) return;
		built = true;

		for (int i = 0; i < (int)allowedGunVanSlots.size(); ++i)
			g_SlotList.emplace_back(i, allowedGunVanSlots[i]);

		for (int i = 0; i < (int)allowedGunVanWeapons.size(); ++i)
			g_WeaponList.emplace_back(i, allowedGunVanWeapons[i]);
	}

	static auto& GetSlotList()
	{
		BuildLists();
		return g_SlotList;
	}

	static auto& GetWeaponList()
	{
		BuildLists();
		return g_WeaponList;
	}

	class GunVanSlotCommand : public ListCommand
	{
	public:
		GunVanSlotCommand() :
		    ListCommand("gunvanslot","Gun Van Slot",
		    "Select the gun van slot", GetSlotList(), 0) {}
	};
	static GunVanSlotCommand _GunVanSlot;

	class GunVanWeaponCommand : public ListCommand
	{
	public:
		GunVanWeaponCommand() :
		    ListCommand("gunvanweapon","Gun Van Weapon",
		    "Select the weapon for the gun van", GetWeaponList(), 0) {}
	};
	static GunVanWeaponCommand _GunVanWeapon;

	class ApplyGunVanWeaponCommand : public Command
	{
		using Command::Command;
		void OnCall() override
		{
			FiberPool::Push([] {
				Tunable t{
				    Joaat(("XM22_GUN_VAN_SLOT_WEAPON_TYPE_" +
				            std::to_string(_GunVanSlot.GetState() + 1)).c_str())
				};

				if (t.IsReady())
					t.Set(Joaat(allowedGunVanWeapons[_GunVanWeapon.GetState()]));
			});
		}
	};

	static ApplyGunVanWeaponCommand _ApplyGunVanWeapon{
	    "setgunvanweapon",
	    "Set Gun Van Weapon",
	    "Applies the selected weapon to the selected gun van slot"};

	class RemoveGunVanWeaponCommand : public Command
	{
		using Command::Command;
		void OnCall() override
		{
			FiberPool::Push([] {
				Tunable t{
				    Joaat(("XM22_GUN_VAN_SLOT_WEAPON_TYPE_" +
				            std::to_string(_GunVanSlot.GetState() + 1)).c_str())
				};
				if (t.IsReady())
					t.Set(0);
			});
		}
	};

	static RemoveGunVanWeaponCommand _RemoveGunVanWeapon{
	    "removegunvanweapon",
	    "Remove Weapon",
	    "Removes the weapon from the selected gun van slot"};

	class GunVanSkinsCommand : public BoolCommand
	{
	public:
		GunVanSkinsCommand() :
		    BoolCommand("gunvanskins","Weapon Skins",
		    "Enables the special liveries for Knife and Bat", false) {}

		void OnEnable() override
		{
			Tunable t{1490225691};
			if (t.IsReady())
				t.Set(false);
		}

		void OnDisable() override
		{
			Tunable t{1490225691};
			if (t.IsReady())
				t.Set(true);
		}
	};

	static GunVanSkinsCommand _GunVanSkins;

	class GunVanBlipCommand : public BoolCommand
	{
	public:
		GunVanBlipCommand() :
		    BoolCommand("gunvanblip","Blip Always Visible",
		    "Makes Gun Van blip always visible", false) {}

		void OnEnable() override
		{
			Tunable t{15999531};
			if (t.IsReady())
				t.Set(10000.f);
		}

		void OnDisable() override
		{
			Tunable t{15999531};
			if (t.IsReady())
				t.Set(500.f);
		}
	};

	static GunVanBlipCommand _GunVanBlip;

	inline constexpr auto gunVanLocations = std::to_array({
		"Paleto Bay - 1","Grapeseed - 2","Sandy Shores - 3","Grand Senora - 4",
		"Vinewood Hills - 5","Chumash - 6","Paleto Forest - 7","Zancudo River - 8",
		"Power Station - 9","Lago Zancudo - 10","Grand Senora - 11",
		"El Burro Heights - 12","Murrieta Heights - 13","Elysian Island - 14",
		"Tataviam - 15","La Mesa - 16","Terminal - 17","La Puerta - 18",
		"La Mesa - 19","Del Perro - 20","Magellan Ave - 21",
		"West Vinewood - 22","Downtown Vinewood - 23","Pillbox Hill - 24",
		"Little Seoul - 25","Alamo Sea - 26","North Chumash - 27",
		"Mount Chiliad - 28","Mirror Park - 29","Davis - 30"
	});

	static std::vector> g_LocationList{};

	static void BuildLocationList()
	{
		if (!g_LocationList.empty())
			return;

		for (int i = 0; i < (int)gunVanLocations.size(); ++i)
			g_LocationList.emplace_back(i, gunVanLocations[i]);
	}

	static auto& GetLocationList()
	{
		BuildLocationList();
		return g_LocationList;
	}

	class GunVanLocationCommand : public ListCommand
	{
	public:
		GunVanLocationCommand() :
		    ListCommand(
		        "gunvanlocation",
		        "Gun Van Location",
		        "Select the gun van location",
		        GetLocationList(),
		        0)
		{
		}
	};
	static GunVanLocationCommand _GunVanLocation;

	class SetGunVanLocationCommand : public Command
	{
		using Command::Command;
		void OnCall() override
		{
			FiberPool::Push([] {
				ScriptGlobal gunVanGlobal(2652582);

				gunVanGlobal.At(2706).As() = _GunVanLocation.GetState();

				gunVanGlobal.At(2707).As() = 0;
			});
		}
	};

	static SetGunVanLocationCommand _SetGunVanLocation{
	    "setgunvanlocation",
	    "Set Location",
	    "Sets the gun van location"};

	class TeleportGunVanCommand : public Command
	{
		using Command::Command;
		void OnCall() override
		{
			FiberPool::Push([] {
				static ScriptFunction getGunVanCoords(
				    "freemode"_J,
				    ScriptPointer("GGVC", "2D 00 02 00 00 74 61 ? ? ? 32"));

				auto coords = getGunVanCoords.Call();
				PED::SET_PED_COORDS_KEEP_VEHICLE(
				    PLAYER::PLAYER_PED_ID(),
				    coords.x,
				    coords.y,
				    coords.z);
			});
		}
	};

	static TeleportGunVanCommand _TeleportGunVan{
	    "teleportgunvan",
	    "Teleport",
	    "Teleport to current gun van location"};
}
