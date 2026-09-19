#include "game/commands/PlayerCommand.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Features
{
    class CopyOutfit : public PlayerCommand
    {
        using PlayerCommand::PlayerCommand;

        void OnCall(Player player) override
        {
            if (!player.IsValid())
                return;

            if (!player.GetPed())
                return;

            for (int i = 0; i < 12; i++)
            {
                PED::SET_PED_COMPONENT_VARIATION(PLAYER::PLAYER_PED_ID(), i, PED::GET_PED_DRAWABLE_VARIATION(player.GetPed().GetHandle(), i), PED::GET_PED_TEXTURE_VARIATION(player.GetPed().GetHandle(), i), PED::GET_PED_PALETTE_VARIATION(player.GetPed().GetHandle(), i));
            }

            for (int i = 0; i < 3; i++)
            {
                const int drawable = PED::GET_PED_PROP_INDEX(player.GetPed().GetHandle(), i, 0);

                if (drawable == -1)
                {
                    PED::CLEAR_PED_PROP(PLAYER::PLAYER_PED_ID(), i, 0);
                    continue;
                }

                PED::SET_PED_PROP_INDEX(PLAYER::PLAYER_PED_ID(), i, drawable, PED::GET_PED_PROP_TEXTURE_INDEX(player.GetPed().GetHandle(), i), true, 0);
            }
        }
    };

    static CopyOutfit _CopyOutfit{"copyoutfit", "Copy Outfit", "Copies the selected player's outfit", 0, false};
}
