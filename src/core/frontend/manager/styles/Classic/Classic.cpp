#include "game/pointers/Pointers.hpp"
#include "game/frontend/Menu.hpp"
#include "core/frontend/manager/UIManager.hpp"
#include "core/commands/BoolCommand.hpp"
namespace YimMenu
{
	static BoolCommand menuSnow{"menubgsnow","Menu Snow Background","Toggle snow particles in the menu background"};
	struct SnowParticle
	{
		ImVec2 pos;
		float speed;
		float size;
	};
	static std::vector<SnowParticle> g_Snow{};
	static bool g_SnowInit = false;
	static void InitSnow(int count, ImVec2 winPos, ImVec2 winSize)
	{
		static bool seeded = false;
		if (!seeded)
		{
			srand((unsigned)time(nullptr));
			seeded = true;
		}
		g_Snow.clear();
		g_Snow.reserve(count);
		for (int i = 0; i < count; ++i)
		{
			g_Snow.push_back({ImVec2(winPos.x + (float)(rand() % (int)winSize.x),winPos.y + (float)(rand() % (int)winSize.y)),
			    20.0f + (rand() % 40),
			    1.5f + (rand() % 3)});
		}
		g_SnowInit = true;
	}
	static void RenderSnowInWindow(ImVec2 winPos, ImVec2 winSize)
	{
		if (!menuSnow.GetState() || !UIManager::ShowingContentWindow())
		{
			g_Snow.clear();
			g_SnowInit = false;
			return;
		}
		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		if (!g_SnowInit)
			InitSnow(120, winPos, winSize);
		ImVec2 clipMin = winPos;
		ImVec2 clipMax = winPos + winSize;
		draw->PushClipRect(clipMin, clipMax, true);
		for (auto& p : g_Snow)
		{
			p.pos.y += p.speed * io.DeltaTime;

			if (p.pos.y > clipMax.y)
			{
				p.pos.y = clipMin.y - 5.0f;
				p.pos.x = clipMin.x + (float)(rand() % (int)winSize.x);
			}
			if (p.pos.x < clipMin.x || p.pos.x > clipMax.x)
				continue;
			//Soft
			draw->AddCircleFilled(
			    p.pos,
			    p.size + 1.5f,
			    IM_COL32(255, 255, 255, 40));

			draw->AddCircleFilled(
			    p.pos,
			    p.size,
			    IM_COL32(255, 255, 255, 120));
		}
		draw->PopClipRect();
	}
	static float LerpFloat(float a, float b, float t)
	{
		if (t < 0.f)
			t = 0.f;
		if (t > 1.f)
			t = 1.f;
		return a + (b - a) * t;
	}
	static float g_MenuAlpha = 0.0f;
	static float g_MenuSlide = 30.0f;
	static float g_ContentSlide = 0.0f;
	static decltype(UIManager::GetActiveSubmenu()) g_LastSubmenu{};
	void RenderClassicTheme()
	{
		ImGuiIO& io = ImGui::GetIO();
		float windowWidth = *Pointers.ScreenResX / 2.5f;
		float centerX = (*Pointers.ScreenResX - windowWidth) * 0.5f;
		float centerY = *Pointers.ScreenResY / 5.0f;
		ImVec2 windowSize(windowWidth,*Pointers.ScreenResY / 2.5f);
		const float animSpeed = 10.0f;
		float t = io.DeltaTime * animSpeed;
		g_MenuAlpha = LerpFloat(g_MenuAlpha, 1.0f, t);
		g_MenuSlide = LerpFloat(g_MenuSlide, 0.0f, t);
		ImGui::SetNextWindowBgAlpha(g_MenuAlpha);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(centerX, centerY + g_MenuSlide),ImGuiCond_FirstUseEver);
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
		if (!ImGui::Begin("##ClassicInputWindow", nullptr, flags))
		{
			ImGui::End();
			return;
		}
		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 winSize = ImGui::GetWindowSize();
		RenderSnowInWindow(winPos, winSize);
		if (ImGui::BeginChild("##submenus",ImVec2(120, ImGui::GetContentRegionAvail().y),true))
		{
			const auto& submenus = UIManager::GetSubmenus();
			auto activeSubmenu = UIManager::GetActiveSubmenu();
			for (auto& submenu : submenus)
			{
				if (ImGui::Selectable(submenu->m_Name.data(),submenu == activeSubmenu))
				{
					UIManager::SetActiveSubmenu(submenu);
					UIManager::SetShowContentWindow(true);
				}
			}
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::TextUnformatted("YimMenuV2");
		if (ImGui::BeginChild("##minisubmenus", ImVec2(0, 50), true))
		{
			auto activeSubmenu = UIManager::GetActiveSubmenu();
			if (activeSubmenu) activeSubmenu->DrawCategorySelectors();
		}
		ImGui::EndChild();

		auto activeSubmenu = UIManager::GetActiveSubmenu();
		if (activeSubmenu != g_LastSubmenu)
		{
			g_ContentSlide = 20.0f;
			g_LastSubmenu = activeSubmenu;
		}
		g_ContentSlide = LerpFloat(g_ContentSlide, 0.0f, t);
		ImGui::Dummy(ImVec2(g_ContentSlide, 0.0f));
		ImGui::SameLine(0.0f, 0.0f);
		if (ImGui::BeginChild("##options", ImVec2(0, 0), true))
		{
			if (ImFont* optionsFont = UIManager::GetOptionsFont())
				ImGui::PushFont(optionsFont);

			if (activeSubmenu)
				activeSubmenu->Draw();

			if (ImGui::GetFont() != ImGui::GetIO().FontDefault)
				ImGui::PopFont();
		}
		ImGui::EndChild();
		ImGui::EndGroup();
		ImGui::End();
	}
}
