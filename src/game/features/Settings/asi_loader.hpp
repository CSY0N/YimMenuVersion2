#pragma once

#include <Windows.h>

#include <filesystem>
#include <string>
#include <vector>

namespace asi_loader
{
    namespace fs = std::filesystem;

    enum class status
    {
        unloaded,
        loaded,
        failed
    };

    struct plugin
    {
        fs::path path;
        std::string name;

        HMODULE module = nullptr;
        status loadStatus = status::unloaded;

        DWORD lastError = ERROR_SUCCESS;
        std::string errorMessage;
    };
    bool initialize();
    void shutdown();
    fs::path get_default_directory();
    bool scan_default_directory();
    bool scan_directory(const fs::path& directory);
    bool select_asi_files(HWND ownerWindow = nullptr);
    bool add_asi(const fs::path& path);
    bool load_asi(const fs::path& path);
    bool load_asi(std::size_t index);
    std::size_t load_all();
    bool unload_asi(const fs::path& path);
    bool unload_asi(std::size_t index);
    std::size_t unload_all();
    bool reload_asi(const fs::path& path);
    bool reload_asi(std::size_t index);
    std::size_t reload_all();
    bool remove_asi(const fs::path& path);
    bool remove_asi(std::size_t index);
    void clear_unloaded();
    bool is_loaded(const fs::path& path);
    std::vector<plugin> get_plugins();
    void unload();
}
