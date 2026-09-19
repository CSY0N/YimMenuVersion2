#include "core/commands/IntCommand.hpp"
#include "core/commands/Command.hpp"
#include "game/gta/Stats.hpp"
#include "game/gta/Natives.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/backend/Tunables.hpp"

namespace YimMenu::Features
{
	static IntCommand LSCarMeetRankSlider{
	    "lscarmeetrank",
	    "LS Car Meet Rank",
	    "Sets LS Car Meet rank (writes CAR_CLUB_REP).",
	    1,
	    1000,
	    1};

	int GetRepForLscmTier(int tier)
	{
		if (tier <= 1)
			return 0;

		Tunable tunable200("TUNER_CARCLUB_REP_INCREMENT_PER_TIER_200"_J);
		Tunable tunable1000("TUNER_CARCLUB_REP_INCREMENT_PER_TIER_1000"_J);

		if (!tunable200.IsReady() || !tunable1000.IsReady())
			return 0;

		const double increment = static_cast<double>(tunable200.Get<int>()) / 2.0;

		if (tier >= 200)
		{
			const int increment1000 = tunable1000.Get<int>();
			const double mainRep = ((199.0 * 100.0) - 100.0) + ((199.0 - 3.0) * ((increment * (199.0 - 3.0)) + increment));

			return ((tier - 199) * increment1000) + static_cast<int>(std::floor(mainRep + 0.5));
		}

		const double rep = ((static_cast<double>(tier) * 100.0) - 100.0) + ((static_cast<double>(tier) - 3.0) * ((increment * (static_cast<double>(tier) - 3.0)) + increment));
		return static_cast<int>(std::floor(rep + 0.5));
	}

	class ApplyLSCarMeetRank : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			const int rank = std::clamp(
			    LSCarMeetRankSlider.GetState(),
			    1,
			    1000);

			const int rep = GetRepForLscmTier(rank);

			Stats::SetInt("MPX_CAR_CLUB_REP", rep);
			STATS::STAT_SAVE(0, 0, 3, 0);
		}
	};

	static ApplyLSCarMeetRank _ApplyLSCarMeetRank{
	    "applylscarmeetrank",
	    "Apply LS Car Meet Rank",
	    "Applies the selected LS Car Meet rank."};
	//Location Needs Changed 
	class Complete_Challenge : Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			Stats::SetBool("MPX_CARMEET_PV_CHLLGE_CMPLT", true);
			Stats::SetBool("MPX_CARMEET_PV_CLMED", false);
			STATS::STAT_SAVE(0, 0, 3, 0);
			Notifications::Show("YimMenuV2", "Done. If you are in LSCM, re-enter to collect the prize.", NotificationType::Success);
		}
	};
	static Complete_Challenge _Complete_Challenge{
    "Complete_Challenge",
    "Unlock LSCM Prize Ride",
    "Unlocks the LS Car Meet prize ride instantly"};
	//tp
	class TeleportToLSCM : public Command
	{
	 using Command::Command;
    void OnCall() override
    {
        FiberPool::Push([]()
        {
            auto blip = HUD::GET_FIRST_BLIP_INFO_ID(777);
            auto coords = HUD::GET_BLIP_COORDS(blip);
            PED::SET_PED_COORDS_KEEP_VEHICLE(PLAYER::PLAYER_PED_ID(), coords.x, coords.y, coords.z);
        });
    }
};
static TeleportToLSCM _TeleportToLSCM{
    "tp_lscm",
    "Teleport to LS Car Meet",
    "Teleport to the LS Car Meet"};
}
