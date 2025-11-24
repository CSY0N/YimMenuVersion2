#include "game/backend/Self.hpp"
#include "game/commands/PlayerCommand.hpp"
#include "core/commands/Vector3Command.hpp"

namespace YimMenu::Features
{
	static Vector3Command _PlayerTeleportCoordinate{"playertpcoord", "Player Teleport Coords", "The coordinates to teleport the player to"};

	class TeleportToCoord : public PlayerCommand
	{
		using PlayerCommand::PlayerCommand;

		virtual void OnCall(Player player) override
		{
			player.TeleportTo(_PlayerTeleportCoordinate.GetState());
		}
	};

	static TeleportToCoord _TeleportToCoord{"tpplayertocoord", "Teleport Player To Coord", "Teleports the player to the specified coordinates"};
}