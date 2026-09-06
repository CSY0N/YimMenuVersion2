#include "core/commands/FloatCommand.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"
#include "types/pad/ControllerInputs.hpp"

namespace YimMenu::Features
{
	static FloatCommand _HornBoostSpeed{"hornboostspeed", "Horn Boost Power", "Controls how strongly the vehicle accelerates while holding the horn", 0.1f, 10.0f, 1.0f};

	class HornBoost : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		static constexpr float horn_boost_speed_default = 10.0f;
		static constexpr float horn_boost_speed_max = 500.0f;

		float m_HornBoostSpeed = horn_boost_speed_default;

		virtual void OnTick() override
		{
			auto veh = Self::GetVehicle();

			if (!veh)
			{
				m_HornBoostSpeed = horn_boost_speed_default;
				return;
			}

			const auto horn = static_cast<int>(ControllerInputs::INPUT_VEH_HORN);

			if (PAD::IS_CONTROL_JUST_PRESSED(0, horn))
			{
				m_HornBoostSpeed = veh.GetSpeed();

				if (m_HornBoostSpeed < horn_boost_speed_default)
					m_HornBoostSpeed = horn_boost_speed_default;
			}

			if (PAD::IS_CONTROL_PRESSED(0, horn))
			{
				m_HornBoostSpeed += _HornBoostSpeed.GetState();

				if (m_HornBoostSpeed > horn_boost_speed_max)
					m_HornBoostSpeed = horn_boost_speed_max;

				const auto position = veh.GetPosition();
				const auto target = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(veh.GetHandle(), 0.0f, m_HornBoostSpeed, 0.0f);

				Vector3 velocity{target.x - position.x, target.y - position.y, target.z - position.z};
				veh.SetVelocity(velocity);
			}
			else if (PAD::IS_CONTROL_JUST_RELEASED(0, horn))
				m_HornBoostSpeed = horn_boost_speed_default;
		}

		virtual void OnDisable() override
		{
			m_HornBoostSpeed = horn_boost_speed_default;
		}
	};

	static HornBoost _HornBoost{"hornboost", "Horn Boost", "Increase vehicle speed and acceleration while holding the horn"};
}
