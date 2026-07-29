#include "core/commands/Command.hpp"
#include "game/pointers/Pointers.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/gta/ScriptGlobal.hpp"
#include "game/backend/PersonalVehicles.hpp"
namespace YimMenu
{
	class deletePersonalVehicle : public Command
	{
		using Command::Command;

		virtual void OnCall() override
		{
			FiberPool::Push([] {
				ScriptGlobal request(2733326);
				*request.At(472).As<int*>() = PersonalVehicles::GetCurrent()->GetId();
				*request.At(473).As<int*>() = 1;
				*request.At(474).As<int*>() = 0;
			});

		}
	};

	static deletePersonalVehicle _deletePersonalVehicle{"deletePV", "Delete Active Personal Vehicle", "Delete Active Personal Vehicle"};
}
