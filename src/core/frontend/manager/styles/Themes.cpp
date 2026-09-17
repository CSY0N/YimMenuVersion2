#include "Themes.hpp"
#include "core/commands/ColorCommand.hpp"
#include "game/frontend/submenus/Settings/GUISettings.hpp"

namespace YimMenu
{
	void DefaultStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.20f, 0.00f, 0.95f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.15f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.18f, 0.00f, 0.95f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.00f, 0.00f, 0.30f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.30f, 0.00f, 0.54f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.00f, 0.40f, 0.00f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.50f, 0.00f, 0.67f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.20f, 0.00f, 0.57f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.80f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.80f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.80f, 0.00f, 0.00f, 0.40f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 0.30f, 0.00f, 0.31f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.40f, 0.00f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.50f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.80f, 0.00f, 0.00f, 0.50f);
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.90f, 0.00f, 0.00f, 0.78f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.80f, 0.60f, 0.00f, 0.25f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.90f, 0.70f, 0.00f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.80f, 0.00f, 0.95f);
		style.Colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.20f, 0.00f, 0.86f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.80f, 0.00f, 0.00f, 0.80f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.15f, 0.00f, 0.97f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.00f, 0.30f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.80f, 0.00f, 0.00f, 0.35f);
		style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		style.GrabRounding = style.FrameRounding = style.ChildRounding = style.WindowRounding = 8.0f;
	}

	void HeritageDayStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_Text] = ImVec4(0.96f, 0.91f, 0.78f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.58f, 0.62f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.025f, 0.075f, 0.105f, 0.98f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.035f, 0.105f, 0.125f, 0.50f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.030f, 0.085f, 0.110f, 0.98f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.030f, 0.120f, 0.155f, 0.95f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.15f, 0.70f, 0.68f, 0.55f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.055f, 0.20f, 0.22f, 0.75f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.10f, 0.42f, 0.42f, 0.85f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.58f, 0.55f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.020f, 0.060f, 0.085f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.60f, 0.22f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.020f, 0.050f, 0.070f, 0.85f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.015f, 0.045f, 0.060f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.15f, 0.55f, 0.55f, 0.85f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.20f, 0.72f, 0.68f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.82f, 0.75f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.62f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.75f, 0.32f, 0.14f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.55f, 0.18f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.42f, 0.42f, 0.60f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.68f, 0.27f, 0.12f, 0.90f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.90f, 0.43f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.08f, 0.34f, 0.36f, 0.65f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.66f, 0.26f, 0.12f, 0.85f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.82f, 0.37f, 0.13f, 1.00f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.64f, 0.62f, 0.55f);
		style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.88f, 0.43f, 0.15f, 0.85f);
		style.Colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 0.58f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.70f, 0.30f, 0.12f, 0.35f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.88f, 0.43f, 0.15f, 0.75f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.58f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_Tab] = ImVec4(0.035f, 0.17f, 0.20f, 0.95f);
		style.Colors[ImGuiCol_TabHovered] = ImVec4(0.68f, 0.27f, 0.12f, 0.90f);
		style.Colors[ImGuiCol_TabActive] = ImVec4(0.12f, 0.48f, 0.48f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.025f, 0.10f, 0.13f, 0.95f);
		style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.08f, 0.30f, 0.32f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.25f, 0.75f, 0.70f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.58f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.78f, 0.34f, 0.13f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.55f, 0.18f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.15f, 0.58f, 0.55f, 0.45f);
		style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 0.62f, 0.22f, 0.95f);
		style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.95f, 0.50f, 0.16f, 1.00f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.40f, 0.85f, 0.80f, 0.80f);
		style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.01f, 0.05f, 0.07f, 0.70f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.01f, 0.04f, 0.05f, 0.82f);
		style.GrabRounding = style.FrameRounding = style.ChildRounding = style.WindowRounding = 8.0f;
	}

	void SetupStyle()
	{
		// Apply default style first
		HeritageDayStyle();

		// Initialize the color/rounding commands and load saved settings
		InitializeColorCommands(); // This will call LoadSettings internally

		// Apply loaded colors/rounding to ImGui
		ApplyThemeToImGui();
	}
}
