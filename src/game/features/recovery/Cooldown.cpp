#include "core/commands/Command.hpp"
#include "game/gta/Natives.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/gta/Stats.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "types/script/Timer.hpp"


namespace YimMenu::Features
{
	class ResetCasinoCooldown : public Command
	{
		using Command::Command;
		virtual void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				Stats::SetInt("MPPLY_CASINO_CHIPS_WON_GD", 0);
				Stats::SetInt("MPPLY_CASINO_BAN_TIME", 0);
				STATS::STAT_SAVE(0, 0, 3, 0);
			}
			else
			{
				Notifications::Show("YimMenuV2", "You Must be Online", NotificationType::Error);
			}
		}
	};
	static ResetCasinoCooldown _ResetCasinoCooldown{"reset_casino_cd", "Reset Casino Cooldowns", "Resets casino daily chip limit and gambling ban"};
	class ResetVehicleSellCooldown : public Command
	{
		using Command::Command;
		virtual void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				Stats::SetInt("MPPLY_VEHICLE_SELL_TIME", 0);
				STATS::STAT_SAVE(0, 0, 3, 0);
			}
			else
			{
				Notifications::Show("YimMenuV2", "You Must be Online", NotificationType::Error);
			}
		}
	};
	static ResetVehicleSellCooldown _ResetVehicleSellCooldown{"reset_vehicle_sell_cd", "Reset Vehicle Sell Cooldown", "Resets the vehicle sell cooldown timer"};
	class ResetDeliveryCooldown : public Command
	{
		using Command::Command;
		virtual void OnCall() override
		{
			if (*Pointers.IsSessionStarted)
			{
				FiberPool::Push([] {
					ScriptGlobal(2686124).At(4373).At(260).At(7, 2).As<TIMER*>()->Destroy();
				});
				STATS::STAT_SAVE(0, 0, 3, 0);
			}
			else
			{
				Notifications::Show("YimMenuV2", "You Must be Online", NotificationType::Error);
			}
		}
	};
	static ResetDeliveryCooldown _ResetDeliveryCooldown{"Reset_vehicle_Delivery_cd", "Reset Vehicle Delivery Cooldown", "Reset Vehicle Delivery Cooldown"};
	class RemoveCayoCooldown : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			*ScriptGlobal(262145).At(29507).As<int*>() = 0;
			*ScriptGlobal(262145).At(29508).As<int*>() = 0;
			*ScriptGlobal(262145).At(29509).As<int*>() = 0;
		}
	};
	static RemoveCayoCooldown _RemoveCayoCooldown{"Reset_Cayo_Perico_cd", "Remove Cayo Cooldown", "Removes the Cayo Perico Heist cooldown"};
	class Reset_Bunker_Vehicle_cd : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			*ScriptGlobal(262145).At(33130).As<int*>() = 0;
		}
	};
    static Reset_Bunker_Vehicle_cd _Reset_Bunker_Vehicle_cd{"reset_bunker_vehicle_cd", "Reset Bunker Vehicle Cooldown", "Resets the bunker vehicle cooldown."};
	class Reset_Tony_Limo_cd : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			*ScriptGlobal(262145).At(33129).As<int*>() = 0;
		}
	};
	static Reset_Tony_Limo_cd _Reset_Tony_Limo_cd{"reset_tony_limo_cd", "Reset Tony Limo Cooldown", "Resets the Tony Limo cooldown."};
	class Reset_Terrorbyte_Drone_Shock_cd : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			*ScriptGlobal(262145).At(24326).As<int*>() = 0;
			*ScriptGlobal(262145).At(24327).As<int*>() = 0;
		}
	};
	static Reset_Terrorbyte_Drone_Shock_cd _Reset_Terrorbyte_Drone_Shock_cd{"reset_terrorbyte_drone_shock_cd", "Reset Terrorbyte Drone Shock Cooldown", "Removes the Terrorbyte drone shock cooldown."};
	class Reset_Auto_Shop_Robbery_cd : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			
			*ScriptGlobal(262145).At(30484).As<int*>() = 0;
		}
	};
	static Reset_Auto_Shop_Robbery_cd _Reset_Auto_Shop_Robbery_cd{"reset_auto_shop_robbery_cd", "Reset Auto Shop Robbery Cooldown", "Removes the Auto Shop robbery cooldown."};
    class Reset_Security_Contract_cd : public Command
	{
		using Command::Command;

		void OnCall() override
		{

			*ScriptGlobal(262145).At(31292).As<int*>() = 0;
			*ScriptGlobal(262145).At(31293).As<int*>() = 0;
		}
	};
	static Reset_Security_Contract_cd _Reset_Security_Contract_cd{"reset_security_contract_cd", "Reset Security Contract Cooldown", "Removes the Security Contract refresh and cooldown timers."};
    class Reset_Doomsday_Heist_cd : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			*ScriptGlobal(262145).At(23103).As<int*>() = 0;
			*ScriptGlobal(262145).At(23104).As<int*>() = 0;
			*ScriptGlobal(262145).At(23105).As<int*>() = 0;
			*ScriptGlobal(262145).At(23106).As<int*>() = 0;
			*ScriptGlobal(262145).At(23107).As<int*>() = 0;
			*ScriptGlobal(262145).At(23108).As<int*>() = 0;
			*ScriptGlobal(262145).At(23109).As<int*>() = 0;
			*ScriptGlobal(262145).At(23110).As<int*>() = 0;
			*ScriptGlobal(262145).At(23111).As<int*>() = 0;
			*ScriptGlobal(262145).At(23112).As<int*>() = 0;
			*ScriptGlobal(262145).At(23113).As<int*>() = 0;
			*ScriptGlobal(262145).At(23114).As<int*>() = 0;
		}
	};
	static Reset_Doomsday_Heist_cd _Reset_Doomsday_Heist_cd{"reset_doomsday_heist_cd", "Reset Doomsday Heist Cooldown", "Removes the Doomsday Heist replay cooldowns."};
}
