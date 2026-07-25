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

	static int g_PreviewVehicleHandle = 0;
	static Hash g_PreviewVehicleHash = 0;
	static Hash g_HoveredVehicleHash = 0;
	static float g_VehicleHoverStart = 0.0f;
	static void DeleteVehiclePreview()
	{
		if (!g_PreviewVehicleHandle)
		{
			g_PreviewVehicleHash = 0;
			return;
		}

		if (!ENTITY::DOES_ENTITY_EXIST(g_PreviewVehicleHandle))
		{
			g_PreviewVehicleHandle = 0;
			g_PreviewVehicleHash = 0;
			return;
		}

		ENTITY::SET_ENTITY_AS_MISSION_ENTITY(g_PreviewVehicleHandle, true, true);

		int handle = g_PreviewVehicleHandle;
		VEHICLE::DELETE_VEHICLE(&handle);

		g_PreviewVehicleHandle = 0;
		g_PreviewVehicleHash = 0;
	}

	static void CreateVehiclePreview(Hash hash)
	{
		if (!STREAMING::IS_MODEL_VALID(hash))
			return;

		if (!STREAMING::IS_MODEL_A_VEHICLE(hash))
			return;

		if (g_PreviewVehicleHash == hash && g_PreviewVehicleHandle && ENTITY::DOES_ENTITY_EXIST(g_PreviewVehicleHandle))
		{
			return;
		}

		DeleteVehiclePreview();

		Ped playerPed = PLAYER::PLAYER_PED_ID();
		ENTITY::DOES_ENTITY_EXIST(playerPed.GetHandle());
		Vector3 previewPosition = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(Self::GetPed().GetHandle(), 3.5f, 6.0f, 0.0f);

		float previewHeading = ENTITY::GET_ENTITY_HEADING(Self::GetPed().GetHandle()) + 180.0f;
		auto previewVehicle = Vehicle::Create(hash, previewPosition, previewHeading);

		g_PreviewVehicleHandle = previewVehicle.GetHandle();

		if (!g_PreviewVehicleHandle || !ENTITY::DOES_ENTITY_EXIST(g_PreviewVehicleHandle))
		{
			g_PreviewVehicleHandle = 0;
			g_PreviewVehicleHash = 0;
			return;
		}

		g_PreviewVehicleHash = hash;
		ENTITY::SET_ENTITY_AS_MISSION_ENTITY(g_PreviewVehicleHandle, true, true);
	    ENTITY::SET_ENTITY_INVINCIBLE(g_PreviewVehicleHandle, true, true);
		ENTITY::SET_ENTITY_COLLISION(g_PreviewVehicleHandle, false, false);
		ENTITY::FREEZE_ENTITY_POSITION(g_PreviewVehicleHandle, true);
		VEHICLE::SET_VEHICLE_DOORS_LOCKED(g_PreviewVehicleHandle, 2);
		VEHICLE::SET_VEHICLE_ENGINE_ON(g_PreviewVehicleHandle, false, true, true);
		VEHICLE::SET_VEHICLE_DIRT_LEVEL(g_PreviewVehicleHandle, 0.0f);
	}

	static void UpdateVehiclePreview()
	{
		if (!g_PreviewVehicleHandle)
			return;

		if (!ENTITY::DOES_ENTITY_EXIST(g_PreviewVehicleHandle))
		{
			g_PreviewVehicleHandle = 0;
			g_PreviewVehicleHash = 0;
			return;
		}
		float heading = ENTITY::GET_ENTITY_HEADING(g_PreviewVehicleHandle);
		heading += ImGui::GetIO().DeltaTime * 25.0f;

		if (heading >= 360.0f)
			heading -= 360.0f;

		ENTITY::SET_ENTITY_HEADING(g_PreviewVehicleHandle, heading);
	}

	static std::string ToLower(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
		return text;
	}

	std::shared_ptr<TabItem> RenderSpawnNewVehicle()
	{
		auto tab = std::make_shared<TabItem>("New Vehicle");

		auto spawn = std::make_shared<Group>("Spawn");
		auto settings = std::make_shared<Group>("Settings");

		static std::vector<std::string> vehicleNames{};
		static std::vector<int> vehicleClasses{};
		static int selectedClass{-1};

		spawn->AddItem(
		    std::make_unique<ImGuiItem>(
		        [] {
			        static bool init = [] {
				        FiberPool::Push(
				            [] {
					            std::unordered_map<std::string, int> nameCount;

					            vehicleNames.clear();
					            vehicleClasses.clear();

					            vehicleNames.reserve(g_VehicleHashes.size());
					            vehicleClasses.reserve(g_VehicleHashes.size());

					            for (const auto& vehicleHash : g_VehicleHashes)
					            {
						            std::string gxt = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(vehicleHash);
									std::string displayName = HUD::GET_FILENAME_FOR_AUDIO_CONVERSATION(gxt.c_str());
						            std::string finalName =
						                displayName == "NULL"
						                    ? gxt
						                    : displayName;

						            int& count = nameCount[finalName];

						            if (count > 0)
						            {
							            finalName +=
							                " " +
							                std::to_string(count + 1);
						            }

						            ++count;
						            std::string maker = HUD::GET_FILENAME_FOR_AUDIO_CONVERSATION(VEHICLE::GET_MAKE_NAME_FROM_VEHICLE_MODEL(vehicleHash));			
						            if (maker != "NULL" &&
						                !maker.empty())
						            {
							            finalName = maker + " " + finalName;
						            }

						            vehicleNames.push_back(finalName);
									int vehicleClass = VEHICLE::GET_VEHICLE_CLASS_FROM_NAME(vehicleHash);
						            vehicleClasses.push_back(vehicleClass);
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
			        {
				        selectedClassName = g_VehicleClassNames[selectedClass];
			        }

			        if (ImGui::BeginCombo("Class", selectedClassName))
			        {
				        if (ImGui::Selectable("All", selectedClass == -1))
				        {
					        selectedClass = -1;
				        }

				        for (int i = 0; i < static_cast<int>(g_VehicleClassNames.size()); ++i)   
				        {
					        if (ImGui::Selectable(g_VehicleClassNames[i], selectedClass == i))
					        {
						        selectedClass = i;
					        }
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
						        {
							        matchesClass = selectedClass == -1 || vehicleClasses[vehicleIndex] == selectedClass;
						        }

						        if (!matchesSearch || !matchesClass)
						        {
							        continue;
						        }

						        ImGui::PushID(static_cast<int>(hash));

						        if (ImGui::Selectable(name.c_str()))
						        {
							        DeleteVehiclePreview();

							        g_HoveredVehicleHash = 0;

							        FiberPool::Push(
							            [hash] {
								            auto handle = Vehicle::Create(hash, Vehicle::GetSpawnLocRelToPed(Self::GetPed().GetHandle(), hash), Self::GetPed().GetHeading());
								            if (spawnInsideVehicle.GetState())
								            {
									            Self::GetPed().SetInVehicle(handle);
								            }

								            if (spawnVehicleMaxed.GetState())
								            {
									            handle.Upgrade();
								            }
							            });
						        }

						        if (ImGui::IsItemHovered())
						        {
							        vehicleHoveredThisFrame = true;

							        if (g_HoveredVehicleHash != hash)
							        {
								        g_HoveredVehicleHash = hash;
										g_VehicleHoverStart = static_cast<float>(ImGui::GetTime());
							        }
									const float hoverTime = static_cast<float>(ImGui::GetTime()) - g_VehicleHoverStart;

							        if (hoverTime >= 0.35f)
							        {
								        CreateVehiclePreview(hash);
							        }
						        }

						        ImGui::PopID();
					        }
				        }

				        ImGui::EndListBox();
			        }

			        if (!vehicleHoveredThisFrame)
			        {
				        g_HoveredVehicleHash = 0;
				        g_VehicleHoverStart = 0.0f;

				        DeleteVehiclePreview();
			        }
			        else
			        {
				        UpdateVehiclePreview();
			        }
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

		spawn->AddItem(
		    std::make_unique<ImGuiItem>(
		        [] {
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
				        {
					        selectedGarageStr.clear();
				        }

				        for (const auto& garage : PersonalVehicles::GetGarages())
				        {
					        if (ImGui::Selectable(
					                garage.c_str(),
					                garage == selectedGarageStr))
					        {
						        selectedGarageStr = garage;
					        }
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

					        for (const auto& item :
					             personalVehicles)
					        {
						        const auto& label = item.first;
						        const auto& personalVehicle = item.second;
						        const std::string lowerName = ToLower(label);
								const bool matchesSearch = lowerName.find(lowerSearch) != std::string::npos;
						        const bool matchesGarage = selectedGarageStr.empty() || personalVehicle->GetGarage() == selectedGarageStr;
						        if (!matchesSearch ||
						            !matchesGarage)
						        {
							        continue;
						        }

						        ImGui::PushID(personalVehicle->GetId());

						        if (ImGui::Selectable(label.c_str()))
						        {
							        FiberPool::Push(
							            [&personalVehicle] {
								            if (spawnClonePersonalVehicle.GetState())
								            {
									            auto coordinates = Vehicle::GetSpawnLocRelToPed(Self::GetPed().GetHandle(), personalVehicle->GetModel());
									            const float heading = Self::GetPed().GetHeading();
												auto handle = personalVehicle->Clone(coordinates, heading);
									     
									            if (spawnInsidePersonalVehicle.GetState())
									            {
										            Self::GetPed().SetInVehicle(
										                handle);
									            }
								            }
								            else
								            {
									            if (!personalVehicle->Request(spawnInsidePersonalVehicle.GetState()))
									            {
													Notifications::Show("Spawn Personal Vehicle", "Failed to spawn Personal Vehicle.", NotificationType::Error);
									            }
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
