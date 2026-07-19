#include "core/commands/Command.hpp"
#include "game/gta/Natives.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/gta/Stats.hpp"
#include "core/backend/ScriptMgr.hpp"
#include "game/gta/data/Stats.h"



namespace YimMenu::Features
{
	class UnlockCareerProgress : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{

		// OSCAR GUZMAN FLIES AGAIN
	    std::make_shared<PackedBoolStat>(51280),
	    std::make_shared<PackedBoolStat>(51285),
	    std::make_shared<PackedBoolStat>(51278),
	    std::make_shared<PackedBoolRangeStat>(51286, 51291),
	    std::make_shared<IntStat>("MPX_PROG_HUB_MFH_EARNINGS", 5000000),
	    std::make_shared<PackedBoolRangeStat>(51292, 51297),
	    std::make_shared<PackedBoolStat>(51279),

	    // THE CLUCKIN' BELL FARM RAID
	    std::make_shared<PackedBoolStat>(28272),
	    std::make_shared<PackedBoolStat>(28287),
	    std::make_shared<PackedBoolStat>(28286),
	    std::make_shared<PackedBoolStat>(28285),
	    std::make_shared<PackedBoolRangeStat>(28283, 28284),
	    std::make_shared<IntStat>("MPX_PROG_HUB_CBR_EARNINGS", 5000000),
	    std::make_shared<PackedBoolStat>(28282),
	    std::make_shared<PackedBoolRangeStat>(28273, 28281),

	    // SAN ANDREAS MERCENARIES
	    std::make_shared<IntBitsStat>("MPX_SUM23_AVOP_PROGRESS", 0, 11),
	    std::make_shared<PackedBoolStat>(41566),
	    std::make_shared<PackedBoolStat>(41706),
	    std::make_shared<PackedBoolRangeStat>(41567, 41572),
	    std::make_shared<PackedBoolRangeStat>(41573, 41578),
	    std::make_shared<PackedBoolRangeStat>(41579, 41584),
	    std::make_shared<PackedBoolRangeStat>(41585, 41590),

	    // OPERATION PAPER TRAIL
	    std::make_shared<IntBitsStat>("MPX_ULP_MISSION_PROGRESS", 0, 13),
	    std::make_shared<PackedBoolRangeStat>(41333, 41338),
	    std::make_shared<PackedBoolStat>(41707),
	    std::make_shared<PackedBoolRangeStat>(41339, 41344),
	    std::make_shared<PackedBoolRangeStat>(41345, 41350),
	    std::make_shared<PackedBoolRangeStat>(41351, 41356),
	    std::make_shared<PackedBoolRangeStat>(41357, 41362),

	    // A SUPERYATCH LIFE
	    std::make_shared<IntBitsStat>("MPX_YACHT_MISSION_FLOW", 4, 15),
	    std::make_shared<PackedBoolStat>(41379),
	    std::make_shared<PackedBoolStat>(41708),
	    std::make_shared<PackedBoolRangeStat>(41380, 41385),
	    std::make_shared<PackedBoolRangeStat>(41386, 41391),
	    std::make_shared<PackedBoolRangeStat>(41392, 41397),
	    std::make_shared<PackedBoolRangeStat>(41398, 41403),

	    // GERALD'S LAST PLAY
	    std::make_shared<PackedBoolRangeStat>(41404, 41409),
	    std::make_shared<PackedBoolStat>(41410),
	    std::make_shared<PackedBoolStat>(41709),
	    std::make_shared<PackedBoolRangeStat>(41411, 41416),
	    std::make_shared<PackedBoolRangeStat>(41417, 41422),
	    std::make_shared<PackedBoolRangeStat>(41423, 41428),
	    std::make_shared<PackedBoolRangeStat>(41429, 41434),

	    // PREMIUM DELUXE REPO WORK
	    std::make_shared<PackedBoolRangeStat>(41436, 41443),
	    std::make_shared<PackedBoolStat>(41435),
	    std::make_shared<PackedBoolStat>(41705),
	    std::make_shared<PackedBoolRangeStat>(41444, 41451),
	    std::make_shared<PackedBoolRangeStat>(41452, 41459),
	    std::make_shared<PackedBoolRangeStat>(41460, 41467),
	    std::make_shared<PackedBoolRangeStat>(41468, 41475),

