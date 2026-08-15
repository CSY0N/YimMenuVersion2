#include "theme.hpp"
#include "core/frontend/manager/UIManager.hpp"

namespace YimMenu::theme
{
	static std::filesystem::path GetThemeConfigPath()
	{
		auto path = GetThemesPath().parent_path();
		path /= "theme.cfg";
		return path;
	}
	static void SaveLastTheme(const std::filesystem::path& theme)
	{
		std::ofstream out(GetThemeConfigPath(), std::ios::trunc);
		if (out.is_open())
			 out << theme.filename().string();
	}
	static std::optional<std::filesystem::path> LoadLastTheme()
	{
		 std::ifstream in(GetThemeConfigPath());
		  if (!in.is_open())
			   return std::nullopt;
		  std::string name;
		  std::getline(in, name);
		  if (name.empty())
			  return std::nullopt;
		  auto path = GetThemesPath() / name;
		  if (std::filesystem::exists(path))
			  return path;
		  return std::nullopt;
	}
	static bool ParseColorFromClipboard(ImVec4& out)
	{
		const char* clip = ImGui::GetClipboardText();
		if (!clip || !clip[0])
			 return false;
		 float r, g, b, a = 1.0f;
		if (sscanf_s(clip, "%f , %f , %f , %f", &r, &g, &b, &a) == 4 ||
			 sscanf_s(clip, "%f,%f,%f,%f", &r, &g, &b, &a) == 4)
		{
			out = ImVec4(r, g, b, a);
			return true;
		}
		if (sscanf_s(clip, "%f , %f , %f", &r, &g, &b) == 3 ||
			 sscanf_s(clip, "%f,%f,%f", &r, &g, &b) == 3)
		{
			 out = ImVec4(r, g, b, 1.0f);
			 return true;
		}
		if (clip[0] == '#') {
			 unsigned int hex = 0;
			  if (sscanf_s(clip + 1, "%x", &hex) == 1)  {
				  if (strlen(clip) == 7) {
					   out.x = ((hex >> 16) & 0xFF) / 255.f;
					   out.y = ((hex >> 8) & 0xFF) / 255.f;
					    out.z = (hex & 0xFF) / 255.f;
						out.w = 1.0f;
						 return true;
		            }
			else if (strlen(clip) == 9)  {
				out.x = ((hex >> 24) & 0xFF) / 255.f;
				out.y = ((hex >> 16) & 0xFF) / 255.f;
				out.z = ((hex >> 8) & 0xFF) / 255.f;
				out.w = (hex & 0xFF) / 255.f;
				return true;
            }
        }
    }
    return false;
}

bool DrawColorWheel(const char* id, ImVec4& color, float radius = 90.f)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw = window->DrawList;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(radius * 2.f, radius * 2.f);
    ImVec2 center = pos + ImVec2(radius, radius);

