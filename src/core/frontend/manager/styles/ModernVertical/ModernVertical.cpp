#include "game/pointers/Pointers.hpp"
#include "game/frontend/Menu.hpp"
#include "core/frontend/manager/UIManager.hpp"
#include "core/frontend/manager/UIItem.hpp"

namespace YimMenu
{
	inline ImU32 ToU32(const ImVec4& c)
	{
		return ImGui::ColorConvertFloat4ToU32(c);
	}
	static constexpr float SIDEBAR_WIDTH = 56.0f;
	static constexpr float ITEM_HEIGHT = 40.0f;
	static constexpr float ICON_SIZE = 20.0f;
	static constexpr float ROUNDING = 8.0f;
	void RenderModernVTheme()
	{
		auto activeSubmenu = UIManager::GetActiveSubmenu();
		const auto& submenus = UIManager::GetSubmenus();

		if (!UIManager::ShowingContentWindow() || !activeSubmenu)
			return;

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

		if (!ImGui::Begin("##Categories&Content", nullptr, flags))
		{
			ImGui::End();
			return;
		}

		ImDrawList* draw = ImGui::GetWindowDrawList();
		ImGui::BeginChild("##Sidebar", ImVec2(SIDEBAR_WIDTH, 0), false);

		ImVec2 sidebarMin = ImGui::GetWindowPos();
		ImVec2 sidebarMax = sidebarMin + ImGui::GetWindowSize();

		draw->AddRectFilled(
		    sidebarMin,
		    sidebarMax,
		    ToU32(SidebarBg),
		    ROUNDING);

		for (size_t i = 0; i < submenus.size(); ++i)
		{
			auto& submenu = submenus[i];
			bool active = submenu == activeSubmenu;

			ImGui::PushID((int)i);
			ImGui::SetCursorPosY(10.f + i * ITEM_HEIGHT);

			ImVec2 itemPos = ImGui::GetCursorScreenPos();
			ImVec2 itemSize = {SIDEBAR_WIDTH, ITEM_HEIGHT};

			ImGui::InvisibleButton("##tab", itemSize);
			bool hovered = ImGui::IsItemHovered();
			bool clicked = ImGui::IsItemClicked();

			if (clicked)
				UIManager::SetActiveSubmenu(submenu);
			if (active || hovered)
			{
				draw->AddRectFilled(itemPos,itemPos + itemSize,ToU32(active ? ItemActiveBg : ItemHoveredBg),ROUNDING);
			}
			if (active)
			{
				draw->AddRectFilled(itemPos,ImVec2(itemPos.x + 3.f, itemPos.y + itemSize.y),ToU32(Accent));
			}
			ImGui::PushFont(Menu::Font::g_AwesomeFont);
			ImVec2 iconSize = ImGui::CalcTextSize(submenu->m_Icon.c_str());
			ImVec2 iconPos( itemPos.x + (SIDEBAR_WIDTH - iconSize.x) * 0.5f,itemPos.y + (ITEM_HEIGHT - iconSize.y) * 0.5f);
			draw->AddText(Menu::Font::g_AwesomeFont,ICON_SIZE,iconPos,ToU32( active ? IconActive :hovered ? IconHovered :IconIdle),submenu->m_Icon.c_str());
			ImGui::PopFont();
			ImGui::PopID();
		}

		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("##Content", ImVec2(0, 0), true);

		ImGui::BeginChild("##Categories", ImVec2(0, 60), false);
		activeSubmenu->DrawCategorySelectors();
		ImGui::EndChild();

		ImGui::Separator();

		ImGui::BeginChild("##Options", ImVec2(0, 0), false);

		if (ImFont* f = UIManager::GetOptionsFont())
			ImGui::PushFont(f);

		activeSubmenu->Draw();

		if (ImGui::GetFont() != ImGui::GetIO().FontDefault)
			ImGui::PopFont();

		ImGui::EndChild();
		ImGui::EndChild();
		ImGui::End();
	}
}