	    // MADRAZO DISPATCH SERVICES
	    std::make_shared<PackedBoolRangeStat>(41476, 41481),
	    std::make_shared<PackedBoolStat>(41506),
	    std::make_shared<PackedBoolStat>(41710),
	    std::make_shared<PackedBoolRangeStat>(41482, 41487),
	    std::make_shared<PackedBoolRangeStat>(41488, 41493),
	    std::make_shared<PackedBoolRangeStat>(41494, 41499),
	    std::make_shared<PackedBoolRangeStat>(41500, 41505),

	    // LOWRIDERS
	    std::make_shared<PackedBoolRangeStat>(42015, 42022),
	    std::make_shared<PackedBoolStat>(41539),
	    std::make_shared<PackedBoolStat>(41711),
	    std::make_shared<PackedBoolRangeStat>(41507, 41514),
	    std::make_shared<PackedBoolRangeStat>(41515, 41522),
	    std::make_shared<PackedBoolRangeStat>(41523, 41530),
	    std::make_shared<PackedBoolRangeStat>(41531, 41538),

	    // AGENTS OF SABOTAGE
	    // std::make_shared<IntStat>("MPX_HACKER_DEN_OWNED", 1),
	    std::make_shared<PackedBoolStat>(9539),
	    std::make_shared<PackedIntStat>(24903, 25),
	    std::make_shared<PackedIntStat>(24904, 2),
	    std::make_shared<IntStat>("MPX_PROG_HUB_DEN24_SAFEEARNINGS", 100000),
	    std::make_shared<IntBitsStat>("MPX_HACKER24_GEN_BS", 5, 12),
	    std::make_shared<PackedBoolStat>(9549),
	    std::make_shared<PackedBoolStat>(9542),
	    std::make_shared<PackedIntStat>(24905, 9),
	    std::make_shared<PackedBoolRangeStat>(9543, 9546),

	    // BOTTOM DOLLAR BOUNTIES
	    // std::make_shared<IntStat>("MPX_BAIL_OFFICE_OWNED", 1),
	    std::make_shared<PackedBoolStat>(9537),
	    std::make_shared<PackedBoolStat>(7639),
	    std::make_shared<PackedIntStat>(7669, 5),
	    std::make_shared<PackedIntStat>(7672, 2),
	    std::make_shared<IntStat>("MPX_PROG_HUB_BOUNTY_EARNINGS", 5000000),
	    std::make_shared<PackedIntStat>(7670, 10),
	    std::make_shared<PackedIntStat>(7674, 3),
	    std::make_shared<PackedIntStat>(7671, 25),
	    std::make_shared<PackedIntStat>(26809, 10),
	    std::make_shared<IntStat>("MPX_PROG_HUB_BOUNTIES_ALIVE_BS", -1),
	    std::make_shared<IntStat>("MPX_BAIL_PROPERTY_EARNINGS", 1000000),

	    // THE CHOP SHOP
	    // std::make_shared<IntStat>("MPX_SALVAGE_YARD_OWNED", 1),
	    std::make_shared<PackedBoolStat>(42038),
	    std::make_shared<PackedBoolStat>(42041),
	    std::make_shared<PackedBoolStat>(42044),
	    std::make_shared<PackedBoolStat>(42045),
	    std::make_shared<IntStat>("MPX_PROG_HUB_SALV23_EARN_SALV", 2500000),
	    std::make_shared<PackedBoolStat>(42042),
	    std::make_shared<IntStat>("MPX_PROG_HUB_SALV23_EARN_PERF", 5),
	    std::make_shared<PackedBoolStat>(42046),
	    std::make_shared<PackedIntStat>(51052, 10),
	    std::make_shared<PackedBoolRangeStat>(42047, 42051),
	    std::make_shared<IntStat>("MPX_PROG_HUB_SALV23_EARN_SELL", 10000000),

