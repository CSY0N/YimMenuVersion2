#include "game/pointers/Pointers.hpp"
#include "game/frontend/Menu.hpp"
#include "core/frontend/manager/UIManager.hpp"
#include "core/commands/BoolCommand.hpp"
namespace YimMenu
{
	// static BoolCommand menuSnow{"menubgsnow","Menu Snow Background","Toggle snow particles in the menu background"};
	static BoolCommand menuPumpkins{"menubgpumpkins", "Menu Pumpkin Background", "Toggle falling pumpkins in the menu background"};

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
	/*static void RenderSnowInWindow(ImVec2 winPos, ImVec2 winSize)
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
	}*/
	static void RenderPumpkin(ImVec2 winPos, ImVec2 winSize)
	{
		if (!menuPumpkins.GetState() || !UIManager::ShowingContentWindow())
		{
			g_Snow.clear();
			g_SnowInit = false;
			return;
		}

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* draw = ImGui::GetWindowDrawList();

		if (!g_SnowInit)
			InitSnow(45, winPos, winSize);

		ImVec2 clipMin = winPos;
		ImVec2 clipMax = winPos + winSize;

		draw->PushClipRect(clipMin, clipMax, true);

		for (auto& p : g_Snow)
		{
			p.pos.y += p.speed * 0.65f * io.DeltaTime;
			p.pos.x += sinf(
			               (float)ImGui::GetTime() * 1.5f + p.pos.y * 0.01f)
			    * 8.0f * io.DeltaTime;

			if (p.pos.y > clipMax.y + 10.0f)
			{
				p.pos.y = clipMin.y - 10.0f;
				p.pos.x = clipMin.x + static_cast<float>(rand() % static_cast<int>(winSize.x));
			}

			if (p.pos.x < clipMin.x || p.pos.x > clipMax.x)
				continue;

			const float size = p.size * 2.3f;

			// Pumpkin glow
			draw->AddCircleFilled(
			    p.pos,
			    size + 3.0f,
			    IM_COL32(255, 100, 0, 25));

			// Pumpkin body
			draw->AddEllipseFilled(
			    p.pos,
			    ImVec2(size, size * 0.78f),
			    IM_COL32(235, 90, 15, 210));

			// Pumpkin sections
			draw->AddEllipse(
			    p.pos,
			    ImVec2(size * 0.50f, size * 0.78f),
			    IM_COL32(255, 145, 25, 180),
			    0.0f,
			    0,
			    1.0f);

			// Stem
			draw->AddRectFilled(
			    ImVec2(p.pos.x - 1.5f, p.pos.y - size * 0.95f),
			    ImVec2(p.pos.x + 1.5f, p.pos.y - size * 0.60f),
			    IM_COL32(75, 110, 45, 230),
			    1.0f);

			// Eyes
			const float eyeY = p.pos.y - size * 0.15f;

			draw->AddTriangleFilled(
			    ImVec2(p.pos.x - size * 0.50f, eyeY),
			    ImVec2(p.pos.x - size * 0.15f, eyeY),
			    ImVec2(p.pos.x - size * 0.32f, eyeY + size * 0.30f),
			    IM_COL32(35, 20, 10, 230));

			draw->AddTriangleFilled(
			    ImVec2(p.pos.x + size * 0.15f, eyeY),
			    ImVec2(p.pos.x + size * 0.50f, eyeY),
			    ImVec2(p.pos.x + size * 0.32f, eyeY + size * 0.30f),
			    IM_COL32(35, 20, 10, 230));

			// Jack-o'-lantern mouth
			draw->AddLine(
			    ImVec2(p.pos.x - size * 0.45f, p.pos.y + size * 0.28f),
			    ImVec2(p.pos.x, p.pos.y + size * 0.48f),
			    IM_COL32(35, 20, 10, 230),
			    1.5f);

			draw->AddLine(
			    ImVec2(p.pos.x, p.pos.y + size * 0.48f),
			    ImVec2(p.pos.x + size * 0.45f, p.pos.y + size * 0.28f),
			    IM_COL32(35, 20, 10, 230),
			    1.5f);
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
		RenderPumpkin(winPos, winSize);
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
