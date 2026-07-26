#include "core/commands/Command.hpp"
#include "core/commands/ListCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "game/pointers/Pointers.hpp"
#include "types/script/globals/GPBD_FM.hpp"
#include "types/script/globals/GPBD_FM_2.hpp"

namespace YimMenu::Features
{
	enum class eAppVinewoodMenuSafe
	{
		NIGHTCLUB,
		ARCADE,
		AGENCY,
		SALVAGE_YARD,
		BAIL_OFFICE,
		GARMENT_FACTORY,
		HANDS_ON_CAR_WASH
	};

	static std::vector<std::pair<int, const char*>> businessNames = {
	    {static_cast<int>(eAppVinewoodMenuSafe::NIGHTCLUB), "Nightclub"},
	    {static_cast<int>(eAppVinewoodMenuSafe::ARCADE), "Arcade"},
	    {static_cast<int>(eAppVinewoodMenuSafe::AGENCY), "Agency"},
	    {static_cast<int>(eAppVinewoodMenuSafe::SALVAGE_YARD), "Salvage Yard"},
	    {static_cast<int>(eAppVinewoodMenuSafe::BAIL_OFFICE), "Bail Office"},
	    {static_cast<int>(eAppVinewoodMenuSafe::GARMENT_FACTORY), "Garment Factory"},
	    {static_cast<int>(eAppVinewoodMenuSafe::HANDS_ON_CAR_WASH), "Hands on Car Wash"}
	};

	static ListCommand _SelectedBusiness{
	    "businesssafe",
	    "Business",
	    "Business to claim earnings from.",
	    businessNames,
	    0};

	static bool IsValidSession()
	{
		return Pointers.IsSessionStarted && *Pointers.IsSessionStarted;
	}

	static void ClaimNightclubSafe()
	{
		const auto playerId = Self::GetPlayer().GetId();

		if (GPBD_FM::Get()->Entries[playerId].PropertyData.NightclubData.SafeCashValue > 0)
			*ScriptGlobal(2708943).As<BOOL*>() = TRUE;
	}

	static void ClaimArcadeSafe()
	{
		const auto playerId = Self::GetPlayer().GetId();

		if (GPBD_FM::Get()->Entries[playerId].PropertyData.ArcadeData.SafeCashValue > 0)
			*ScriptGlobal(2708952).As<BOOL*>() = TRUE;
	}

	static void ClaimAgencySafe()
	{
		const auto playerId = Self::GetPlayer().GetId();

		if (GPBD_FM::Get()->Entries[playerId].PropertyData.FixerHQData.SafeCashValue > 0)
			*ScriptGlobal(2708961).As<BOOL*>() = TRUE;
	}

	static void ClaimSalvageYardSafe()
	{
		const auto playerId = Self::GetPlayer().GetId();

		if (GPBD_FM::Get()->Entries[playerId].PropertyData.SalvageYardData.TotalEarnings > 0)
			*ScriptGlobal(2708970).As<BOOL*>() = TRUE;
	}

	static void ClaimBailOfficeSafe()
	{
		const auto playerId = Self::GetPlayer().GetId();

		if (GPBD_FM::Get()->Entries[playerId].PropertyData.BailShopData.SafeCashValue > 0)
			*ScriptGlobal(2708979).As<BOOL*>() = TRUE;
	}

	static void ClaimGarmentFactorySafe()
	{
		const auto playerId = Self::GetPlayer().GetId();

		if (GPBD_FM::Get()->Entries[playerId].PropertyData.HackerDenData.SafeCashValue > 0)
			*ScriptGlobal(2708994).As<BOOL*>() = TRUE;
	}

	static void ClaimHandsOnCarWashSafe()
	{
		const auto playerId = Self::GetPlayer().GetId();

		if (GPBD_FM_2::Get()->Entries[playerId].SYVehSaleData.HOWCData.SafeCashValue > 0)
			*ScriptGlobal(2709001).As<BOOL*>() = TRUE;
	}

	class ClaimSafeEarnings : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (!IsValidSession())
				return;

			switch (static_cast<eAppVinewoodMenuSafe>(_SelectedBusiness.GetState()))
			{
			case eAppVinewoodMenuSafe::NIGHTCLUB:
				ClaimNightclubSafe();
				break;

			case eAppVinewoodMenuSafe::ARCADE:
				ClaimArcadeSafe();
				break;

			case eAppVinewoodMenuSafe::AGENCY:
				ClaimAgencySafe();
				break;

			case eAppVinewoodMenuSafe::SALVAGE_YARD:
				ClaimSalvageYardSafe();
				break;

			case eAppVinewoodMenuSafe::BAIL_OFFICE:
				ClaimBailOfficeSafe();
				break;

			case eAppVinewoodMenuSafe::GARMENT_FACTORY:
				ClaimGarmentFactorySafe();
				break;

			case eAppVinewoodMenuSafe::HANDS_ON_CAR_WASH:
				ClaimHandsOnCarWashSafe();
				break;
			}
		}
	};

	class ClaimAllSafeEarnings : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			if (!IsValidSession())
				return;

			ClaimNightclubSafe();
			ClaimArcadeSafe();
			ClaimAgencySafe();
			ClaimSalvageYardSafe();
			ClaimBailOfficeSafe();
			ClaimGarmentFactorySafe();
			ClaimHandsOnCarWashSafe();
		}
	};

	static ClaimSafeEarnings _ClaimSafeEarnings{"claimsafeearnings", "Claim Safe Earnings", "Claims safe earnings from the selected business."};
	static ClaimAllSafeEarnings _ClaimAllSafeEarnings{"claimallsafeearnings", "Claim All Safe Earnings", "Claims available safe earnings from every owned business."};
}
