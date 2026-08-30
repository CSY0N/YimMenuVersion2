#include "SpawnVehicle.hpp"
#include "core/backend/FiberPool.hpp"
#include "core/backend/ScriptMgr.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/PersonalVehicles.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/data/Vehicles.hpp"
#include "game/gta/Natives.hpp"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

namespace YimMenu::Submenus
{
	static BoolCommand spawnInsideVehicle{"spawninsideveh", "Spawn Inside", "Spawn inside the vehicle."};
	static BoolCommand spawnVehicleMaxed{"spawnvehmaxed", "Spawn Maxed", "Spawn the vehicle maxed."};
	static BoolCommand spawnInsidePersonalVehicle{"spawninsidepv", "Spawn Inside", "Spawn inside the personal vehicle."};
	static BoolCommand spawnClonePersonalVehicle{"spawnclonepv", "Spawn Clone", "Spawn a clone of the personal vehicle."};

	static std::atomic<Hash> g_PreviewVehicleHash{};
	static std::atomic<int> g_PreviewVehicleHandle{0};

	static std::string ToLower(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return text;
	}

	static void DeleteVehiclePreview()
	{
		g_PreviewVehicleHash = 0;

		FiberPool::Push([] {
			int handle = g_PreviewVehicleHandle.exchange(0);

			if (!handle || !ENTITY::DOES_ENTITY_EXIST(handle))
				return;

			ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, true, true);
			ENTITY::DELETE_ENTITY(&handle);
		});
	}

	static void CreateVehiclePreview(Hash hash)
	{
		if (!hash || g_PreviewVehicleHash == hash)
			return;

		g_PreviewVehicleHash = hash;

		FiberPool::Push([hash] {
			if (g_PreviewVehicleHash != hash)
				return;

			int oldHandle = g_PreviewVehicleHandle.exchange(0);

			if (oldHandle && ENTITY::DOES_ENTITY_EXIST(oldHandle))
			{
				ENTITY::SET_ENTITY_AS_MISSION_ENTITY(oldHandle, true, true);
				ENTITY::DELETE_ENTITY(&oldHandle);
			}

			auto ped = Self::GetPed();

			if (!ped || g_PreviewVehicleHash != hash)
				return;

			Vector3 previewPosition = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(ped.GetHandle(), 3.5f, 6.0f, 0.0f);
			float previewHeading = ENTITY::GET_ENTITY_HEADING(ped.GetHandle()) + 180.0f;

			auto previewVehicle = Vehicle::Create(hash, previewPosition, previewHeading);

			if (!previewVehicle)
			{
				if (g_PreviewVehicleHash == hash)
					g_PreviewVehicleHash = 0;

				return;
			}

			int handle = previewVehicle.GetHandle();

			if (g_PreviewVehicleHash != hash)
			{
				ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, true, true);
				ENTITY::DELETE_ENTITY(&handle);
				return;
			}

			g_PreviewVehicleHandle = handle;

			ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, true, true);
			ENTITY::SET_ENTITY_INVINCIBLE(handle, true, true);
			ENTITY::SET_ENTITY_COLLISION(handle, false, false);
			ENTITY::FREEZE_ENTITY_POSITION(handle, true);
			VEHICLE::SET_VEHICLE_DOORS_LOCKED(handle, 2);
			VEHICLE::SET_VEHICLE_ENGINE_ON(handle, false, true, true);
			VEHICLE::SET_VEHICLE_DIRT_LEVEL(handle, 0.0f);

			while (g_PreviewVehicleHash == hash && g_PreviewVehicleHandle == handle)
			{
				if (!ENTITY::DOES_ENTITY_EXIST(handle))
					break;

				float heading = ENTITY::GET_ENTITY_HEADING(handle) + 0.35f;

				if (heading >= 360.0f)
					heading -= 360.0f;

				ENTITY::SET_ENTITY_HEADING(handle, heading);
				ScriptMgr::Yield();
			}
		});
	}

	std::shared_ptr<TabItem> RenderSpawnNewVehicle()
	{
		auto tab = std::make_shared<TabItem>("New Vehicle");
		auto spawn = std::make_shared<Group>("Spawn");
		auto settings = std::make_shared<Group>("Settings");

		static std::vector<std::string> vehicleNames{};
		static std::vector<int> vehicleClasses{};
		static int selectedClass{-1};

		spawn->AddItem(std::make_unique<ImGuiItem>([] {
			static bool init = [] {
				FiberPool::Push([] {
					std::unordered_map<std::string, int> nameCount;

					vehicleNames.clear();
					vehicleClasses.clear();

					vehicleNames.reserve(g_VehicleHashes.size());
					vehicleClasses.reserve(g_VehicleHashes.size());

					for (const auto& vehicleHash : g_VehicleHashes)
					{
						std::string gxt = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(vehicleHash);
						std::string displayName = HUD::GET_FILENAME_FOR_AUDIO_CONVERSATION(gxt.c_str());
						std::string finalName = displayName == "NULL" ? gxt : displayName;

						int& count = nameCount[finalName];

						if (count > 0)
							finalName += " " + std::to_string(count + 1);

						++count;

						std::string maker = HUD::GET_FILENAME_FOR_AUDIO_CONVERSATION(VEHICLE::GET_MAKE_NAME_FROM_VEHICLE_MODEL(vehicleHash));

						if (maker != "NULL" && !maker.empty())
							finalName = maker + " " + finalName;

						vehicleNames.push_back(finalName);
						vehicleClasses.push_back(VEHICLE::GET_VEHICLE_CLASS_FROM_NAME(vehicleHash));
					}
				});

				return true;
			}();

			static char search[64]{};

			ImGui::SetNextItemWidth(300.0f);
			ImGui::InputTextWithHint("Name", "Search", search, sizeof(search));

			ImGui::SetNextItemWidth(300.0f);

			const char* selectedClassName = "All";

			if (selectedClass >= 0 && selectedClass < static_cast<int>(g_VehicleClassNames.size()))
				selectedClassName = g_VehicleClassNames[selectedClass];

			if (ImGui::BeginCombo("Class", selectedClassName))
			{
				if (ImGui::Selectable("All", selectedClass == -1))
					selectedClass = -1;

				for (int i = 0; i < static_cast<int>(g_VehicleClassNames.size()); ++i)
				{
					if (ImGui::Selectable(g_VehicleClassNames[i], selectedClass == i))
						selectedClass = i;
				}

				ImGui::EndCombo();
			}

			const int visible = std::min(20, static_cast<int>(vehicleNames.size()));
			const float height = visible * ImGui::GetTextLineHeightWithSpacing();
			bool vehicleHoveredThisFrame = false;

			if (ImGui::BeginListBox("##vehicles", ImVec2(300.0f, height)))
			{
				if (vehicleNames.empty())
				{
					ImGui::TextUnformatted("Natives not cached yet.");
				}
				else
				{
					const std::string lowerSearch = ToLower(search);
					const std::size_t vehicleCount = std::min(vehicleNames.size(), g_VehicleHashes.size());

					for (std::size_t vehicleIndex = 0; vehicleIndex < vehicleCount; ++vehicleIndex)
					{
						const Hash hash = g_VehicleHashes[vehicleIndex];
						const std::string& name = vehicleNames[vehicleIndex];
						const std::string lowerName = ToLower(name);
						const bool matchesSearch = lowerName.find(lowerSearch) != std::string::npos;
						bool matchesClass = selectedClass == -1;

						if (vehicleIndex < vehicleClasses.size())
							matchesClass = selectedClass == -1 || vehicleClasses[vehicleIndex] == selectedClass;

						if (!matchesSearch || !matchesClass)
							continue;

						ImGui::PushID(static_cast<int>(hash));

						if (ImGui::Selectable(name.c_str()))
						{
							DeleteVehiclePreview();

							FiberPool::Push([hash] {
								auto handle = Vehicle::Create(hash, Vehicle::GetSpawnLocRelToPed(Self::GetPed().GetHandle(), hash), Self::GetPed().GetHeading());

								if (!handle)
									return;

								if (spawnInsideVehicle.GetState())
									Self::GetPed().SetInVehicle(handle);

								if (spawnVehicleMaxed.GetState())
									handle.Upgrade();
							});
						}

						if (ImGui::IsItemHovered())
						{
							vehicleHoveredThisFrame = true;
							CreateVehiclePreview(hash);
						}

						ImGui::PopID();
					}
				}

				ImGui::EndListBox();
			}

			if (!vehicleHoveredThisFrame && g_PreviewVehicleHash != 0)
				DeleteVehiclePreview();
		}));

		settings->AddItem(std::make_shared<BoolCommandItem>("spawninsideveh"_J));
		settings->AddItem(std::make_shared<BoolCommandItem>("spawnvehmaxed"_J));

		tab->AddItem(spawn);
		tab->AddItem(settings);

		return tab;
	}

	std::shared_ptr<TabItem> RenderSpawnPersonalVehicle()
	{
		auto tab = std::make_shared<TabItem>("Personal Vehicle");
		auto spawn = std::make_shared<Group>("Spawn");
		auto settings = std::make_shared<Group>("Settings");

		static std::string selectedGarageStr{};

		spawn->AddItem(std::make_unique<ImGuiItem>([] {
			if (g_PreviewVehicleHash != 0)
				DeleteVehiclePreview();

			if (!*Pointers.IsSessionStarted)
			{
				ImGui::TextDisabled("Join GTA Online.");
				return;
			}

			PersonalVehicles::Update();

			static char search[64]{};

			ImGui::SetNextItemWidth(300.0f);
			ImGui::InputTextWithHint("Name", "Search", search, sizeof(search));

			ImGui::SetNextItemWidth(300.0f);

			if (ImGui::BeginCombo("Garage", selectedGarageStr.empty() ? "All" : selectedGarageStr.c_str()))
			{
				if (ImGui::Selectable("All", selectedGarageStr.empty()))
					selectedGarageStr.clear();

				for (const auto& garage : PersonalVehicles::GetGarages())
				{
					if (ImGui::Selectable(garage.c_str(), garage == selectedGarageStr))
						selectedGarageStr = garage;
				}

				ImGui::EndCombo();
			}

			const auto& personalVehicles = PersonalVehicles::GetPersonalVehicles();
			const int visible = std::min(20, static_cast<int>(personalVehicles.size()));
			const float height = visible * ImGui::GetTextLineHeightWithSpacing();

			if (ImGui::BeginListBox("##personalvehicles", ImVec2(300.0f, height)))
			{
				if (personalVehicles.empty())
				{
					ImGui::TextUnformatted("Stats not loaded yet.");
				}
				else
				{
					const std::string lowerSearch = ToLower(search);

					for (const auto& item : personalVehicles)
					{
						const auto& label = item.first;
						const auto& personalVehicle = item.second;
						const std::string lowerName = ToLower(label);
						const bool matchesSearch = lowerName.find(lowerSearch) != std::string::npos;
						const bool matchesGarage = selectedGarageStr.empty() || personalVehicle->GetGarage() == selectedGarageStr;

						if (!matchesSearch || !matchesGarage)
							continue;

						ImGui::PushID(personalVehicle->GetId());

						if (ImGui::Selectable(label.c_str()))
						{
							FiberPool::Push([&personalVehicle] {
								if (spawnClonePersonalVehicle.GetState())
								{
									auto coordinates = Vehicle::GetSpawnLocRelToPed(Self::GetPed().GetHandle(), personalVehicle->GetModel());
									const float heading = Self::GetPed().GetHeading();
									auto handle = personalVehicle->Clone(coordinates, heading);

									if (spawnInsidePersonalVehicle.GetState())
										Self::GetPed().SetInVehicle(handle);
								}
								else if (!personalVehicle->Request(spawnInsidePersonalVehicle.GetState()))
								{
									Notifications::Show("Spawn Personal Vehicle", "Failed to spawn Personal Vehicle.", NotificationType::Error);
								}
							});
						}

						ImGui::PopID();
					}
				}

				ImGui::EndListBox();
			}
		}));

		settings->AddItem(std::make_shared<BoolCommandItem>("spawninsidepv"_J));
		settings->AddItem(std::make_shared<BoolCommandItem>("spawnclonepv"_J));

		tab->AddItem(spawn);
		tab->AddItem(settings);

		return tab;
	}

	std::shared_ptr<Category> BuildSpawnVehicleMenu()
	{
		auto menu = std::make_shared<Category>("Spawn");
		auto tabBar = std::make_shared<TabBarItem>("Spawn");

		tabBar->AddItem(RenderSpawnNewVehicle());
		tabBar->AddItem(RenderSpawnPersonalVehicle());

		menu->AddItem(std::move(tabBar));

		return menu;
	}
}