	    // LOS SANTOS DRUG WARS
	    std::make_shared<IntBitsStat>("MPX_XM22_MISSIONS_SA", 0, 16),
	    std::make_shared<IntBitsStat>("MPX_XM22_MISSIONS_SA", 6, 10),
	    std::make_shared<IntBitsStat>("MPX_XM22_MISSIONS_SA", 17, 21),
	    std::make_shared<PackedBoolStat>(42037),
	    std::make_shared<IntStat>("MPX_LFETIME_BIKER_BUY_COMPLET6", 100),
	    std::make_shared<IntBitsStat>("MPX_XM22_FLOW", 20, 24),
	    std::make_shared<IntStat>("MPX_LIFETIME_BKR_SELL_EARNINGS6", 10000000),
	    std::make_shared<IntBitStat>("(MPX_XM22_FLOW", 0),
	    std::make_shared<IntStat>("MPX_LFETIME_BIKER_SELL_COMPLET6", 10),
	    std::make_shared<PackedIntStat>(41241, 5),
	    std::make_shared<IntBitStat>("(MPX_XM22_MISSIONS_SA", 5),
	    std::make_shared<IntBitStat>("(MPX_XM22_MISSIONS_SA", 10),
	    std::make_shared<IntBitStat>("(MPX_XM22_MISSIONS_SA", 26),
	    std::make_shared<IntStat>("MPX_PROG_HUB_LSDW_FJ_NO_DEATHS", 25),
	    std::make_shared<IntStat>("MPX_PROG_HUB_DAX_CLONE_KILLS", 100),
	    std::make_shared<PackedIntStat>(42084, 24),
	    std::make_shared<PackedBoolRangeStat>(41660, 41670),

	    // THE CONTRACT
	    std::make_shared<PackedBoolStat>(28257),
	    std::make_shared<IntStat>("MPX_FIXER_COUNT", 100),
	    std::make_shared<IntBitStat>("(MPX_FIXER_GENERAL_BS", 0),
	    std::make_shared<IntBitStat>("(MPX_FIXER_GENERAL_BS", 2),
	    std::make_shared<IntStat>("MPX_PROG_HUB_FXER_PAY_HIT_BONUS", 100),
	    std::make_shared<PackedIntStat>(42085, 10),
	    std::make_shared<IntStat>("MPX_PROG_HUB_FIXER_SEC_CON_SPEC", 100),
	    std::make_shared<IntStat>("MPX_FIXER_EARNINGS", 5000000),

	    // AFTER HOURS
	    std::make_shared<PackedBoolStat>(22067),
	    std::make_shared<IntStat>("MPX_NIGHTCLUB_JOBS_DONE", 25),
	    std::make_shared<IntStat>("MPX_PROG_HUB_DANCE_DUR", 5),
	    std::make_shared<PackedBoolStat>(15533),
	    std::make_shared<PackedBoolRangeStat>(22082, 22083),
	    std::make_shared<PackedBoolStat>(36868),
	    std::make_shared<PackedBoolStat>(36944),
	    std::make_shared<PackedBoolStat>(41989),
	    std::make_shared<PackedIntStat>(42089, 8),
	    std::make_shared<IntStat>("MPX_NIGHTCLUB_VIP_APPEAR", 25),
	    std::make_shared<IntStat>("MPX_PROG_HUB_NCLUB_POP_MAX_TME", 600),
	    std::make_shared<IntStat>("MPX_HUB_EARNINGS", 50000000),

	    // SMUGGLER'S RUN
	    std::make_shared<PackedBoolStat>(15966),
	    std::make_shared<IntStat>("MPX_LFETIME_HANGAR_BUY_COMPLET", 100),
	    std::make_shared<PackedBoolStat>(41676),
	    std::make_shared<PackedBoolStat>(32398),
	    std::make_shared<IntStat>("MPX_LFETIME_HANGAR_SEL_COMPLET", 1),
	    std::make_shared<PackedBoolStat>(41987),
	    std::make_shared<PackedBoolStat>(36924),
	    std::make_shared<IntStat>("MPX_PROG_HUB_SMUGGLER_CRATES", 1000),
	    std::make_shared<PackedBoolRangeStat>(36925, 36932),
	    std::make_shared<IntStat>("MPX_LFETIME_HANGAR_EARNINGS", 50000000),

