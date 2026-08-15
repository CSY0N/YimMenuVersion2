#include "game/pointers/Pointers.hpp"
#include "game/frontend/Menu.hpp"
#include "core/frontend/manager/UIManager.hpp"

namespace YimMenu
{
	inline ImU32 ToU32(const ImVec4& c)
	{
		return ImGui::ColorConvertFloat4ToU32(c);
	}
	static constexpr float TOPBAR_HEIGHT = 64.0f;
	static constexpr float ITEM_WIDTH = 60.0f;
	static constexpr float ICON_SIZE = 20.0f;
	static constexpr float ROUNDING = 8.0f;
	void RenderModernTheme()
	{
		ImGuiIO& io = ImGui::GetIO();

		auto activeSubmenu = YimMenu::UIManager::GetActiveSubmenu();
		if (!activeSubmenu || !YimMenu::UIManager::ShowingContentWindow())
			return;

		float windowWidth = *YimMenu::Pointers.ScreenResX / 2.5f;
		float windowHeight = *YimMenu::Pointers.ScreenResY / 2.5f;

		ImVec2 windowPos(
		    (*YimMenu::Pointers.ScreenResX - windowWidth) * 0.5f,
		    *YimMenu::Pointers.ScreenResY / 5.0f);

		ImGui::SetNextWindowPos(windowPos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_FirstUseEver);
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
		if (!ImGui::Begin("##Categories&Content", nullptr, flags))
		{
			ImGui::End();
			return;
		}

		ImDrawList* draw = ImGui::GetWindowDrawList();
		ImVec2 winPos = ImGui::GetWindowPos();
		float winW = ImGui::GetWindowWidth();

		constexpr float bubbleSize = 52.0f;
		constexpr float bubbleSpacing = 70.0f;
		constexpr float rounding = 8.0f;
		constexpr float barHeight = bubbleSize + 24.0f;

		const auto& submenus = YimMenu::UIManager::GetSubmenus();

		ImGui::BeginChild("##TopIconBar", ImVec2(0, barHeight), false);

		ImVec2 barMin = ImGui::GetWindowPos();
		ImVec2 barMax = barMin + ImGui::GetWindowSize();
		float startX =barMin.x + (winW - (bubbleSpacing * submenus.size())) * 0.5f;

		for (size_t i = 0; i < submenus.size(); ++i)
		{
			auto& submenu = submenus[i];
			bool isActive = submenu == activeSubmenu;

			ImVec2 bubblePos(startX + i * bubbleSpacing,barMin.y + 12.0f);

			ImGui::SetCursorScreenPos(bubblePos);
			ImGui::PushID((int)i);

			ImGui::InvisibleButton("##bubble", ImVec2(bubbleSize, bubbleSize));
			bool hovered = ImGui::IsItemHovered();
			bool clicked = ImGui::IsItemClicked();

			if (clicked)
				YimMenu::UIManager::SetActiveSubmenu(submenu);

			draw->AddRectFilled(
			    bubblePos,
			    bubblePos + ImVec2(bubbleSize, bubbleSize),
			    ToU32(isActive ? ItemActiveBg : hovered ? ItemHoveredBg :SidebarBg),
			    rounding);

			if (isActive)
			{
				draw->AddRectFilled(ImVec2(bubblePos.x, bubblePos.y + bubbleSize - 3.f),ImVec2(bubblePos.x + bubbleSize, bubblePos.y + bubbleSize),ToU32(Accent),2.0f);
			}
			ImGui::PushFont(YimMenu::Menu::Font::g_AwesomeFont);
			ImVec2 iconSize = ImGui::CalcTextSize(submenu->m_Icon.c_str());
			ImVec2 iconPos(bubblePos.x + (bubbleSize - iconSize.x) * 0.5f,bubblePos.y + (bubbleSize - iconSize.y) * 0.5f);
			draw->AddText(YimMenu::Menu::Font::g_AwesomeFont,ICON_SIZE,iconPos,ToU32(isActive ? IconActive : hovered ? IconHovered:IconIdle),submenu->m_Icon.c_str());
			ImGui::PopFont();
			ImGui::PopID();
		}
		ImGui::EndChild();

		if (ImGui::BeginChild("##categorySelectors", ImVec2(0, 60), true))
		{
			activeSubmenu->DrawCategorySelectors();
		}
		ImGui::EndChild();


		if (ImGui::BeginChild("##options", ImVec2(0, 0), true))
		{
			if (ImFont* optionsFont = YimMenu::UIManager::GetOptionsFont())
				ImGui::PushFont(optionsFont);

			activeSubmenu->Draw();

			if (ImGui::GetFont() != ImGui::GetIO().FontDefault)
				ImGui::PopFont();
		}
		ImGui::EndChild();

		ImGui::End();
	}
}
