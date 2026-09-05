#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	class AutoKillEnemies : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{

			if (!Self::GetPed())
				return;

			for (::Ped ped : Self::GetPed().GetNearbyPeds())
			{
				if (!ENTITY::DOES_ENTITY_EXIST(ped) || PED::IS_PED_A_PLAYER(ped) || ENTITY::IS_ENTITY_DEAD(ped, false))
					continue;

				const auto pos = ENTITY::GET_ENTITY_COORDS(ped, false);
				const float dx = pos.x - Self::GetPed().GetPosition().x;
				const float dy = pos.y - Self::GetPed().GetPosition().y;
				const float dz = pos.z - Self::GetPed().GetPosition().z;

				if ((dx * dx + dy * dy + dz * dz) > 10000.0f)
					continue;

				if (PED::IS_PED_IN_COMBAT(ped, Self::GetPed().GetHandle()))
					ENTITY::SET_ENTITY_HEALTH(ped, 0, 0, 0);
			}
		}
	};

	static AutoKillEnemies _AutoKillEnemies{"autokillenemies", "Auto Kill Enemies", "Automatically eliminates nearby hostile NPCs."};
}