	    // GUNRUNNING
	    // std::make_shared<IntStat>("MPX_FACTORYSETUP5", 1),
	    std::make_shared<IntStat>("MPX_LFETIME_BIKER_BUY_COMPLET5", 25),
	    std::make_shared<PackedIntStat>(9359, 25),
	    std::make_shared<PackedBoolStat>(36870),
	    std::make_shared<IntStat>("MPX_WVM_FLOW_BITSET_MISSIONS0", -1),
	    std::make_shared<IntStat>("MPX_LIFETIME_BKR_SEL_COMPLETBC5", 1),
	    std::make_shared<IntStat>("MPX_LFETIME_BIKER_SELL_COMPLET5", 1),
	    std::make_shared<IntBitsStat>("MPX_SR_WEAPON_BIT_SET", 1, 17),
	    std::make_shared<PackedBoolRangeStat>(42002, 42013),
	    std::make_shared<PackedBoolRangeStat>(36831, 36838),
	    std::make_shared<IntStat>("MPX_LIFETIME_BKR_SELL_EARNINGS5", 25000000),

	    // IMPORT / EXPORT
	    // std::make_shared<IntStat>("MPX_OWNED_IE_WAREHOUSE", 1),
	    std::make_shared<IntStat>("MPX_LFETIME_IE_EXPORT_COMPLETED", 25),
	    std::make_shared<PackedBoolRangeStat>(42029, 42036),
	    std::make_shared<PackedBoolRangeStat>(41874, 41883),
	    std::make_shared<IntStat>("MPX_PROG_HUB_VEH_CARGO_SELL_PER", 50),
	    std::make_shared<IntStat>("MPX_PROG_HUB_VEH_CARGO_SPECIAL", 5),
	    std::make_shared<PackedBoolStat>(41988),
	    std::make_shared<PackedBoolRangeStat>(41540, 41547),
	    std::make_shared<IntStat>("MPX_LFETIME_IE_MISSION_EARNINGS", 25000000),

	    // BIKERS
	    std::make_shared<PackedBoolStat>(36871),
	    std::make_shared<PackedBoolStat>(36872),
	    std::make_shared<PackedBoolRangeStat>(36875, 36887),
	    std::make_shared<PackedBoolStat>(36873),
	    std::make_shared<PackedBoolStat>(42001),
	    std::make_shared<PackedBoolStat>(36874),
	    std::make_shared<IntStat>("MPX_LIFETIME_BKR_SELL_COMPLETBC", 1),
	    std::make_shared<IntStat>("MPX_LIFETIME_BKR_SEL_COMPLETBC1", 1),
	    std::make_shared<IntStat>("MPX_LIFETIME_BKR_SEL_COMPLETBC2", 1),
	    std::make_shared<IntStat>("MPX_LIFETIME_BKR_SEL_COMPLETBC3", 1),
	    std::make_shared<IntStat>("MPX_LIFETIME_BKR_SEL_COMPLETBC4", 1),
	    std::make_shared<IntStat>("MPX_BAR_RESUPPLY_CR", 10),
	    std::make_shared<IntStat>("MPX_LIFETIME_BKR_SELL_EARNINGS0", 25000000),
	    std::make_shared<IntStat>("MPX_PROG_HUB_BIK_CUST_DEL_CASH", 2500000),
	    std::make_shared<IntStat>("MPX_PROG_HUB_CLBH_BAR_EARNINGS", 500000),
	    std::make_shared<IntStat>("MPX_PROG_HUB_BIK_CONTRACT_COUNT", 50),

	    // FURTHER ADVENTURES IN FINANCE AND FELONY
	    std::make_shared<PackedBoolStat>(36888),
	    std::make_shared<PackedBoolStat>(36889),
	    std::make_shared<PackedBoolRangeStat>(36892, 36915),
	    std::make_shared<PackedBoolStat>(36890),
	    std::make_shared<IntStat>("MPX_WARHOUSESLOT0", 1),
	    std::make_shared<IntStat>("MPX_LIFETIME_BUY_COMPLETE", 1),
	    std::make_shared<PackedBoolRangeStat>(7559, 7553),
	    std::make_shared<PackedBoolStat>(36891),
	    std::make_shared<PackedBoolRangeStat>(36860, 36865),
	    std::make_shared<IntStat>("MPX_PROG_HUB_FAIFAF_CRATES_COL", 250),
	    std::make_shared<IntStat>("MPX_LIFETIME_CONTRA_EARNINGS", 50000000),

