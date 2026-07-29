#include "core/commands/Command.hpp"
#include "game/gta/Natives.hpp"
#include "game/gta/ScriptGlobal.hpp"

namespace YimMenu::Features
{
    class UnlockKortzHeistPaintings : public Command
    {
        static constexpr int GLOBAL_BASE = 262145;
        using Command::Command;
        virtual void OnCall() override    
        {
            if (*Pointers.IsSessionStarted)
            {
                *ScriptGlobal(GLOBAL_BASE).At(38278).As<int*>() = 1;// Sod Off
                *ScriptGlobal(GLOBAL_BASE).At(38279).As<int*>() = 1;// Cooked
                *ScriptGlobal(GLOBAL_BASE).At(38280).As<int*>() = 1;// The Great Circle Back
                *ScriptGlobal(GLOBAL_BASE).At(38281).As<int*>() = 1;// Don't Forgo These Blueprints
                *ScriptGlobal(GLOBAL_BASE).At(38282).As<int*>() = 1;// Do You See Me
                *ScriptGlobal(GLOBAL_BASE).At(38283).As<int*>() = 1;// La Duchesse
                *ScriptGlobal(GLOBAL_BASE).At(38284).As<int*>() = 1;// Explain Yourself
                *ScriptGlobal(GLOBAL_BASE).At(38285).As<int*>() = 1;// The Chief
                *ScriptGlobal(GLOBAL_BASE).At(38286).As<int*>() = 1;// Canis Hominem Edit
                *ScriptGlobal(GLOBAL_BASE).At(38287).As<int*>() = 1;// Orange Crush
                *ScriptGlobal(GLOBAL_BASE).At(38288).As<int*>() = 1;// With Friends Like These
                *ScriptGlobal(GLOBAL_BASE).At(38289).As<int*>() = 1;// Het Gouden Hondje
                *ScriptGlobal(GLOBAL_BASE).At(38290).As<int*>() = 1;// Swingset Study No. LXIX
                *ScriptGlobal(GLOBAL_BASE).At(38291).As<int*>() = 1;// The Hunter Becomes the Hunted
            }
        }
    };
    static UnlockKortzHeistPaintings _UnlockKortzHeistPaintings{"KortzHeistPaintings", "Kortz Heist Paintings", "Unlock The Kortz Heist Secondary Target Paintings For The Mansion"};

}
