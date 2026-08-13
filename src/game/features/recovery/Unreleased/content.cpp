#include "core/commands/Command.hpp"
#include "game/gta/Natives.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "core/backend/ScriptMgr.hpp"
#include "core/backend/FiberPool.hpp"
#include "core/commands/ListCommand.hpp"


namespace YimMenu::Features
{

	class Unlockcontent : public Command
	{
		using Command::Command;
		virtual void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				*ScriptGlobal(262145).At(37823).As<int*>() = 1; // Bomb Squad, Mansion Raid
				*ScriptGlobal(262145).At(37824).As<int*>() = 1;  // Knockdown
				*ScriptGlobal(262145).At(37825).As<int*>() = 1; // Alpha Mail
				*ScriptGlobal(262145).At(37826).As<int*>() = 1;  // Meteoric, Hot Off The Press
				*ScriptGlobal(262145).At(37639).As<int*>() = 1; // LS Fire Department
				*ScriptGlobal(262145).At(37640).As<int*>() = 1;  // Forklift Operator
				*ScriptGlobal(262145).At(37641).As<int*>() = 1; // Paper Route
				*ScriptGlobal(262145).At(37642).As<int*>() = 1;  // Lucky Clover Event
			}
			else
			{
				Notifications::Show("YimMenuV2", "You Must be Online", NotificationType::Error);
			}
		}
	};
	static Unlockcontent _Unlockcontent{
	    "Unreleased_content",
	    "Unreleased content",
	    "Enable hidden awards on pause menu (Temporarily)"};
	// Lucky Clover Event Locations
	inline constexpr auto luckyCloverLocationsNames = std::to_array({"Location 1", "Location 2", "Location 3", "Location 4", "Location 5", "Location 6", "Location 7", "Location 8", "Location 9", "Location 10", "Location 11", "Location 12", "Location 13", "Location 14", "Location 15", "Location 16", "Location 17", "Location 18", "Location 19", "Location 20", "Location 21", "Location 22", "Location 23", "Location 24", "Location 25"});

	 inline const std::vector<Vector3> luckyCloverCoords = {
	    {-170.894f, 6467.136f, 37.548f},
	    {1945.482f, 4660.073f, 44.175f},
	    {2237.149f, 5604.759f, 61.283f},
	    {-1042.996f, 4910.542f, 211.853f},
	    {-1592.181f, 2790.916f, 21.219f},
	    {-2974.886f, 390.944f, 17.245f},
	    {2565.692f, 2618.218f, 40.87f},
	    {899.078f, 3617.451f, 34.764f},
	    {-262.125f, 2195.476f, 132.418f},
	    {1428.609f, 1141.87f, 117.035f},
	    {1171.734f, -440.862f, 72.471f},
	    {-1702.923f, -1089.984f, 17.468f},
	    {-175.132f, 970.152f, 239.148f},
	    {-1006.209f, 236.567f, 68.424f},
	    {48.41f, -263.682f, 52.892f},
	    {1156.125f, -1668.12f, 36.675f},
	    {368.398f, -2006.965f, 25.061f},
	    {272.907f, -1204.327f, 41.204f},
	    {-993.914f, -568.652f, 27.752f},
	    {-959.197f, -2763.485f, 16.089f},
	    {-1506.016f, 1500.528f, 116.604f},
	    {2568.694f, 306.722f, 113.423f},
	    {414.393f, 5572.641f, 774.887f},
	    {-887.416f, -1158.412f, 15.053f},
	    {1552.086f, 2187.34f, 89.276f}};

	static std::vector<std::pair<int, const char*>> g_LuckyCloverList{};

	static void BuildLuckyCloverList()
	{
		if (!g_LuckyCloverList.empty())
			return;

		for (int i = 0; i < (int)luckyCloverLocationsNames.size(); ++i)
			g_LuckyCloverList.emplace_back(i, luckyCloverLocationsNames[i]);
	}

	static auto& GetLuckyCloverList()
	{
		BuildLuckyCloverList();
		return g_LuckyCloverList;
	}

	class LuckyCloverLocationCommand : public ListCommand
	{
	public:
		LuckyCloverLocationCommand() :
		    ListCommand(
		        "luckyclover_location",
		        "Lucky Clover Location",
		        "Select a Lucky Clover location to teleport",
		        GetLuckyCloverList(),
		        0)
		{
		}
	};
	static LuckyCloverLocationCommand _LuckyCloverLocation;

	class TeleportLuckyCloverCommand : public Command
	{
		using Command::Command;
		void OnCall() override
		{
			FiberPool::Push([] {
				int idx = _LuckyCloverLocation.GetState();
				if (idx < 0 || idx >= (int)luckyCloverCoords.size())
					return;

				Vector3 coords = luckyCloverCoords[idx];
				PED::SET_PED_COORDS_KEEP_VEHICLE(
				    PLAYER::PLAYER_PED_ID(),
				    coords.x,
				    coords.y,
				    coords.z);
			});
		}
	};
	static TeleportLuckyCloverCommand _TeleportLuckyClover{
	    "teleport_luckyclover",
	    "Teleport",
	    "Teleport to the selected Lucky Clover location"};
}
