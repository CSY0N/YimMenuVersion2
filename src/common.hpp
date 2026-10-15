#pragma once

// The editor parser lacks the detailed source_location intrinsic on some VS versions.
// Use the STL's basic function-name mode for IntelliSense; compiled logging is unchanged.
#if defined(__INTELLISENSE__) && defined(_MSC_VER)
#ifndef _USE_DETAILED_FUNCTION_NAME_IN_SOURCE_LOCATION
#define _USE_DETAILED_FUNCTION_NAME_IN_SOURCE_LOCATION 0
#endif
#endif

#include <AsyncLogger/Logger.hpp>
#include <MinHook.h>
#include <windows.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <imgui.h>
#include <iostream>
#include <map>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>
#include <stack>
#include <unordered_set>

#include <nlohmann/json.hpp>

using namespace al;
#include "core/logger/LogHelper.hpp"

#undef Yield
using json = nlohmann::json;
namespace YimMenu
{
	using namespace std::chrono_literals;
	using namespace std::string_view_literals;
    inline uint16_t g_failed_sig_count{};
    inline uint16_t g_found_sig_count{};
    inline uint16_t g_total_sig_count{};
	extern std::atomic<bool> g_Running;
	extern HINSTANCE g_DllInstance;
	extern HANDLE g_MainThread;
	extern DWORD g_MainThreadId;
}
