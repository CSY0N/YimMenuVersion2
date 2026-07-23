#include "core/commands/BoolCommand.hpp"
#include "game/backend/ScriptPatches.hpp"

namespace YimMenu::Features
{
	class HighDemandSell : public BoolCommand
	{
		using BoolCommand::BoolCommand;

		ScriptPatch m_RivalPatch{};
		ScriptPatch m_PrivatePatch1{};
		ScriptPatch m_PrivatePatch2{};
		ScriptPatch m_PrivatePatch3{};
		ScriptPatch m_PrivatePatch4{};
		ScriptPatch m_PrivatePatch5{};

		virtual void OnEnable() override
		{
			if (!m_RivalPatch)
			{
				m_RivalPatch = ScriptPatches::AddPatch("gb_contraband_sell"_J, ScriptPointer("HighDemandRivals", "2D 01 06 00 00 71 39 04 71").Add(5), {0x25, 0x19, 0x2E, 0x01, 0x01});
			}

			if (!m_PrivatePatch1)
			{
				m_PrivatePatch1 = ScriptPatches::AddPatch("gb_contraband_sell"_J, ScriptPointer("HDPrivate1", "2C 01 ? ? 06 56 5A 00 37 0B").Add(4), {0x2B, 0x55, 0x00, 0x00});
			}

			if (!m_PrivatePatch2)
			{
				m_PrivatePatch2 = ScriptPatches::AddPatch("gb_contraband_sell"_J, ScriptPointer("HDPrivate2", "2C 01 ? ? 06 56 5C 00 37 0C").Add(4), {0x2B, 0x55, 0x00, 0x00});
			}

			if (!m_PrivatePatch3)
			{
				m_PrivatePatch3 = ScriptPatches::AddPatch("gb_contraband_sell"_J, ScriptPointer("HDPrivate3", "2C 01 ? ? 06 56 67 00 37 0B").Add(4), {0x2B, 0x55, 0x00, 0x00});
			}

			if (!m_PrivatePatch4)
			{
				m_PrivatePatch4 = ScriptPatches::AddPatch("gb_contraband_sell"_J, ScriptPointer("HDPrivate4", "2C 01 ? ? 06 56 54 00 37 10").Add(4), {0x2B, 0x55, 0x00, 0x00});
			}

			if (!m_PrivatePatch5)
			{
				m_PrivatePatch5 = ScriptPatches::AddPatch("gb_contraband_sell"_J, ScriptPointer("HDPrivate5", "2C 01 ? ? 06 56 5C 00 37 0A").Add(4), {0x2B, 0x55, 0x00, 0x00});
			}

			m_RivalPatch->Enable();
			m_PrivatePatch1->Enable();
			m_PrivatePatch2->Enable();
			m_PrivatePatch3->Enable();
			m_PrivatePatch4->Enable();
			m_PrivatePatch5->Enable();
		}

		virtual void OnDisable() override
		{
			if (m_RivalPatch)
				m_RivalPatch->Disable();

			if (m_PrivatePatch1)
				m_PrivatePatch1->Disable();

			if (m_PrivatePatch2)
				m_PrivatePatch2->Disable();

			if (m_PrivatePatch3)
				m_PrivatePatch3->Disable();

			if (m_PrivatePatch4)
				m_PrivatePatch4->Disable();

			if (m_PrivatePatch5)
				m_PrivatePatch5->Disable();
		}
	};
	static HighDemandSell _HighDemandSell{"high_demand_sell", "High Demand Sell", "Enables High Demand bonus in private sessions by forcing 25 rivals and bypassing the private session check"};
}
