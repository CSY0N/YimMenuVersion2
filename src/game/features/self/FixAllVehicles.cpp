// https://github.com/Mr-X-GTA/YimMenu/blob/master/src/util/mobile.hpp

#include "core/commands/Command.hpp"
#include "game/gta/Natives.hpp"
#include "game/gta/Bits.hpp"
#include "game/gta/ScriptGlobal.hpp"


namespace YimMenu::Features
{
	static inline const ScriptGlobal vehicle_global(1583778);
	enum eVehicleFlags
	{
		TRIGGER_SPAWN_TOGGLE = 1 << 0,
		DESTROYED = 1 << 1,
		HAS_INSURANCE = 1 << 2,
		UNK0 = 1 << 3,
		IMPOUNDED = 1 << 6,
		UNK1 = 1 << 10,
		SPAWN_AT_MORS_MUTUAL = 1 << 11,
		UNK2 = 1 << 16
	};

	inline bool fix_index(int veh_idx, bool spawn_veh = false)
	{
		bool can_be_fixed = Bits::HasBitsSets(vehicle_global.At(veh_idx, 143).At(104).As<int*>(), eVehicleFlags::DESTROYED | eVehicleFlags::HAS_INSURANCE);

		if (can_be_fixed)
		{
			Bits::ClearBits(vehicle_global.At(veh_idx, 143).At(104).As<int*>(), eVehicleFlags::DESTROYED | eVehicleFlags::IMPOUNDED | eVehicleFlags::UNK2);

			if (spawn_veh)
			{
				Bits::SetBits(vehicle_global.At(veh_idx, 143).At(104).As<int*>(), eVehicleFlags::TRIGGER_SPAWN_TOGGLE | eVehicleFlags::SPAWN_AT_MORS_MUTUAL);
			}
		}
		return can_be_fixed;
	}

	inline int fix_all()
	{
		int fixed_count = 0;

		const int arr_size = *vehicle_global.As<int*>();
		for (int i = 0; i < arr_size; i++)
		{
			if (fix_index(i, true))
			{
				fixed_count++;
			}
		}

		return fixed_count;
	}
	class FixAllVehicles : public Command
	{
		using Command::Command;

		void OnCall() override
		{
			fix_all();
		}
	};

	static FixAllVehicles _FixAllVehicles{"fixallvehicles", "Fix All Vehicles", "Repairs all destroyed vehicles through Mors Mutual Insurance."};
}
