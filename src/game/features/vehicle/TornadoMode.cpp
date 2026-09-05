#include "core/commands/FloatCommand.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
	static FloatCommand _TornadoStrength{"tornadostrength", "Tornado Strength", "Controls how strongly the tornado affects your vehicle", 0.1f, 5.0f, 1.0f};

	class TornadoMode : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		float m_Angle = 0.0f;

		virtual void OnTick() override
		{
			auto veh = Self::GetVehicle();

			if (!veh)
				return;

			const float strength = _TornadoStrength.GetState();

			m_Angle += 0.035f * strength;

			if (m_Angle > 6.283185f)
				m_Angle = 0.0f;

			const float spinForce = 12.0f * strength;
			const float liftForce = 4.0f * strength;

			ENTITY::APPLY_FORCE_TO_ENTITY(veh.GetHandle(), 1, -std::sin(m_Angle) * spinForce, std::cos(m_Angle) * spinForce, liftForce, 0.0f, 0.0f, 2.5f * strength, 0, false, true, true, false, true);
		}

		virtual void OnDisable() override
		{
			m_Angle = 0.0f;
		}
	};

	static TornadoMode _TornadoMode{"tornadomode", "Tornado Mode", "Throws your vehicle into a powerful rising tornado"};
}
