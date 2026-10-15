// Game-free checks exercise the compiled callbacks, UI and persistent settings.
#include "common.hpp"
#include "TinkerScriptData.hpp"
#include "core/filemgr/FileMgr.hpp"
#include <imgui.h>
using namespace YimMenu::TinkerScript;
namespace YimMenu::TinkerScript
{
	void BindRenderApi(Context& context);
	void BindCloudSaves(Context& context);
	void RequestCloudSave(std::string)
	{
	}
	void StartCloudSaveTool()
	{
	}
}
static Value Vector()
{
	return MakeTable({{"x", 0}, {"y", 0}, {"z", 0}});
}
// A small memory model lets toggle callbacks be checked without touching GTA globals.
static std::map<std::size_t, Value> memory;
static Value Address(std::size_t index = 0)
{
	auto result = MakeTable();
	for (auto name : {"at", "add"})
		Member(result, name) = MakeFunction([index](const Values& args) -> Values {
			auto offset = static_cast<std::size_t>(Number(args[1]));
			return {Address(index + (args.size() > 2 ? 1 + offset * static_cast<std::size_t>(Number(args[2])) : offset))};
		});
	Member(result, "deref") = MakeFunction([index](const Values&) -> Values {
		return {Address(index)};
	});
	Member(result, "get_int") = MakeFunction([index](const Values&) -> Values {
		return {memory[index].IsNil() ? Value(0) : memory[index]};
	});
	Member(result, "get_float") = MakeFunction([](const Values&) -> Values {
		return {0.0};
	});
	Member(result, "get_string") = MakeFunction([](const Values&) -> Values {
		return {""};
	});
	Member(result, "get_vector3") = MakeFunction([](const Values&) -> Values {
		return {Vector()};
	});
	for (auto name : {"set_int", "set_float", "set_string", "set_vector3"})
		Member(result, name) = MakeFunction([index](const Values& args) -> Values {
			memory[index] = args[1];
			return {};
		});
	return result;
}
static void Check(bool value, const char* message)
{
	if (!value)
		throw std::runtime_error(message);
}
int main(int argc, char** argv)
{
	try
	{
		Check(argc == 2, "Usage: TinkerScriptssTests <new test directory>");
		auto root = std::filesystem::absolute(argv[1]);
		Check(!std::filesystem::exists(root), "Use a new directory so existing settings cannot be overwritten");
		YimMenu::FileMgr::Init(root);
		// The real render bindings need an ImGui context during feature initialization.
		ImGui::CreateContext();
		auto& io = ImGui::GetIO();
		io.DisplaySize = {1920, 1080};
		io.DeltaTime = 1.0f / 60;
		io.IniFilename = nullptr;
		io.Fonts->AddFontDefault();
		io.Fonts->Build();
		auto previous = root / "scripts/TinkerScripts/include/Settings";
		std::filesystem::create_directories(previous);
		std::ofstream(previous / "TS_Settings.lua") << "TS_Settings.HGWHrestock = false\nTS_Settings.HGgoodtype = 6\n";
		Context context;
		InitializeStandardLibrary(context);

		context.Bind("ScriptNatives", "START_NEW_SCRIPT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("ScriptNatives", "VDIST", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("CAMERA", "DO_SCREEN_FADE_IN", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("CAMERA", "GET_FOLLOW_PED_CAM_VIEW_MODE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("CAMERA", "INVALIDATE_IDLE_CAM", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("CAMERA", "IS_GAMEPLAY_CAM_RENDERING", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("CAMERA", "IS_SCREEN_FADED_OUT", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("ENTITY", "GET_ENTITY_COORDS", [](const Values&) -> Values {
			return {Vector()};
		});
		context.Bind("ENTITY", "GET_ENTITY_FORWARD_VECTOR", [](const Values&) -> Values {
			return {Vector()};
		});
		context.Bind("ENTITY", "GET_ENTITY_HEADING", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("ENTITY", "GET_ENTITY_HEIGHT_ABOVE_GROUND", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("ENTITY", "GET_ENTITY_MODEL", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("ENTITY", "IS_ENTITY_A_MISSION_ENTITY", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("ENTITY", "SET_ENTITY_AS_MISSION_ENTITY", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("ENTITY", "SET_ENTITY_HEADING", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("FileMgr", "GetMenuRootPath", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("FileMgr", "ReadFileContent", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("FileMgr", "WriteFileContent", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "BEGIN_SCALEFORM_MOVIE_METHOD", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "DRAW_RECT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "DRAW_SCALEFORM_MOVIE_FULLSCREEN", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "DRAW_SPRITE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "END_SCALEFORM_MOVIE_METHOD", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "HAS_SCALEFORM_MOVIE_LOADED", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("GRAPHICS", "REQUEST_SCALEFORM_MOVIE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "REQUEST_STREAMED_TEXTURE_DICT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "SET_ARTIFICIAL_LIGHTS_STATE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "SET_ARTIFICIAL_VEHICLE_LIGHTS_STATE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "START_NETWORKED_PARTICLE_FX_NON_LOOPED_ON_PED_BONE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "START_PARTICLE_FX_NON_LOOPED_ON_PED_BONE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GRAPHICS", "USE_PARTICLE_FX_ASSET", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GTA", "CELL_HORIZONTAL_MODE_TOGGLE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("GTA", "CELL_SET_INPUT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "ADD_BLIP_FOR_ENTITY", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "BEGIN_TEXT_COMMAND_DISPLAY_HELP", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "BEGIN_TEXT_COMMAND_DISPLAY_TEXT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "BEGIN_TEXT_COMMAND_THEFEED_POST", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "DOES_BLIP_EXIST", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("HUD", "END_TEXT_COMMAND_DISPLAY_HELP", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "END_TEXT_COMMAND_DISPLAY_TEXT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "END_TEXT_COMMAND_THEFEED_POST_TICKER", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "GET_BLIP_INFO_ID_COORD", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "GET_BLIP_INFO_ID_ENTITY_INDEX", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "GET_FIRST_BLIP_INFO_ID", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "IS_HELP_MESSAGE_BEING_DISPLAYED", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("HUD", "IS_WARNING_MESSAGE_READY_FOR_CONTROL", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("HUD", "SET_BLIP_COLOUR", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "SET_BLIP_PRIORITY", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "SET_BLIP_SCALE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "SET_BLIP_SPRITE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "SET_TEXT_LEADING", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "SET_TEXT_SCALE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("HUD", "THEFEED_SET_BACKGROUND_COLOR_FOR_NEXT_POST", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("MISC", "DISPLAY_ONSCREEN_KEYBOARD", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("MISC", "GET_GAME_TIMER", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("MISC", "GET_ONSCREEN_KEYBOARD_RESULT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("MISC", "UPDATE_ONSCREEN_KEYBOARD", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("NETSHOPPING", "NET_GAMESERVER_GET_PRICE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("NETWORK", "NETWORK_IS_ACTIVITY_SESSION", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("NETWORK", "NETWORK_IS_IN_TRANSITION", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("NETWORK", "NETWORK_IS_SESSION_STARTED", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("OBJECT", "DOOR_SYSTEM_SET_DOOR_STATE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PAD", "IS_CONTROL_JUST_PRESSED", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("PAD", "IS_CONTROL_JUST_RELEASED", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("PAD", "IS_CONTROL_PRESSED", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("PAD", "IS_DISABLED_CONTROL_PRESSED", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("PAD", "SET_CONTROL_VALUE_NEXT_FRAME", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PED", "GET_PED_CONFIG_FLAG", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PED", "GET_VEHICLE_PED_IS_IN", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PED", "IS_PED_IN_ANY_VEHICLE", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("PED", "IS_PED_SITTING_IN_ANY_VEHICLE", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("PED", "SET_PED_CONFIG_FLAG", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PED", "SET_PED_COORDS_KEEP_VEHICLE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PED", "SET_PED_INTO_VEHICLE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PED", "SET_PED_TO_RAGDOLL", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PLAYER", "GIVE_ACHIEVEMENT_TO_PLAYER", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PLAYER", "HAS_ACHIEVEMENT_BEEN_PASSED", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("PLAYER", "PLAYER_ID", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("PLAYER", "PLAYER_PED_ID", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("SCRIPT", "HAS_SCRIPT_LOADED", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("SCRIPT", "REQUEST_SCRIPT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("SCRIPT", "SET_SCRIPT_AS_NO_LONGER_NEEDED", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("SCRIPT", "TERMINATE_THIS_THREAD", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("STATS", "STAT_SAVE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("STREAMING", "HAS_ANIM_DICT_LOADED", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("STREAMING", "IS_IPL_ACTIVE", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("STREAMING", "IS_PLAYER_SWITCH_IN_PROGRESS", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("STREAMING", "REMOVE_NAMED_PTFX_ASSET", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("STREAMING", "REMOVE_PTFX_ASSET", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("STREAMING", "REQUEST_ANIM_DICT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("STREAMING", "REQUEST_NAMED_PTFX_ASSET", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("TASK", "CLEAR_PED_TASKS", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("TASK", "CLEAR_PED_TASKS_IMMEDIATELY", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("TASK", "SET_DRIVE_TASK_DRIVING_STYLE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("TASK", "STOP_ANIM_TASK", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("TASK", "TASK_PLAY_ANIM", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("TASK", "TASK_VEHICLE_DRIVE_TO_COORD", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("TASK", "TASK_VEHICLE_DRIVE_WANDER", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "BRING_VEHICLE_TO_HALT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "GET_PED_IN_VEHICLE_SEAT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "GET_RANDOM_VEHICLE_IN_SPHERE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "IS_VEHICLE_STOPPED_AT_TRAFFIC_LIGHTS", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_CUSTOM_PRIMARY_COLOUR", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_CUSTOM_SECONDARY_COLOUR", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_ENGINE_ON", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_EXTRA_COLOURS", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_MAX_SPEED", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_MOD", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_MOD_KIT", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_NEON_COLOUR", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_NEON_ENABLED", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_NUMBER_PLATE_TEXT_INDEX", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_TYRE_SMOKE_COLOR", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_UNDRIVEABLE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_WHEEL_TYPE", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "SET_VEHICLE_XENON_LIGHT_COLOR_INDEX", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("VEHICLE", "TOGGLE_VEHICLE_MOD", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("WEAPON", "GET_SELECTED_PED_WEAPON", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("WEAPON", "GIVE_WEAPON_TO_PED", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("WEAPON", "HAS_PED_GOT_WEAPON", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("WEAPON", "SET_CURRENT_PED_WEAPON", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("commandmgr", "add_bool_command", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("commandmgr", "add_list_command", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("commandmgr", "add_looped_command", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("commandmgr", "get_command", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("entities", "get_all_objects_as_handles", [](const Values&) -> Values {
			return {MakeTable()};
		});
		context.Bind("entities", "get_all_peds_as_handles", [](const Values&) -> Values {
			return {MakeTable()};
		});
		context.Bind("event", "register_handler", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("log", "error", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("log", "info", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("log", "warn", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("memory", "scan_pattern", [](const Values&) -> Values {
			return {Address()};
		});
		context.Bind("menu", "add_always_draw_imgui", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("menu", "get_submenu", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("menu", "set_menu_icon", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("natives", "load_natives", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("network", "is_session_started", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("network", "join_session", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("notify", "error", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("notify", "info", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("notify", "success", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("notify", "warn", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("script", "run_in_callback", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("script", "yield", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("scripts", "is_active", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("scripts", "run_as_script", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("scripts", "start_new_script", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "get_bool", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("stats", "get_int", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "get_masked_bool", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("stats", "get_masked_int", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "get_packed_bool", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("stats", "get_packed_int", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "get_string", [](const Values&) -> Values {
			return {Value("")};
		});
		context.Bind("stats", "set_bool", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "set_float", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "set_int", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "set_masked_bool", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "set_masked_int", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "set_packed_bool", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "set_packed_bool_range", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "set_packed_int", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("stats", "set_string", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("transactions", "can_use_transactions", [](const Values&) -> Values {
			return {Value(false)};
		});
		context.Bind("transactions", "run_service", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("tunables", "get_int", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("tunables", "set_bool", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("tunables", "set_float", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("tunables", "set_int", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("util", "get_game_version", [](const Values&) -> Values {
			return {Value(0)};
		});
		context.Bind("util", "time", [](const Values&) -> Values {
			return {Value(0)};
		});

		context.Global("session_types") = MakeTable({{"invite_only", 0}});
		context.Global("menu_event") = MakeTable({{"Unload", 0}});
		context.Global("package") = MakeTable({{"loaded", MakeTable()}});
		context.Global("Vector3") = MakeFunction([](const Values& args) -> Values {
			return {MakeTable({{"x", At(args, 0).IsNil() ? Value(0) : args[0]}, {"y", At(args, 1).IsNil() ? Value(0) : args[1]}, {"z", At(args, 2).IsNil() ? Value(0) : args[2]}})};
		});
		for (auto name : {"ScriptGlobal", "Global", "ScriptLocal", "Local", "Entity", "Ped", "Vehicle"})
			context.Global(name) = MakeFunction([](const Values& args) -> Values {
				return {Address(static_cast<std::size_t>(Number(args.back())))};
			});
		context.Bind("util", "get_game_version", [](const Values&) -> Values {
			return {"1.70", "1158.16"};
		});
		context.Bind("util", "time", [](const Values&) -> Values {
			return {0};
		});
		auto handles = MakeTable();
		int commandCount = 0, categoryCount = 0;
		std::map<std::string, Values> specifications;
		std::vector<std::pair<std::string, Value>> draw;
		std::vector<Value> alwaysDraw;
		auto makeHandle = [](Value value) {
			auto saved = MakeCell(value);
			auto handle = MakeTable();
			Member(handle, "get_value") = MakeFunction([saved](const Values&) -> Values {
				return {*saved};
			});
			Member(handle, "set_value") = MakeFunction([saved](const Values& args) -> Values {
				*saved = args[1];
				return {};
			});
			Member(handle, "draw") = MakeFunction([](const Values&) -> Values {
				return {};
			});
			Member(handle, "enable") = MakeFunction([saved](const Values&) -> Values {
				*saved = true;
				return {};
			});
			Member(handle, "disable") = MakeFunction([saved](const Values&) -> Values {
				*saved = false;
				return {};
			});
			return handle;
		};
		for (auto kind : {"add_bool_command", "add_looped_command", "add_list_command"})
			context.Bind("commandmgr", kind, [&, kind](const Values& args) -> Values {
				auto name = Text(args[0]);
				specifications[name] = args;
				Check(Member(handles, name).IsNil(), "Duplicate command registered");
				auto initial = std::string_view(kind) == "add_bool_command" ? At(args, 3) : std::string_view(kind) == "add_list_command" ? At(args, 4) :
				                                                                                                                           Value(false);
				auto handle = makeHandle(initial);
				Member(handles, name) = handle;
				++commandCount;
				return {handle};
			});
		context.Bind("commandmgr", "get_command", [&](const Values& args) -> Values {
			auto& value = Member(handles, Text(args[0]));
			if (value.IsNil())
				value = makeHandle(false);
			return {value};
		});
		context.Bind("menu", "get_submenu", [&](const Values&) -> Values {
			auto submenu = MakeTable();
			Member(submenu, "add_category") = MakeFunction([&](const Values& args) -> Values {
				auto category = MakeTable();
				++categoryCount;
				auto name = Text(args[1]);
				Member(category, "imgui") = MakeFunction([&, name](const Values& args) -> Values {
					draw.emplace_back(name, args[1]);
					return {};
				});
				return {category};
			});
			return {submenu};
		});
		context.Bind("menu", "add_always_draw_imgui", [&](const Values& args) -> Values {
			alwaysDraw.push_back(args[0]);
			return {};
		});
		BindRenderApi(context);
		BindCloudSaves(context);
		InitializeBlips(context);
		InitializeUnlocks(context);
		InitializeTunables(context);
		InitializeTattoos(context);
		InitializeSettings(context);
		InitializeFeatures(context);
		Check(commandCount == 96 && categoryCount == 7, "Native startup is incomplete");
		std::cout << "PASS: 96 native commands and seven categories register without LuaJIT\n";
		// Check the actual bindings so enum flags cannot silently become true/false.
		auto checkEnum = [&](const char* table, const char* name, auto expected) {
			auto value = Member(context.Global(table), name);
			Check(std::holds_alternative<double>(value.data), "ImGui enum was stored as a boolean");
			Check(Number(value) == static_cast<double>(expected), "ImGui enum value changed");
		};
		checkEnum("ImGuiStyleVar", "WindowTitleAlign", ImGuiStyleVar_WindowTitleAlign);
		checkEnum("ImGuiInputTextFlags", "ReadOnly", ImGuiInputTextFlags_ReadOnly);
		checkEnum("ImGuiCol", "Button", ImGuiCol_Button);
		checkEnum("ImGuiCol", "ButtonActive", ImGuiCol_ButtonActive);
		checkEnum("ImGuiCol", "ButtonHovered", ImGuiCol_ButtonHovered);
		checkEnum("ImGuiTabBarFlags", "FittingPolicyScroll", ImGuiTabBarFlags_FittingPolicyScroll);
		checkEnum("ImGuiTabBarFlags", "NoTabListScrollingButtons", ImGuiTabBarFlags_NoTabListScrollingButtons);
		checkEnum("ImGuiWindowFlags", "NoMove", ImGuiWindowFlags_NoMove);
		checkEnum("ImGuiWindowFlags", "NoResize", ImGuiWindowFlags_NoResize);
		enum class SignedFlag : int { Negative = -2, Zero = 0 };
		Check(Number(Value(SignedFlag::Negative)) == -2 && Number(Value(SignedFlag::Zero)) == 0, "Scoped enum conversion changed");
		Check(std::holds_alternative<bool>(Value(true).data), "Boolean storage changed");
		std::cout << "PASS: ImGui enum bindings preserve numeric flags and indices\n";
		Check(!Truthy(Member(context.Global("TS_Settings"), "HGWHrestock")) && Number(Member(context.Global("TS_Settings"), "HGgoodtype")) == 6, "Earlier settings were not migrated");
		Check(Truthy(Equal(Modulo(-5, 3), 1)), "Negative modulo differs from the original behavior");
		auto match = Invoke(Value("MPX_TATTOO_FM_CURRENT_1 = -42"), "match", {"^(.-) = (.-)$"});
		Check(Text(At(match, 0)) == "MPX_TATTOO_FM_CURRENT_1" && Text(At(match, 1)) == "-42", "Tattoo import parsing failed");
		Check(Text(First(Call(Member(context.Global("string"), "format"), {"%d / %.2f", 42, 2.5}))) == "42 / 2.50", "Number formatting changed");
		std::cout << "PASS: one-based tables, negative modulo, formatting and tattoo parsing\n";
		Check(Integer(2147483648.0) == INT_MIN && UnsignedBits(-1) == 0xffffffffu, "Integer wrapping changed");
		Check(Number(First(Call(Member(context.Global("bit"), "bor"), {-2147483648.0, 1}))) == -2147483647.0, "Signed bit operations changed");
		Call(specifications.at("fastrespawn")[4], {});
		Check(Number(memory[2709139]) == 1, "Fast Respawn enable callback changed");
		Call(specifications.at("fastrespawn")[5], {});
		Check(Number(memory[2709139]) == 0, "Fast Respawn disable callback changed");
		Call(specifications.at("m25_runboost")[4], {});
		Check((Integer(memory[2673276 + 3768]) & (1 << 10)) != 0, "Run Boost failed to set its bit");
		Call(specifications.at("m25_runboost")[5], {});
		Check((Integer(memory[2673276 + 3768]) & (1 << 10)) == 0, "Run Boost failed to clear its bit");
		Call(specifications.at("clearworld")[4], {});
		Check(Number(memory[1690470]) == 1, "Clear World enable callback changed");
		Call(specifications.at("clearworld")[5], {});
		Check(Number(memory[1690470]) == 0, "Clear World disable callback changed");
		std::cout << "PASS: integer masks and reversible Fast Respawn, Run Boost and Clear World callbacks\n";
		SaveNativeSettings(context, true, MakeTable({{"LoadSettings", true}, {"CloseBCSonUnload", false}}));
		InitializeSettings(context);
		Call(context.Global("require"), {"TS_Settings"});
		Check(Truthy(Member(context.Global("TS_Settings"), "NoClipWindowTop")) && !Truthy(Member(context.Global("TS_Settings"), "CloseBCSonUnload")), "Saved settings did not round-trip");
		Check(std::filesystem::exists(root / "TinkerScripts/Settings.json"), "Native settings file was not created");
		std::cout << "PASS: earlier settings migration and JSON save/load\n";
		for (auto& [name, callback] : draw)
		{
			ImGui::NewFrame();
			ImGui::SetNextWindowSize({1800, 1000});
			ImGui::Begin(name.c_str());
			Call(callback, {});
			ImGui::End();
			ImGui::Render();
		}
		ImGui::DestroyContext();
		std::cout << "PASS: all category render callbacks complete in headless ImGui\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
