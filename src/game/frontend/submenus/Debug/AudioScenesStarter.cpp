#include "AudioScenesStarter.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/gta/data/AudioSceneNames.hpp"
#include "game/gta/Natives.hpp"

namespace YimMenu::Submenus
{
	std::shared_ptr<Category> BuildAudioScenesMenu()
	{
		auto menu = std::make_shared<Category>("Audio Scenes");
		auto audioGroup = std::make_shared<Group>("All Audio Scenes");

		audioGroup->AddItem(std::make_unique<ImGuiItem>([]
		{
			static char audioSearch[64] = "";
			static std::string audioLower;
			static std::string activeScene;

			ImGui::InputTextWithHint("##AudioSceneSearch", "Search audio scenes", audioSearch, sizeof(audioSearch));

			audioLower = audioSearch;
			std::transform(audioLower.begin(), audioLower.end(), audioLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			ImGui::Separator();
			ImGui::BeginChild("AudioSceneList", ImVec2(0, 400), true);

			for (const auto& scene : audioSceneNames)
			{
				if (!audioLower.empty())
				{
					std::string lower = scene;
					std::transform(audioLower.begin(), audioLower.end(), audioLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

					if (lower.find(audioLower) == std::string::npos)
						continue;
				}

				const bool selected = activeScene == scene;

				if (ImGui::Selectable(scene, selected))
				{
					activeScene = scene;

					FiberPool::Push([scene]
					{
						AUDIO::START_AUDIO_SCENE(scene);
					});
				}
			}

			ImGui::EndChild();

			if (ImGui::Button("Stop Selected"))
			{
				if (!activeScene.empty())
				{
					const std::string scene = activeScene;

					FiberPool::Push([scene]
					{
						AUDIO::STOP_AUDIO_SCENE(scene.c_str());
					});

					activeScene.clear();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop All Audio Scenes"))
			{
				FiberPool::Push([]
				{
					AUDIO::STOP_AUDIO_SCENES();
				});

				activeScene.clear();
			}
		}));

		menu->AddItem(std::move(audioGroup));
		return menu;
	}
}
