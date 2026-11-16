#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"
#include "game/gta/Pools.hpp"

#include <chrono>
#include <unordered_map>

namespace YimMenu::Features
{
	class AllVehsLoudRadio : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;
		using Clock = std::chrono::steady_clock;

		static constexpr float MAX_DISTANCE_SQ = 250.f * 250.f;           
		static constexpr int MAX_WORK_PER_TICK = 8;                       
		static constexpr auto REAPPLY_INTERVAL = std::chrono::seconds(3); 
		static constexpr auto RETRY_INTERVAL = std::chrono::seconds(2);   
		static constexpr auto CLEANUP_INTERVAL = std::chrono::seconds(5);

		struct VehState
		{
			Clock::time_point NextAttempt{};
			bool Applied = false;
			bool Forced = false; 
		};

		std::unordered_map<int, VehState> m_States;
		Clock::time_point m_NextCleanup{};

		void OnTick() override
		{
			const auto now = Clock::now();
			const auto selfPos = Self::GetPed().GetPosition();
			int work = 0;

			for (auto veh : Pools::GetVehicles())
			{
				if (work >= MAX_WORK_PER_TICK)
					break;

				if (!veh.IsValid())
					continue;

				auto& state = m_States[veh.GetHandle()];
				if (now < state.NextAttempt)
					continue;

				const int handle = veh.GetHandle();

				if (!ENTITY::DOES_ENTITY_EXIST(handle) || ENTITY::IS_ENTITY_DEAD(handle, false))
					continue;

				const auto pos = veh.GetPosition();
				const float dx = pos.x - selfPos.x, dy = pos.y - selfPos.y, dz = pos.z - selfPos.z;
				if (dx * dx + dy * dy + dz * dz > MAX_DISTANCE_SQ)
					continue;

				++work;

				if (!veh.HasControl() && !veh.RequestControl(0))
				{
					if (!state.Forced)
					{
						veh.ForceControl();
						state.Forced = true;
					}

					if (!veh.HasControl())
					{
						state.NextAttempt = now + RETRY_INTERVAL;
						continue;
					}
				}

				if (!ENTITY::DOES_ENTITY_EXIST(handle))
					continue;

				AUDIO::SET_VEHICLE_RADIO_ENABLED(handle, true);
				AUDIO::SET_VEHICLE_RADIO_LOUD(handle, true);

				if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(handle))
					VEHICLE::SET_VEHICLE_ENGINE_ON(handle, true, true, false);

				state.Applied = true;
				state.NextAttempt = now + REAPPLY_INTERVAL;
			}

			Cleanup(now);
		}

		void Cleanup(Clock::time_point now)
		{
			if (now < m_NextCleanup)
				return;
			m_NextCleanup = now + CLEANUP_INTERVAL;

			for (auto it = m_States.begin(); it != m_States.end();)
			{
				if (!ENTITY::DOES_ENTITY_EXIST(it->first))
					it = m_States.erase(it);
				else
					++it;
			}
		}

		void OnDisable() override
		{
			for (auto& [handle, state] : m_States)
			{
				if (!state.Applied || !ENTITY::DOES_ENTITY_EXIST(handle))
					continue;

				if (NETWORK::NETWORK_HAS_CONTROL_OF_ENTITY(handle))
					AUDIO::SET_VEHICLE_RADIO_LOUD(handle, false);
			}

			m_States.clear();
			m_NextCleanup = {};
		}
	};

	static AllVehsLoudRadio _AllVehsLoudRadio{
	    "loudsubwoofer",
	    "Subwoofer",
	    "Enables loud radio on nearby vehicles, players can hear it."};
}
