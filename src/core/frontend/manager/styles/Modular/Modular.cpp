#include <algorithm>
#include "game/pointers/Pointers.hpp"
#include "game/frontend/Menu.hpp"
#include "core/frontend/manager/UIManager.hpp"
#include "game/frontend/submenus/Settings/GUISettings.hpp"

namespace YimMenu
{
	void RenderModularTheme()
	{
		YimMenu::SyncColorCommandsToStyle();

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* drawList = ImGui::GetBackgroundDrawList();

		const auto& submenus = UIManager::GetSubmenus();
		auto activeSubmenu = UIManager::GetActiveSubmenu();

		const float bubblePadding = 10.0f;
		const float leftOffset = 25.0f;
		const float topOffset = 25.0f;
		const float defaultWidth = 80.0f;
		const float defaultHeight = 80.0f;

		for (size_t i = 0; i < submenus.size(); ++i)
		{
			auto& submenu = submenus[i];

			std::string windowId = "NavItem_" + submenu->m_Name;

			ImVec2 startPos(leftOffset, topOffset + i * (defaultHeight + bubblePadding));
			ImVec2 defaultSize(defaultWidth, defaultHeight);

			ImGui::SetNextWindowPos(startPos, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(defaultSize, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints(ImVec2(60, 80), ImVec2(200, 200));
			ImGui::SetNextWindowBgAlpha(0.0f);

			ImGuiWindowFlags navFlags =
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoScrollbar;

			if (ImGui::Begin(windowId.c_str(), nullptr, navFlags))
			{
				ImVec2 winPos = ImGui::GetWindowPos();
				ImVec2 winSize = ImGui::GetWindowSize();

				ImVec2 center(winPos.x + winSize.x * 0.5f, winPos.y + winSize.y * 0.5f - 10.0f);

				ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Border);
				ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
				ImU32 buttonColor = ImGui::GetColorU32(ImGuiCol_Button);
				ImU32 hoverColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
				ImU32 activeColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);

				ImVec2 bubbleSize = winSize - ImVec2(20, 30);
				ImVec2 bubblePos(winPos.x + (winSize.x - bubbleSize.x) * 0.5f, winPos.y + 10.0f);

				ImGui::SetCursorScreenPos(bubblePos);
				ImGui::InvisibleButton(("##bubble_" + submenu->m_Name).c_str(), bubbleSize);

				bool hovered = ImGui::IsItemHovered();
				bool clicked = ImGui::IsItemClicked();

				if (clicked)
				{
					if (activeSubmenu == submenu)
					{
						UIManager::SetShowContentWindow(!UIManager::ShowingContentWindow());
					}
					else
					{
						UIManager::SetActiveSubmenu(submenu);
						UIManager::SetShowContentWindow(true);
						activeSubmenu = submenu;
					}
				}

				ImU32 bubbleBgColor = submenu == activeSubmenu ? activeColor : (hovered ? hoverColor : buttonColor);

				drawList->AddRectFilled(bubblePos, bubblePos + bubbleSize, bubbleBgColor, 10.0f);
				drawList->AddRect(bubblePos, bubblePos + bubbleSize, borderColor, 10.0f, 0, 1.5f);

				ImGui::PushFont(Menu::Font::g_AwesomeFont);

				float iconScale = std::clamp(bubbleSize.y * 0.4f, 14.0f, 32.0f);
				ImVec2 iconSize = ImGui::CalcTextSize(submenu->m_Icon.c_str());
				ImVec2 iconPos(center.x - iconSize.x * 0.5f, bubblePos.y + bubbleSize.y * 0.5f - iconSize.y * 0.5f);

				drawList->AddText(Menu::Font::g_AwesomeFont, iconScale, iconPos, textColor, submenu->m_Icon.c_str());

				ImGui::PopFont();

				ImVec2 labelSize = ImGui::CalcTextSize(submenu->m_Name.c_str());
				ImVec2 labelPos(center.x - labelSize.x * 0.5f, bubblePos.y + bubbleSize.y + 6.0f);

				drawList->AddText(labelPos, textColor, submenu->m_Name.c_str());
			}

			ImGui::End();
		}

		if (activeSubmenu)
		{
			ImGui::SetNextWindowSizeConstraints(ImVec2(100, 40), ImVec2(io.DisplaySize.x * 0.9f, 200));

			ImGuiWindowFlags categoryFlags =
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_AlwaysAutoResize;

			if (ImGui::Begin("##CategorySelectorWindow", nullptr, categoryFlags))
			{
				activeSubmenu->DrawCategorySelectors();
			}

			ImGui::End();
		}

		if (UIManager::ShowingContentWindow() && activeSubmenu)
		{
			ImVec2 defaultSize(*Pointers.ScreenResX / 2.8f, *Pointers.ScreenResY / 2.5f);

			const bool useCustomBackground =
				Menu::customMenuBackground.GetState() &&
				Menu::g_BackgroundTexture.IsValid();

			ImGui::SetNextWindowBgAlpha(useCustomBackground ? 0.0f : 1.0f);
			ImGui::SetNextWindowSize(defaultSize, ImGuiCond_FirstUseEver);

			ImGuiWindowFlags contentFlags =
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoTitleBar;

			if (ImGui::Begin("##OptionsContentWindow", nullptr, contentFlags))
			{
				ImVec2 winPos = ImGui::GetWindowPos();
				ImVec2 winSize = ImGui::GetWindowSize();

				if (useCustomBackground)
				{
					ImDrawList* contentDraw = ImGui::GetWindowDrawList();

					contentDraw->AddImage(Menu::g_BackgroundTexture.GetTextureID(), winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE);
				}

				bool pushedFont = false;

				if (ImFont* optionsFont = UIManager::GetOptionsFont())
				{
					ImGui::PushFont(optionsFont);
					pushedFont = true;
				}

				activeSubmenu->Draw();

				if (pushedFont)
					ImGui::PopFont();
			}

			ImGui::End();
		}
	}
}