	    // LOS SANTOS TUNERS
	    // std::make_shared<IntStat>("MPX_CAR_CLUB_MEMBERSHIP", 1),
	    std::make_shared<PackedBoolStat>(31737),
	    std::make_shared<PackedBoolStat>(41870),
	    std::make_shared<PackedBoolStat>(31753),
	    std::make_shared<IntBitsStat>("MPX_TUNER_COMP_BS", 0, 7),
	    std::make_shared<PackedBoolStat>(32397),
	    std::make_shared<PackedIntStat>(30226, 10),
	    std::make_shared<IntStat>("MPX_TUNER_COUNT", 25),
	    std::make_shared<IntStat>("MPX_PROG_HUB_TUNER_CUS_DEL_CASH", 5000000),

	    // THE DIAMOND CASINO & RESORT
	    std::make_shared<PackedBoolStat>(27089),
	    std::make_shared<PackedBoolStat>(27090),
	    std::make_shared<IntBitStat>("(MPX_VCM_FLOW_PROGRESS", 6),
	    std::make_shared<IntBitStat>("(MPX_VCM_FLOW_PROGRESS", 12),
	    std::make_shared<IntBitStat>("(MPX_VCM_FLOW_PROGRESS", 11),
	    std::make_shared<IntBitStat>("(MPX_VCM_FLOW_PROGRESS", 17),
	    std::make_shared<PackedBoolStat>(36916),
	    std::make_shared<PackedBoolRangeStat>(36844, 36859),
	    std::make_shared<PackedBoolRangeStat>(41548, 41553),
	    std::make_shared<PackedBoolStat>(41868),
	    std::make_shared<PackedIntStat>(42093, 11),
	    std::make_shared<PackedBoolRangeStat>(41560, 41565),
	    std::make_shared<PackedBoolRangeStat>(41554, 41559),

	    // THE CAYO PERICO HEIST
	    std::make_shared<PackedBoolStat>(30309),
	    std::make_shared<PackedBoolStat>(30522),
	    std::make_shared<IntBitStat>("MPX_H4_MISSIONS", 0),
	    std::make_shared<IntBitStat>("(MPX_H4_PROGRESS", 1),
	    std::make_shared<IntBitsStat>("MPX_H4_PROGRESS", 6, 11),
	    std::make_shared<IntBitsStat>("MPX_H4_H4_DJ_MISSIONS", 0, 6),
	    std::make_shared<PackedBoolStat>(41677),
	    std::make_shared<IntStat>("MPX_PROG_HUB_CAYO_PRP_NO_DEATHS", 25),
	    std::make_shared<PackedIntStat>(42094, 15),
	    std::make_shared<IntStat>("MPX_CR_SUBMARINE", 1),
	    std::make_shared<IntStat>("MPX_CR_STRATEGIC_BOMBER", 1),
	    std::make_shared<IntStat>("MPX_CR_SMUGGLER_PLANE", 1),
	    std::make_shared<IntStat>("MPX_CR_STEALTH_HELI", 1),
	    std::make_shared<IntStat>("MPX_CR_PATROL_BOAT", 1),
	    std::make_shared<IntStat>("MPX_CR_SMUGGLER_BOAT", 1),
	    std::make_shared<IntStat>("MPX_PROG_HUB_CAYO_H_EARNINGS", 50000000),

	    // THE DIAMOND CASINO HEIST

	    std::make_shared<IntBitStat>("(MPX_CAS_HEIST_FLOW", 1),
	    std::make_shared<PackedBoolStat>(28270),
	    std::make_shared<PackedBoolStat>(36842),
	    std::make_shared<IntBitStat>("(MPX_CAS_HEIST_FLOW", 10),
	    std::make_shared<PackedBoolStat>(32399),
	    std::make_shared<IntBitsStat>("MPX_CAS_HEIST_FLOW", 11, 13),
	    std::make_shared<PackedBoolStat>(42025),
	    std::make_shared<PackedBoolStat>(42086),
	    std::make_shared<PackedBoolRangeStat>(41678, 41681),
	    std::make_shared<IntStat>("MPX_PROG_HUB_CASINO_H_EARNINGS", 50000000),