    ImGui::InvisibleButton(id, size);
    bool active = ImGui::IsItemActive();
    bool hovered = ImGui::IsItemHovered();

    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);

    bool changed = false;
    const int segments = 240;

    for (int i = 0; i < segments; ++i)
    {
        float a0 = (i / (float)segments) * IM_PI * 2.f;
        float a1 = ((i + 1) / (float)segments) * IM_PI * 2.f;

        ImVec2 p0 = center + ImVec2(cosf(a0), sinf(a0)) * radius;
        ImVec2 p1 = center + ImVec2(cosf(a1), sinf(a1)) * radius;

        ImU32 col0 = ImColor::HSV(i / (float)segments, 1.f, 1.f);
        ImU32 col1 = ImColor::HSV((i + 1) / (float)segments, 1.f, 1.f);

        draw->AddTriangleFilled(center, p0, p1, col0);
    }

    draw->AddCircleFilled(center, radius, IM_COL32(255, 255, 255, 35), segments);
    draw->AddCircle(center, radius + 2.f, IM_COL32(255, 255, 255, hovered ? 40 : 20), segments, 2.5f);

    if (active)
    {
        ImVec2 delta = io.MousePos - center;
        float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);

        if (dist <= radius)
        {
            float angle = atan2f(delta.y, delta.x);
            if (angle < 0.f)
                angle += IM_PI * 2.f;

            h = angle / (IM_PI * 2.f);
            s = ImClamp(dist / radius, 0.f, 1.f);
            v = 1.f;

            changed = true;
        }
    }

    float handle_angle = h * IM_PI * 2.f;
    float handle_radius = s * radius;
    ImVec2 handle_pos = center + ImVec2(cosf(handle_angle) * handle_radius, sinf(handle_angle) * handle_radius);

    draw->AddCircleFilled(handle_pos + ImVec2(1, 1), 7.f, IM_COL32(0, 0, 0, 120));
    draw->AddCircleFilled(handle_pos, 6.f, IM_COL32(255, 255, 255, 230));
    draw->AddCircle(handle_pos, 6.f, IM_COL32(0, 0, 0, 160), 0, 1.5f);

    if (changed)
        ImGui::ColorConvertHSVtoRGB(h, s, v, color.x, color.y, color.z);

    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Separator();

    if (ImGui::Button("Copy RGB"))
    {
        char buf[64];
        sprintf_s(buf, "%.3f, %.3f, %.3f", color.x, color.y, color.z);
        ImGui::SetClipboardText(buf);
    }

    ImGui::SameLine();

    if (ImGui::Button("Copy RGBA"))
    {
        char buf[64];
        sprintf_s(buf, "%.3f, %.3f, %.3f, %.3f",
            color.x, color.y, color.z, color.w);
        ImGui::SetClipboardText(buf);
    }

    ImGui::SameLine();

    if (ImGui::Button("Paste"))
    {
        if (ParseColorFromClipboard(color))
            changed = true;
    }

    return changed;
}
	bool ColorWheelPicker_UIM(const char* label, ImVec4& color)
	{
		bool changed = false;
		ImGui::PushID(label);
		ImGui::TextUnformatted(label);
		ImGui::SameLine(200);
		if (ImGui::ColorButton(
		        "##preview",
		        color,
		        ImGuiColorEditFlags_NoTooltip,
		        ImVec2(28, 14)))
		{
			ImGui::OpenPopup("##wheel_popup");
		}
		if (ImGui::BeginPopup("##wheel_popup",
		        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
		{
			ImGui::TextUnformatted(label);
			ImGui::Separator();
			ImGui::Spacing();
			float radius = 70.f;
			float wheel_w = radius * 2.f;
			float avail = ImGui::GetContentRegionAvail().x;
			float pad = (avail > wheel_w) ? (avail - wheel_w) * 0.5f : 0.f;
			if (pad > 0.f)
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad);
			changed |= DrawColorWheel("##wheel", color, radius);
			ImGui::Spacing();
			ImGui::SliderFloat("R", &color.x, 0.f, 1.f);
			ImGui::SliderFloat("G", &color.y, 0.f, 1.f);
			ImGui::SliderFloat("B", &color.z, 0.f, 1.f);
			ImGui::SliderFloat("A", &color.w, 0.f, 1.f);
			ImGui::Spacing();
			ImGui::Separator();
			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
		ImGui::PopID();
		return changed;
	}


	static std::filesystem::path GetThemesPath()
	{
		const char* appdata = std::getenv("APPDATA");
		if (!appdata)
			return {};
		std::filesystem::path path = appdata;
		path /= "YimMenuV2";
		path /= "Themes";
		std::filesystem::create_directories(path);
		return path;
	}
	static void WriteUIColor(std::ofstream& out, const char* name, const ImVec4& c, bool comma = true)
	{
		out << "    \"" << name << "\": ["
		    << c.x << ", " << c.y << ", " << c.z << ", " << c.w << "]";
		if (comma)
			out << ",";
		out << "\n";
	}
	static void ReadUIColor(ImVec4& c, const std::vector<float>& v, size_t& i)
	{
		c = {v[i++], v[i++], v[i++], v[i++]};
	}
	void SaveTheme(const std::filesystem::path& file)
	{
		ImGuiStyle& s = ImGui::GetStyle();
		std::ofstream out(file, std::ios::trunc);
		if (!out.is_open())
			return;
		out << "{\n";
		out << "  \"style\": {\n";
		out << "    \"alpha\": " << s.Alpha << ",\n";
		out << "    \"window_rounding\": " << s.WindowRounding << ",\n";
		out << "    \"frame_rounding\": " << s.FrameRounding << ",\n";
		out << "    \"popup_rounding\": " << s.PopupRounding << ",\n";
		out << "    \"scrollbar_rounding\": " << s.ScrollbarRounding << ",\n";
		out << "    \"grab_rounding\": " << s.GrabRounding << ",\n";
		out << "    \"scrollbar_size\": " << s.ScrollbarSize << ",\n";
		out << "    \"window_padding\": [" << s.WindowPadding.x << ", " << s.WindowPadding.y << "],\n";
		out << "    \"frame_padding\": [" << s.FramePadding.x << ", " << s.FramePadding.y << "],\n";
		out << "    \"item_spacing\": [" << s.ItemSpacing.x << ", " << s.ItemSpacing.y << "],\n";
		out << "    \"item_inner_spacing\": [" << s.ItemInnerSpacing.x << ", " << s.ItemInnerSpacing.y << "]\n";
		out << "  },\n";
		out << "  \"colors\": [\n";
		for (int i = 0; i < ImGuiCol_COUNT; i++)
		{
			ImVec4& c = s.Colors[i];
			out << "    [" << c.x << ", " << c.y << ", " << c.z << ", " << c.w << "]";
			if (i != ImGuiCol_COUNT - 1)
				out << ",";
			out << "\n";
		}
		out << "  ],\n";
		out << "  \"ui_colors\": {\n";
		WriteUIColor(out, "SidebarBg", SidebarBg);
		WriteUIColor(out, "ItemHoveredBg", ItemHoveredBg);
		WriteUIColor(out, "ItemActiveBg", ItemActiveBg);
		WriteUIColor(out, "Accent", Accent);
		WriteUIColor(out, "IconActive", IconActive);
		WriteUIColor(out, "IconHovered", IconHovered);
		WriteUIColor(out, "IconIdle", IconIdle, false);
		out << "  }\n";

		out << "}\n";
	}
	void LoadTheme(const std::filesystem::path& file)
	{
		ImGuiStyle& s = ImGui::GetStyle();
		std::ifstream in(file);
		if (!in.is_open())
			return;
		std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
		std::vector<float> values;
		values.reserve(256);
		const char* p = data.c_str();
		char* end = nullptr;
		while (*p)
		{
			if ((*p >= '0' && *p <= '9') || *p == '-' || *p == '.')
			{
				float v = strtof(p, &end);
				if (p != end)
				{
					values.push_back(v);
					p = end;
					continue;
				}
			}
			++p;
		}
		const size_t needed = 1 + 6 + 8 + (ImGuiCol_COUNT * 4);
		if (values.size() < needed)
			return;
		size_t i = 0;
		s.Alpha = values[i++];
		s.WindowRounding = values[i++];
		s.FrameRounding = values[i++];
		s.PopupRounding = values[i++];
		s.ScrollbarRounding = values[i++];
		s.GrabRounding = values[i++];
		s.ScrollbarSize = values[i++];
		s.WindowPadding = {values[i++], values[i++]};
		s.FramePadding = {values[i++], values[i++]};
		s.ItemSpacing = {values[i++], values[i++]};
		s.ItemInnerSpacing = {values[i++], values[i++]};
		for (int c = 0; c < ImGuiCol_COUNT; c++)
		{
			s.Colors[c] = {
			    values[i++],
			    values[i++],
			    values[i++],
			    values[i++]};
		}
		if (i + (7 * 4) <= values.size())
		{
			ReadUIColor(SidebarBg, values, i);
			ReadUIColor(ItemHoveredBg, values, i);
			ReadUIColor(ItemActiveBg, values, i);
			ReadUIColor(Accent, values, i);
			ReadUIColor(IconActive, values, i);
			ReadUIColor(IconHovered, values, i);
			ReadUIColor(IconIdle, values, i);
		}
	}
	static std::vector<std::filesystem::path> GetJsonThemes()
	{
		std::vector<std::filesystem::path> out;
		for (auto& e : std::filesystem::directory_iterator(GetThemesPath()))
		{
			if (e.is_regular_file() && e.path().extension() == ".json")
				out.push_back(e.path());
		}
		return out;
	}
	void theme_editor()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;
		static int editing = -1;
		static bool open_color_popup = false;
		ImGui::BeginChild("ThemeEditor", ImVec2(0, 0), true);
		if (ImGui::CollapsingHeader("Style", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Alpha", &style.Alpha, 0.20f, 1.00f, "%.2f");
			ImGui::SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 16.0f);
			ImGui::SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 16.0f);
			ImGui::SliderFloat("Popup Rounding", &style.PopupRounding, 0.0f, 16.0f);
			ImGui::SliderFloat("Scrollbar Rounding", &style.ScrollbarRounding, 0.0f, 16.0f);
			ImGui::SliderFloat("Grab Rounding", &style.GrabRounding, 0.0f, 16.0f);
			ImGui::SliderFloat2("Window Padding", (float*)&style.WindowPadding, 0.0f, 20.0f);
			ImGui::SliderFloat2("Frame Padding", (float*)&style.FramePadding, 0.0f, 20.0f);
			ImGui::SliderFloat2("Item Spacing", (float*)&style.ItemSpacing, 0.0f, 20.0f);
			ImGui::SliderFloat2("Item Inner Spacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f);
			ImGui::SliderFloat("Scrollbar Size", &style.ScrollbarSize, 6.0f, 24.0f);
		}
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::TreeNode("Icons"))
			{
				ColorWheelPicker_UIM("Sidebar BG", SidebarBg);
				ColorWheelPicker_UIM("Accent", Accent);
				ColorWheelPicker_UIM("Item Active", ItemActiveBg);
				ColorWheelPicker_UIM("Item Hover", ItemHoveredBg);
				ColorWheelPicker_UIM("Icon Idle", IconIdle);
				ColorWheelPicker_UIM("Icon Hover", IconHovered);
				ColorWheelPicker_UIM("Icon Active", IconActive);
				ImGui::TreePop();
			}
			static ImGuiTextFilter filter;
			filter.Draw("Filter colors");
			ImGui::Spacing();
			ImGui::BeginChild("##color_list", ImVec2(0, 260.f), true);
			for (int i = 0; i < ImGuiCol_COUNT; i++)
			{
				const char* name = ImGui::GetStyleColorName(i);
				if (!filter.PassFilter(name))
					continue;
				ImGui::PushID(i);
				ImGui::Selectable(name, false);
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				{
					editing = i;
					open_color_popup = true;
				}
				ImGui::SameLine(220);
				ImGui::ColorButton("##preview", colors[i], ImGuiColorEditFlags_NoTooltip, ImVec2(28, 14));
				ImGui::PopID();
			}
			ImGui::EndChild();
		}
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static char theme_name[64] = "MyTheme";
			static int selected_theme = -1;
			if (ImGui::Button("Dark"))
				ImGui::StyleColorsDark();
			ImGui::SameLine();
			if (ImGui::Button("Classic"))
				ImGui::StyleColorsClassic();
			ImGui::SameLine();
			if (ImGui::Button("Light"))
				ImGui::StyleColorsLight();
			ImGui::Separator();
			ImGui::InputText("Theme Name", theme_name, IM_ARRAYSIZE(theme_name));
			if (ImGui::Button("Save Theme"))
			{
				auto path = GetThemesPath() / (std::string(theme_name) + ".json");
				SaveTheme(path);
			}
			ImGui::Separator();
			ImGui::TextUnformatted("Available Themes:");
			auto themes = GetJsonThemes();
			ImGui::BeginChild("##themes", ImVec2(0, 150.f), true);
			for (int i = 0; i < (int)themes.size(); i++)
			{
				std::string name = themes[i].stem().string();
				if (ImGui::Selectable(name.c_str(), selected_theme == i))
					selected_theme = i;
			}
			ImGui::EndChild();
			if (selected_theme >= 0 && selected_theme < (int)themes.size())
			{
				if (ImGui::Button("Load Selected"))
			    {
					 LoadTheme(themes[selected_theme]);
					 SaveLastTheme(themes[selected_theme]);
				}
			}
		}
		if (open_color_popup && editing != -1)
		{
			ImGui::OpenPopup("Color Wheel");
			open_color_popup = false;
			ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing, ImVec2(0.0f, 0.0f));
			ImGui::SetNextWindowSize(ImVec2(300.f, 380.f), ImGuiCond_Appearing);
		}
		if (ImGui::BeginPopup("Color Wheel",
		        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
		{
			if (editing >= 0 && editing < ImGuiCol_COUNT)
			{
				ImGui::TextUnformatted(ImGui::GetStyleColorName(editing));
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::PushID(editing);
				float radius = 70.f;
				float wheel_w = radius * 2.f;
				float avail = ImGui::GetContentRegionAvail().x;
				float pad = (avail > wheel_w) ? (avail - wheel_w) * 0.5f : 0.f;
				if (pad > 0.f)
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad);
				DrawColorWheel("##wheel", colors[editing], radius);
				ImGui::Spacing();
				ImGui::SliderFloat("R", &colors[editing].x, 0.f, 1.f);
				ImGui::SliderFloat("G", &colors[editing].y, 0.f, 1.f);
				ImGui::SliderFloat("B", &colors[editing].z, 0.f, 1.f);
				ImGui::SliderFloat("A", &colors[editing].w, 0.f, 1.f);
				ImGui::PopID();
				ImGui::Spacing();
				ImGui::Separator();
				if (ImGui::Button("Close"))
					ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::EndChild();
	}
	void AutoLoadTheme()
	{
		if (auto theme = LoadLastTheme())
			LoadTheme(*theme);
	}
}
