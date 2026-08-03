#include "asi_loader.hpp"
#include <shlobj.h>
#include <commdlg.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <mutex>
#include <system_error>
#include <cwctype>
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Shell32.lib")

namespace asi_loader
{
    namespace
    {
        std::vector<plugin> g_plugins;
        std::mutex g_pluginMutex;

        constexpr std::size_t FILE_DIALOG_BUFFER_SIZE = 65536;

        void log_message(const std::string& message)
		{
			const std::string output = "[YimMenuV2 ASI Loader] " + message + "\n";
			OutputDebugStringA(output.c_str());
		};

        std::string get_windows_error_message(const DWORD error)
        {
            if (error == ERROR_SUCCESS)
                return {};

            LPSTR messageBuffer = nullptr;
			const DWORD size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

            if (size == 0 || !messageBuffer)
                return "Unknown Windows error.";

            std::string message(messageBuffer, size);
            LocalFree(messageBuffer);

            while (!message.empty() && (message.back() == '\r' || message.back() == '\n' || message.back() == ' '))
			{
				message.pop_back();
			}

            return message;
        }

        bool is_asi_file(const fs::path& path)
        {
            if (!path.has_extension())
                return false;

            std::string extension = path.extension().string();

            std::transform(
			    extension.begin(),
			    extension.end(),
			    extension.begin(),
			    [](const unsigned char character) {
				    return static_cast<char>(std::tolower(character));
			    });

            return extension == ".asi";
        }

        fs::path normalize_path(const fs::path& path)
        {
            std::error_code error;

            fs::path normalized = fs::weakly_canonical(path, error);

            if (!error)
                return normalized;

            normalized = fs::absolute(path, error);

            if (!error)
                return normalized.lexically_normal();

            return path.lexically_normal();
        }

        bool paths_equal(
		    const fs::path& first,
		    const fs::path& second)
		{
			const std::wstring firstPath =
			    normalize_path(first).wstring();

			const std::wstring secondPath =
			    normalize_path(second).wstring();

			return _wcsicmp(
			           firstPath.c_str(),
			           secondPath.c_str())
			    == 0;
		}

        auto find_plugin_unlocked(const fs::path& path)
        {
			return std::find_if(
			    g_plugins.begin(),
			    g_plugins.end(),
			    [&path](const plugin& currentPlugin) {
				    return paths_equal(
				        currentPlugin.path,
				        path);
			    });
        }

        bool validate_asi_path(
            const fs::path& path,
            std::string& errorMessage)
        {
			if (path.empty())
			{
				errorMessage = "The ASI path is empty.";
				return false;
			}

            if (!is_asi_file(path))
            {
				errorMessage =
				    "The selected file does not have an .asi extension.";

                return false;
            }

            std::error_code error;

            if (!fs::exists(path, error) || error)
			{
				errorMessage =
				    "The selected ASI file does not exist.";

				return false;
			}

            if (!fs::is_regular_file(path, error) || error)
			{
				errorMessage =
				    "The selected ASI path is not a regular file.";

				return false;
			}

            return true;
        }

