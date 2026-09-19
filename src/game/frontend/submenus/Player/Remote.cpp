#include "Remote.hpp"

namespace YimMenu::Submenus
{
    std::shared_ptr<Category> BuildRemoteMenu()
    {
        auto menu = std::make_shared<Category>("Remote");

        auto actions = std::make_shared<Group>("Actions", 1);
        actions->AddItem(std::make_shared<PlayerCommandItem>("ceokick"_J));
        actions->AddItem(std::make_shared<PlayerCommandItem>("sendpizza"_J));

        auto missions = std::make_shared<Group>("Force Mission");
        missions->AddItem(std::make_shared<ListCommandItem>("forcemissiontype"_J));
        missions->AddItem(std::make_shared<PlayerCommandItem>("forcemission"_J));


        auto notifications = std::make_shared<Group>("Notifications");
        notifications->AddItem(std::make_shared<ListCommandItem>("notificationtype"_J));
        notifications->AddItem(std::make_shared<PlayerCommandItem>("sendnotification"_J));

        menu->AddItem(actions);
		menu->AddItem(missions);
        menu->AddItem(notifications);

        return menu;
    }
}
