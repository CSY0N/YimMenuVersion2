#include "Peaceful.hpp"
#include "game/frontend/items/Items.hpp"

namespace YimMenu::Submenus
{
    std::shared_ptr<Category> BuildPeacefulMenu()
    {
        auto menu = std::make_shared<Category>("Peaceful");

        auto friendly = std::make_shared<Group>("Friendly", 1);

        friendly->AddItem(std::make_shared<PlayerCommandItem>("copyoutfit"_J));

        menu->AddItem(friendly);

        return menu;
    }
}
