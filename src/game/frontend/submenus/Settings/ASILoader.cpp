#include "ASILoader.hpp"
#include "game/features/Settings/asi_loader.hpp"
#include "game/frontend/items/Items.hpp"
#include <imgui.h>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace YimMenu::Submenus
{
	const char* GetStatusText(const asi_loader::status status)
	{
		switch (status)
		{
		case asi_loader::status::loaded:
			return "Loaded";

		case asi_loader::status::failed:
			return "Failed";

		case asi_loader::status::unloaded:
		default:
			return "Unloaded";
		}
	}

    std::shared_ptr<Category> BuildASILoaderMenu()
    {
        auto menu = std::make_shared<Category>("ASI Loader");
        auto loaderGroup = std::make_shared<Group>("ASI Plugins");

        loaderGroup->AddItem(std::make_unique<ImGuiItem>([]
        {
            static char searchBuffer[128]{};
            static std::string statusMessage;
            static bool initialized = false;

            if (!initialized)
            {
                if (asi_loader::initialize())
                    statusMessage = "ASI directory initialized.";
                else
                    statusMessage = "Failed to initialize ASI directory.";

                initialized = true;
            }

            ImGui::TextUnformatted("Load and manage ASI plugins.");

            ImGui::Spacing();

            if (ImGui::Button("Select ASI Files"))
			{
				if (asi_loader::select_asi_files())
				{
					statusMessage = "Selected ASI files were copied and loaded.";
				}
				else
				{
					statusMessage = "No ASI was loaded. Check its error information.";
				}
			}
            

            ImGui::SameLine();

            if (ImGui::Button("Scan Folder"))
            {
                if (asi_loader::scan_default_directory())
                    statusMessage = "ASI folder scanned.";
                else
                    statusMessage = "Failed to scan ASI folder.";
            }

            ImGui::SameLine();

            if (ImGui::Button("Load All"))
            {
				const std::size_t count = asi_loader::load_all();
                statusMessage = "Loaded " + std::to_string(count) + " ASI plugin(s).";
            }

            ImGui::SameLine();

            if (ImGui::Button("Unload All"))
            {
				const std::size_t count = asi_loader::unload_all();
                statusMessage = "Processed " + std::to_string(count) + " ASI plugin(s).";
            }

            ImGui::Spacing();

            ImGui::InputTextWithHint(
                "##ASISearch",
                "Search ASI plugins",
                searchBuffer,
                sizeof(searchBuffer));

            std::string lowerSearch = searchBuffer;

            std::transform(
			    lowerSearch.begin(),
			    lowerSearch.end(),
			    lowerSearch.begin(),
			    [](const unsigned char character) {
				    return static_cast<char>(
				        std::tolower(character));
			    });

            ImGui::Separator();

            const std::vector<asi_loader::plugin> plugins = asi_loader::get_plugins();
            ImGui::Text("Plugins: %zu", plugins.size());
        
            ImGui::BeginChild("##ASIPluginList", ImVec2(0.0f, 420.0f), true);
            if (plugins.empty())
            {
				ImGui::TextDisabled("No ASI plugins found.");
            }

            for (std::size_t index = 0;
                 index < plugins.size();
                 ++index)
			{
				const asi_loader::plugin& plugin = plugins[index];

                std::string lowerName = plugin.name;

                std::transform(
				    lowerName.begin(),
				    lowerName.end(),
				    lowerName.begin(),
				    [](const unsigned char character) {
					    return static_cast<char>(
					        std::tolower(character));
				    });

                if (!lowerSearch.empty() && lowerName.find(lowerSearch) == std::string::npos)
				{
					continue;
				}

                ImGui::PushID(static_cast<int>(index));

                ImGui::BeginGroup();

                ImGui::TextUnformatted(plugin.name.c_str());

               ImGui::TextDisabled("%s", plugin.path.string().c_str());
               ImGui::Text("Status: %s", GetStatusText(plugin.loadStatus));

                if (plugin.loadStatus ==
                    asi_loader::status::loaded)
                {
                    if (ImGui::Button("Unload"))
                    {
						if (asi_loader::unload_asi(index))
							statusMessage = plugin.name + " unloaded.";
						else
							statusMessage = "Failed to unload " + plugin.name + ".";
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Reload"))
                    {
						if (asi_loader::reload_asi(index))
							statusMessage = plugin.name + " reloaded.";
						else
							statusMessage = "Failed to reload " + plugin.name + ".";
                    }
                }
                else
                {
                    if (ImGui::Button("Load"))
                    {
						if (asi_loader::load_asi(index))
							statusMessage = plugin.name + " loaded.";
						else
							statusMessage = "Failed to load " + plugin.name + ".";
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Remove"))
                    {
						if (asi_loader::remove_asi(index))
							statusMessage = plugin.name + " removed from the list.";
						else
							statusMessage = "Failed to remove " + plugin.name + ".";
                    }
                }

                if (!plugin.errorMessage.empty())
                {
                    ImGui::Spacing();
					ImGui::TextWrapped("Error %lu: %s", plugin.lastError, plugin.errorMessage.c_str());
                }

                ImGui::EndGroup();
                ImGui::Separator();
                ImGui::PopID();
            }

            ImGui::EndChild();

            ImGui::Spacing();

            if (!statusMessage.empty())
			{
				ImGui::TextWrapped("%s", statusMessage.c_str());
			}
        }));

        menu->AddItem(std::move(loaderGroup));

        return menu;
    }
}