       bool load_plugin_unlocked(plugin& currentPlugin)
		{
		   if (currentPlugin.module)
		   {
			   currentPlugin.loadStatus = status::loaded;
			   currentPlugin.lastError = ERROR_SUCCESS;
			   currentPlugin.errorMessage.clear();

			   return true;
		   }

			std::string validationError;

			if (!validate_asi_path(
			        currentPlugin.path,
			        validationError))
			{
				currentPlugin.module = nullptr;
				currentPlugin.loadStatus = status::failed;
				currentPlugin.lastError = ERROR_FILE_NOT_FOUND;
				currentPlugin.errorMessage = validationError;

				log_message(
				    "Failed to load \"" + currentPlugin.path.string() + "\": " + validationError);

				return false;
			}

			const fs::path normalizedPath = normalize_path(currentPlugin.path);

			const fs::path pluginDirectory = normalizedPath.parent_path();

			SetLastError(ERROR_SUCCESS);
			HMODULE module = LoadLibraryExW(normalizedPath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
			if (!module)
			{
				const DWORD firstError = GetLastError();

				log_message("Modern loading method failed for \"" + normalizedPath.string() + "\". Error " + std::to_string(firstError) + ". Trying fallback.");
		        module = LoadLibraryExW(normalizedPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
			}

			if (!module)
			{
				const DWORD error = GetLastError();

				currentPlugin.module = nullptr;
				currentPlugin.loadStatus = status::failed;
				currentPlugin.lastError = error;
				currentPlugin.errorMessage =
				    get_windows_error_message(error);

				log_message("Failed to load \"" + normalizedPath.string() + "\". Windows error " + std::to_string(error) + ": " + currentPlugin.errorMessage);

				return false;
			}

			currentPlugin.path = normalizedPath;
			currentPlugin.module = module;
			currentPlugin.loadStatus = status::loaded;
			currentPlugin.lastError = ERROR_SUCCESS;
			currentPlugin.errorMessage.clear();

			log_message("Successfully loaded \"" + normalizedPath.string() + "\". Module address: " + std::to_string(reinterpret_cast<std::uintptr_t>(module)));

			return true;
		}

        bool unload_plugin_unlocked(plugin& currentPlugin)
        {
            if (!currentPlugin.module)
            {
                currentPlugin.loadStatus = status::unloaded;
                currentPlugin.lastError = ERROR_SUCCESS;
                currentPlugin.errorMessage.clear();

                return true;
            }

            SetLastError(ERROR_SUCCESS);

            if (!FreeLibrary(currentPlugin.module))
            {
                const DWORD error = GetLastError();

                currentPlugin.loadStatus = status::failed;
                currentPlugin.lastError = error;
                currentPlugin.errorMessage =
                    get_windows_error_message(error);
				log_message("Failed to unload \"" + currentPlugin.path.string() + "\". Error " + std::to_string(error) + ": " + currentPlugin.errorMessage);

                return false;
            }

            currentPlugin.module = nullptr;
            currentPlugin.loadStatus = status::unloaded;
            currentPlugin.lastError = ERROR_SUCCESS;
            currentPlugin.errorMessage.clear();

            log_message("Unloaded \"" + currentPlugin.path.string() + "\".");

            return true;
        }
    }

    fs::path get_default_directory()
	{
		wchar_t appDataPath[MAX_PATH]{};

		const HRESULT result = SHGetFolderPathW(
		    nullptr,
		    CSIDL_APPDATA,
		    nullptr,
		    SHGFP_TYPE_CURRENT,
		    appDataPath);

		if (FAILED(result))
		{
			log_message("Failed to locate the AppData Roaming folder.");

			return {};
		}

		return fs::path(appDataPath) / L"YimMenuV2" / L"asi";
	}

    bool initialize()
	{
		const fs::path directory = get_default_directory();
		if (directory.empty())
			return false;

		std::error_code error;
		fs::create_directories(directory, error);

		if (error)
		{
			log_message("Failed to create ASI directory \"" + directory.string() + "\": " + error.message());
			return false;
		}
		log_message("ASI directory: \"" + directory.string() + "\".");
		if (!scan_directory(directory))
			return false;
		const std::size_t loadedCount = load_all();
		log_message("Automatically loaded " + std::to_string(loadedCount) + " ASI plugin(s).");

		return true;
	}

    void shutdown()
    {
        unload_all();

        const std::scoped_lock lock(g_pluginMutex);
        g_plugins.clear();

        log_message("Loader shutdown completed.");
    }

    bool scan_default_directory()
    {
		const fs::path directory = get_default_directory();

        if (directory.empty())
            return false;

        return scan_directory(directory);
    }

    bool scan_directory(const fs::path& directory)
	{
		if (directory.empty())
		{
			log_message("Cannot scan an empty directory path.");

			return false;
		}

		std::error_code error;

		if (!fs::exists(directory, error) || error)
		{
			log_message("ASI directory does not exist: \"" + directory.string() + "\".");

			return false;
		}

		if (!fs::is_directory(directory, error) || error)
		{
			log_message("ASI path is not a directory: \"" + directory.string() + "\".");

			return false;
		}

		std::size_t discoveredCount = 0;
		fs::directory_iterator iterator(directory, fs::directory_options::skip_permission_denied, error);

		const fs::directory_iterator end;

		while (!error && iterator != end)
		{
			const fs::directory_entry& entry = *iterator;

			std::error_code entryError;

			if (entry.is_regular_file(entryError) && !entryError && is_asi_file(entry.path()))
			{
				if (add_asi(entry.path()))
					++discoveredCount;
			}

			iterator.increment(error);
		}

		if (error)
		{
			log_message("Directory scan ended with an error: " + error.message());

			return false;
		}
		log_message("Scan completed. Added " + std::to_string(discoveredCount) + " new ASI plugin(s).");
		return true;
	}
	bool select_asi_files(HWND ownerWindow)
	{
		std::vector<wchar_t> fileBuffer(
		    FILE_DIALOG_BUFFER_SIZE,
		    L'\0');

         const fs::path destinationDirectory = get_default_directory();

    if (destinationDirectory.empty())
    {
		log_message("Cannot select ASI files because the default directory could not be resolved.");

        return false;
    }

    std::error_code directoryError;

    fs::create_directories(
        destinationDirectory,
        directoryError);

    if (directoryError)
    {
		log_message("Failed to create the ASI directory: " + directoryError.message());

        return false;
    }

    std::wstring initialDirectory = destinationDirectory.wstring();

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = ownerWindow;

    dialog.lpstrFilter =
        L"ASI Plugins (*.asi)\0*.asi\0"
        L"All Files (*.*)\0*.*\0";

    dialog.lpstrFile = fileBuffer.data();
    dialog.nMaxFile = static_cast<DWORD>(fileBuffer.size());
    dialog.lpstrTitle = L"Select ASI Plugins";
    dialog.lpstrInitialDir = initialDirectory.c_str();
    dialog.Flags =
        OFN_EXPLORER |
        OFN_FILEMUSTEXIST |
        OFN_PATHMUSTEXIST |
        OFN_ALLOWMULTISELECT |
        OFN_DONTADDTORECENT;

    if (!GetOpenFileNameW(&dialog))
    {
		const DWORD dialogError = CommDlgExtendedError();

        if (dialogError != 0)
        {
			log_message("ASI selection dialog failed. Error: " + std::to_string(dialogError));
        }

        return false;
    }

    const wchar_t* current = fileBuffer.data();
    const fs::path firstEntry = current;

    current += std::wcslen(current) + 1;
    std::vector<fs::path> selectedFiles;

    if (*current == L'\0')
    {
        selectedFiles.push_back(firstEntry);
    }
    else
    {
		const fs::path selectedDirectory =
			firstEntry;

        while (*current != L'\0')
        {
			selectedFiles.emplace_back(selectedDirectory / current);
            current += std::wcslen(current) + 1;
        }
    }

    std::size_t copiedCount = 0;
    std::size_t loadedCount = 0;

    for (const fs::path& selectedFile :
         selectedFiles)
    {
        if (!is_asi_file(selectedFile))
        {
			log_message("Skipped non-ASI file: \"" + selectedFile.string() + "\".");

            continue;
        }

        const fs::path destinationPath = destinationDirectory / selectedFile.filename();

        std::error_code copyError;

        if (!paths_equal(
                selectedFile,
                destinationPath))
		{
            fs::copy_file(
                selectedFile,
                destinationPath,
                fs::copy_options::overwrite_existing,
                copyError);

            if (copyError)
            {
				log_message("Failed to copy \"" + selectedFile.string() + "\" to \"" + destinationPath.string() + "\": " + copyError.message());

                continue;
            }

            ++copiedCount;
        }

        const fs::path pathToLoad = normalize_path(destinationPath);

        if (load_asi(pathToLoad))
		{
			++loadedCount;
		}
		else if (is_loaded(pathToLoad))
		{
			++loadedCount;
		}
    }
	   log_message("Selected " + std::to_string(selectedFiles.size()) + " file(s), copied " + std::to_string(copiedCount) + ", loaded " + std::to_string(loadedCount) + ".");
       return loadedCount > 0;
	}

    bool add_asi(const fs::path& path)
    {
        std::string errorMessage;

        if (!validate_asi_path(path, errorMessage))
        {
			log_message("Cannot add \"" + path.string() + "\": " + errorMessage);

            return false;
        }

        const fs::path normalizedPath = normalize_path(path);

        const std::scoped_lock lock(g_pluginMutex);

        if (find_plugin_unlocked(normalizedPath) !=
            g_plugins.end())
        {
			log_message("Plugin is already registered: \"" + normalizedPath.string() + "\".");

            return false;
        }

        plugin newPlugin;
        newPlugin.path = normalizedPath;
        newPlugin.name =
            normalizedPath.filename().string();
        newPlugin.module = nullptr;
        newPlugin.loadStatus = status::unloaded;
        newPlugin.lastError = ERROR_SUCCESS;

        g_plugins.push_back(std::move(newPlugin));
        log_message("Registered \"" + normalizedPath.string() + "\".");

        return true;
    }

    bool load_asi(const fs::path& path)
    {
		const fs::path normalizedPath = normalize_path(path);

        {
            const std::scoped_lock lock(g_pluginMutex);

            const auto iterator = find_plugin_unlocked(normalizedPath);

            if (iterator != g_plugins.end())
				return load_plugin_unlocked(*iterator);
        }

        if (!add_asi(normalizedPath))
            return false;

        const std::scoped_lock lock(g_pluginMutex);

        const auto iterator = find_plugin_unlocked(normalizedPath);

        if (iterator == g_plugins.end())
            return false;

        return load_plugin_unlocked(*iterator);
    }

    bool load_asi(const std::size_t index)
    {
        const std::scoped_lock lock(g_pluginMutex);

        if (index >= g_plugins.size())
        {
			log_message("Cannot load plugin: invalid index.");

            return false;
        }

        return load_plugin_unlocked(g_plugins[index]);
    }

    std::size_t load_all()
    {
        const std::scoped_lock lock(g_pluginMutex);

        std::size_t loadedCount = 0;

        for (plugin& currentPlugin : g_plugins)
        {
            if (load_plugin_unlocked(currentPlugin))
                ++loadedCount;
        }

        log_message("Load-all completed. " + std::to_string(loadedCount) + " of " + std::to_string(g_plugins.size()) + " plugin(s) are loaded.");
        return loadedCount;
    }

    bool unload_asi(const fs::path& path)
    {
        const std::scoped_lock lock(g_pluginMutex);

        const auto iterator =
            find_plugin_unlocked(path);

        if (iterator == g_plugins.end())
        {
			log_message("Cannot unload unregistered plugin: \"" + path.string() + "\".");

            return false;
        }

        return unload_plugin_unlocked(*iterator);
    }

    bool unload_asi(const std::size_t index)
    {
        const std::scoped_lock lock(g_pluginMutex);

        if (index >= g_plugins.size())
        {
			log_message("Cannot unload plugin: invalid index.");

            return false;
        }

        return unload_plugin_unlocked(g_plugins[index]);
    }

    std::size_t unload_all()
    {
        const std::scoped_lock lock(g_pluginMutex);

        std::size_t unloadedCount = 0;

        for (auto iterator = g_plugins.rbegin();
             iterator != g_plugins.rend();
             ++iterator)
        {
            if (unload_plugin_unlocked(*iterator))
                ++unloadedCount;
        }
		log_message("Unload-all completed. Processed " + std::to_string(unloadedCount) + " plugin(s).");

        return unloadedCount;
    }

    bool reload_asi(const fs::path& path)
    {
        const std::scoped_lock lock(g_pluginMutex);

        const auto iterator = find_plugin_unlocked(path);

        if (iterator == g_plugins.end())
        {
			log_message("Cannot reload unregistered plugin: \"" + path.string() + "\".");

            return false;
        }

        if (!unload_plugin_unlocked(*iterator))
            return false;

        return load_plugin_unlocked(*iterator);
    }

    bool reload_asi(const std::size_t index)
    {
        const std::scoped_lock lock(g_pluginMutex);

        if (index >= g_plugins.size())
        {
			log_message("Cannot reload plugin: invalid index.");

            return false;
        }

        plugin& currentPlugin =
            g_plugins[index];

        if (!unload_plugin_unlocked(currentPlugin))
            return false;

        return load_plugin_unlocked(currentPlugin);
    }

    std::size_t reload_all()
    {
        const std::scoped_lock lock(g_pluginMutex);

        std::size_t reloadedCount = 0;

        for (plugin& currentPlugin : g_plugins)
        {
            if (!unload_plugin_unlocked(currentPlugin))
                continue;

            if (load_plugin_unlocked(currentPlugin))
                ++reloadedCount;
        }
		log_message("Reload-all completed. Reloaded " + std::to_string(reloadedCount) + " plugin(s).");

        return reloadedCount;
    }

    bool remove_asi(const fs::path& path)
    {
        const std::scoped_lock lock(g_pluginMutex);

        const auto iterator = find_plugin_unlocked(path);

        if (iterator == g_plugins.end())
            return false;

        if (iterator->module)
        {
			log_message("Unload the plugin before removing it: \"" + iterator->path.string() + "\".");

            return false;
        }

        log_message("Removed \"" + iterator->path.string() + "\".");

        g_plugins.erase(iterator);

        return true;
    }

    bool remove_asi(const std::size_t index)
    {
        const std::scoped_lock lock(g_pluginMutex);

        if (index >= g_plugins.size())
            return false;

        if (g_plugins[index].module)
        {
			log_message("Unload the plugin before removing it.");

            return false;
        }

        g_plugins.erase(g_plugins.begin() + static_cast<std::ptrdiff_t>(index));

        return true;
    }

    void clear_unloaded()
    {
        const std::scoped_lock lock(g_pluginMutex);

        std::erase_if(
		    g_plugins,
		    [](const plugin& currentPlugin) {
			    return currentPlugin.module == nullptr;
		    });
    }

    bool is_loaded(const fs::path& path)
    {
        const std::scoped_lock lock(g_pluginMutex);

        const auto iterator =
            find_plugin_unlocked(path);

        return iterator != g_plugins.end() && iterator->module != nullptr;
    }

    std::vector<plugin> get_plugins()
    {
        const std::scoped_lock lock(g_pluginMutex);
        return g_plugins;
    }

    void unload()
    {
        shutdown();
    }
}
