#include "UIManager.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/frontend/Menu.hpp"
#include "core/commands/ListCommand.hpp"
#include <imgui_internal.h>

namespace YimMenu::Features
{
	static const std::vector<std::pair<int, const char*>> g_StyleOptions = {
		{0, "Classic"},
		{1, "Modern"},
		{2, "Modern (Vertical)"},
	    {3, "Modern (Modular)"},
	};

	// Expose as global reference so other code can use it
	static ListCommand _StyleSelector{
		"styleselector",
		"UI Style",
		"Choose the UI style",
		g_StyleOptions,
		0};
}

namespace YimMenu
{
	static void DrawMenuBackground()
{
    if (!Menu::customMenuBackground.GetState())
        return;

    if (!Menu::g_BackgroundTexture.IsValid())
        return;

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (!window)
        return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();

    drawList->AddImage(
        Menu::g_BackgroundTexture.GetTextureID(),
        windowPos,
        ImVec2(
            windowPos.x + windowSize.x,
            windowPos.y + windowSize.y
        ),
        ImVec2(0.0f, 0.0f),
        ImVec2(1.0f, 1.0f),
        IM_COL32(255, 255, 255, 160)
    );
}

	void UIManager::AddSubmenuImpl(const std::shared_ptr<Submenu>&& submenu)
	{
		if (!m_ActiveSubmenu)
			m_ActiveSubmenu = submenu;

		m_Submenus.push_back(std::move(submenu));
	}

	void UIManager::RemoveSubmenuImpl(const std::shared_ptr<Submenu>& submenu)
	{
		std::erase(m_Submenus, submenu);

		if (m_ActiveSubmenu == submenu)
			m_ActiveSubmenu = m_Submenus.empty() ? nullptr : m_Submenus.front();
	}

	void UIManager::SetActiveSubmenuImpl(const std::shared_ptr<Submenu> submenu)
	{
		m_ActiveSubmenu = submenu;
	}

	void UIManager::DrawImpl()
	{
		int selectedIndex = Features::_StyleSelector.GetState();

		// Render the selected theme based on the index, append when adding new themes
		switch (static_cast<UITheme>(selectedIndex))
		{
		case UITheme::Classic:
			RenderClassicTheme();
			break;
		case UITheme::Modern:
			RenderModernTheme();
			break;
		case UITheme::ModernV:
			RenderModernVTheme();
			break;
		case UITheme::Modular:
			RenderModularTheme();
			break;
		default:
			RenderClassicTheme(); // Default theme
			break;
		}
	}

	std::shared_ptr<Submenu> UIManager::GetActiveSubmenuImpl()
	{
		return m_ActiveSubmenu;
	}

	std::shared_ptr<Category> UIManager::GetActiveCategoryImpl()
	{
		if (m_ActiveSubmenu)
		{
			return m_ActiveSubmenu->GetActiveCategory();
		}

		return nullptr;
	}
}