	    // THE DOOMSDAY HEIST
	    std::make_shared<PackedBoolStat>(18139),
	    std::make_shared<PackedBoolStat>(36861),
	    std::make_shared<IntBitsStat>("MPX_GANGOPS_FLOW_BITSET_MISS0", 0, 2),
	    std::make_shared<PackedBoolStat>(41712),
	    std::make_shared<PackedBoolStat>(36862),
	    std::make_shared<PackedBoolStat>(41713),
	    std::make_shared<PackedBoolStat>(36863),
	    std::make_shared<PackedBoolStat>(41714),
	    std::make_shared<IntStat>("MPX_PROG_HUB_DOOM_PRP_NO_DEATHS", 14),
	    std::make_shared<PackedIntStat>(42087, 10),
	    std::make_shared<PackedBoolStat>(41685),
	    std::make_shared<PackedBoolStat>(41690),
	    std::make_shared<PackedBoolStat>(41696),
			std::make_shared<PackedBoolRangeStat>(41697, 41699),
			std::make_shared<IntStat>("MPX_PROG_HUB_DOOMSDAY_ACTS", 15),

			// ORIGINAL HEISTS
			std::make_shared<PackedBoolStat>(36867),
			std::make_shared<PackedBoolStat>(36933),
			std::make_shared<PackedBoolStat>(41700),
			std::make_shared<PackedBoolStat>(41715),
			std::make_shared<PackedBoolStat>(41716),
			std::make_shared<PackedBoolStat>(41717),
			std::make_shared<PackedBoolStat>(41718),
			std::make_shared<PackedBoolStat>(41719),
			std::make_shared<PackedIntStat>(42100, 5),
			std::make_shared<PackedIntStat>(42090, 15),
			std::make_shared<IntStat>("MPX_PROG_HUB_HEIST_EARNINGS", 50000000),
			std::make_shared<PackedBoolRangeStat>(41701, 41704),
			std::make_shared<PackedBoolStat>(36917),
			std::make_shared<PackedBoolStat>(42000),

			// ARENA WAR
			std::make_shared<PackedBoolStat>(25009),
			std::make_shared<PackedBoolRangeStat>(41647, 41655),
			std::make_shared<PackedIntStat>(22063, 20),
			std::make_shared<IntStat>("MPX_ARENAWARS_AP_TIER", 1000),
			std::make_shared<PackedIntStat>(42088, 15),
			std::make_shared<PackedIntStat>(42000, 20),

			// ADVERSARY MODS
			std::make_shared<PackedBoolRangeStat>(41594, 41646),
			std::make_shared<PackedBoolStat>(36840),
			std::make_shared<IntStat>("MPX_PROG_HUB_ADV_WINS", 50),
			std::make_shared<PackedBoolStat>(36921),

			// SURVIVALS
			std::make_shared<IntStat>("MPX_PROG_HUB_SURVIVALS_PLAYED", 1),
			std::make_shared<PackedBoolStat>(41672),
			std::make_shared<IntStat>("MPX_PROG_HUB_SURV_WAVES", 250),
			std::make_shared<PackedBoolStat>(41673),
			std::make_shared<PackedBoolStat>(41332),
			std::make_shared<PackedBoolStat>(41331),

			// RACING
			std::make_shared<PackedBoolStat>(42023),
			std::make_shared<PackedBoolRangeStat>(41363, 42151),
			std::make_shared<IntStat>("MPX_RACES_WON", 50),
			std::make_shared<IntBitsStat>("MPX_PROG_HUB_T_TRIAL_PAR_TIME", 0, 25),
			std::make_shared<IntBitsStat>("MPX_PROG_HUB_T_TRL_PAR_TIME_RC", 0, 25),
			std::make_shared<IntBitsStat>("MPX_PROG_HUB_T_TRL_PAR_TIME_HSW", 0, 25),
			std::make_shared<IntBitsStat>("MPX_PROG_HUB_T_TRL_PAR_TIME_BKE", 0, 25),
			std::make_shared<PackedIntStat>(41246, 20),
			std::make_shared<IntStat>("MPX_PROG_HUB_FST_LPS_RSTAR_RAC", 50),


			// DEATHMATCHES
			std::make_shared<IntStat>("MPX_DM_END", 1),
			std::make_shared<IntStat>("MPX_PROG_HUB_DM_TDM_PLAYS", 1),
			std::make_shared<IntStat>("MPX_PROG_HUB_DM_COMMUNITY_PLAYS", 10),
			std::make_shared<IntStat>("MPX_PROG_HUB_DTHM_KILL_5_WO_DIE", 25),
			std::make_shared<PackedBoolStat>(36922),

			// VEHICLE ENTHUSIAST
			std::make_shared<PackedBoolStat>(41864),
			std::make_shared<PackedBoolStat>(42014),
			std::make_shared<PackedBoolStat>(41865),
			std::make_shared<PackedBoolStat>(41863),
			std::make_shared<PackedBoolStat>(41840),
			std::make_shared<PackedBoolStat>(41839),
			std::make_shared<PackedBoolStat>(41841),
			std::make_shared<PackedBoolStat>(41838),
			std::make_shared<PackedIntStat>(42092, 5),
			std::make_shared<PackedIntStat>(42091, 10),
			std::make_shared<IntStat>("MPX_HUB_VEH_ENTH_OWNED_VEHS", 99),
			std::make_shared<PackedBoolStat>(41866),
			std::make_shared<PackedBoolRangeStat>(41842, 41862),
			std::make_shared<IntStat>("MPX_VEHICLES_CUSTOMISED", 50),

			// WEAPONS EXPERT
			std::make_shared<PackedBoolStat>(36934),
			std::make_shared<PackedBoolStat>(36935),
			std::make_shared<PackedBoolStat>(36936),
			std::make_shared<PackedBoolStat>(36937),
			std::make_shared<PackedBoolStat>(36938),
			std::make_shared<PackedBoolStat>(36920),
			std::make_shared<IntStat>("MPX_PROG_HUB_WEAP_TYPE_ANSR", -1),
			std::make_shared<PackedIntStat>(41242, 8),
			std::make_shared<PackedBoolStat>(36942),
			std::make_shared<IntStat>("MPX_PROG_HUB_10_CHAL_ANSR", 10),
			std::make_shared<PackedBoolStat>(36941),
			std::make_shared<PackedBoolRangeStat>(15456, 15460),

			// A Safehouse In The Hills
			std::make_shared<PackedBoolStat>(54773),
			std::make_shared<PackedBoolStat>(54774),
			std::make_shared<PackedBoolStat>(54775),
			std::make_shared<PackedBoolRangeStat>(54776, 54780),
			std::make_shared<IntStat>("MPX_PROG_HUB_KNOWAY_EARNINGS", 5000000),
			std::make_shared<PackedBoolRangeStat>(54781, 54785),
			std::make_shared<PackedBoolStat>(54786),

			// The Kortz Center Heist
			std::make_shared<PackedBoolStat>(60011),
			std::make_shared<PackedBoolStat>(60021),
			std::make_shared<IntBitStat>("MPX_K26_GENERAL_BS", 2),
			std::make_shared<IntStat>("MPX_AWD_PREPPER", 20),
			std::make_shared<PackedBoolStat>(60022),
			std::make_shared<PackedBoolStat>(60023),
			std::make_shared<PackedIntStat>(60049, 5),
			std::make_shared<PackedBoolRangeStat>(60024, 60027),
			std::make_shared<PackedIntStat>(60050, 15),
			std::make_shared<PackedBoolStat>(60028),
			std::make_shared<IntStat>("MPX_PROG_HUB_KORTZ_STOLEN_VALUE", 50000000),
			std::make_shared<IntBitsStat>("MPX_K26_TARGETS_OWNED_BS", 1, 26),

			  //Misc
			   Stats::SaveStats();
			}
			else
			{
				Notifications::Show("YimMenuV2", "You Must be Online", NotificationType::Error);
			}
		}
	};
	static UnlockCareerProgress _UnlockCareerProgress{
	    "unlockallcareerprogress",
	    "Unlock All Career Progress",
	    "Completes all Career Progress categories"};
 }
