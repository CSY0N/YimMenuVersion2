#include "LuaDownloader.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/frontend/items/Items.hpp"
#include <Windows.h>
#include <ShlObj.h>
#include <Shellapi.h>
#include <urlmon.h>
#include <filesystem>
#include <mutex>
#include <string>
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")

namespace YimMenu::Submenus
{
    static std::filesystem::path GetLuaScriptsFolder()
    {
        char appDataPath[MAX_PATH]{};
		const HRESULT result = SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appDataPath);

        if (FAILED(result))
            return {};

        return std::filesystem::path(appDataPath)
            / "YimMenuV2"
            / "Scripts";
    }

    static bool IsValidLuaFileName(const std::string& fileName)
    {
        if (fileName.empty())
            return false;
        const std::filesystem::path path(fileName);
        if (path.has_parent_path())
            return false;
        return true;
    }

static std::string ReplaceAll(std::string value, const std::string& from, const std::string& to)
	{
	if (from.empty())
		return value;

    std::size_t position = 0;

    while ((position = value.find(from, position)) != std::string::npos)
    {
        value.replace(position, from.length(), to);
        position += to.length();
    }

    return value;
}

static std::string UrlEncodeFilePath(std::string path)
{
	path = ReplaceAll(path, " ", "%20");
	path = ReplaceAll(path, "#", "%23");

	return path;
}

static std::string BuildGitHubRawUrl(std::string url, const std::string& fileName)
{
	if (url.find("raw.githubusercontent.com") != std::string::npos)
		return UrlEncodeFilePath(url);
	if (url.find("github.com") != std::string::npos && url.find("/blob/") != std::string::npos)
	{
		url = ReplaceAll(
			url,
			"https://github.com/",
			"https://raw.githubusercontent.com/");

		url = ReplaceAll(url, "/blob/", "/");
		return UrlEncodeFilePath(url);
	}
	if (url.find("github.com") != std::string::npos)
	{
		while (!url.empty() && url.back() == '/')
			url.pop_back();
		url = ReplaceAll(url, "https://github.com/", "https://raw.githubusercontent.com/");
		return UrlEncodeFilePath(
			url + "/main/" + fileName);
	}

	return UrlEncodeFilePath(url);
}

static bool DownloadLuaScript(const std::string& enteredUrl, std::string fileName)
{
	if (enteredUrl.empty() || fileName.empty())
		return false;

	const auto scriptsFolder = GetLuaScriptsFolder();

	if (scriptsFolder.empty())
		return false;

	std::error_code error;
	std::filesystem::create_directories(scriptsFolder, error);

	if (error)
		return false;

	std::filesystem::path filePath = scriptsFolder / fileName;

	if (filePath.extension() != ".lua")
		filePath.replace_extension(".lua");

	fileName = filePath.filename().string();

	const std::string downloadUrl = BuildGitHubRawUrl(enteredUrl, fileName);

	std::filesystem::remove(filePath, error);
	error.clear();
	const HRESULT result = URLDownloadToFileA(nullptr, downloadUrl.c_str(), filePath.string().c_str(), 0, nullptr);

	if (FAILED(result))
	{
		std::filesystem::remove(filePath, error);
		return false;
	}

	std::ifstream downloadedFile(filePath, std::ios::binary);

	if (!downloadedFile.is_open())
	{
		std::filesystem::remove(filePath, error);
		return false;
	}

	std::string beginning(512, '\0');

	downloadedFile.read(beginning.data(), static_cast<std::streamsize>(beginning.size()));

	beginning.resize(static_cast<std::size_t>(downloadedFile.gcount()));

	downloadedFile.close();

	if (beginning.find("<!DOCTYPE html") != std::string::npos || beginning.find("<html") != std::string::npos || beginning.find("<HTML") != std::string::npos)
	{
		std::filesystem::remove(filePath, error);
		return false;
	}

	return std::filesystem::exists(filePath) && std::filesystem::file_size(filePath, error) > 0;
}

    std::shared_ptr<Category> BuildLuaDownloaderMenu()
    {
        auto menu = std::make_shared<Category>("Lua Downloader");
        auto downloaderGroup =
            std::make_shared<Group>("Download Lua Scripts");

        downloaderGroup->AddItem(std::make_unique<ImGuiItem>([]
        {
            static char luaUrl[512]{};
            static char luaFileName[128]{};

            static std::mutex statusMutex;
            static std::string statusMessage;
            static bool downloadSuccess = false;
            static bool downloading = false;

            ImGui::TextWrapped("Download Lua scripts directly into:");

            ImGui::TextDisabled(
			    "AppData\\Roaming\\YimMenuV2\\Scripts");

            ImGui::Separator();

            ImGui::InputTextWithHint("##LuaDownloadUrl", "GitHub raw link or direct .lua link", luaUrl, sizeof(luaUrl));
			ImGui::InputTextWithHint("##LuaFileName", "File name, for example Script.lua", luaFileName, sizeof(luaFileName));

            bool isDownloading = false;
            {
                std::scoped_lock lock(statusMutex);
                isDownloading = downloading;
            }

            if (isDownloading)
				ImGui::BeginDisabled();

            if (ImGui::Button("Download Lua Script"))
            {
                const std::string url(luaUrl);
                const std::string fileName(luaFileName);

                if (url.empty())
                {
                    std::scoped_lock lock(statusMutex);

                    downloadSuccess = false;
                    statusMessage = "Enter a direct Lua script URL.";
                }
                else if (!IsValidLuaFileName(fileName))
                {
                    std::scoped_lock lock(statusMutex);

                    downloadSuccess = false;
                    statusMessage = "Enter a valid Lua file name.";
                }
                else
                {
                    {
                        std::scoped_lock lock(statusMutex);

                        downloading = true;
                        downloadSuccess = false;
                        statusMessage = "Downloading...";
                    }

                    FiberPool::Push([url, fileName]
                    {
                        const bool success =
                            DownloadLuaScript(url, fileName);

                        std::scoped_lock lock(statusMutex);

                        downloading = false;
                        downloadSuccess = success;

                        if (success)
                        {
							statusMessage = "Lua script downloaded successfully.";
                        }
                        else
                        {
							statusMessage = "Download failed. Use a direct or GitHub raw link.";
                        }
                    });
                }
            }

            if (isDownloading)
                ImGui::EndDisabled();

            ImGui::SameLine();

            if (ImGui::Button("Open Scripts Folder"))
            {
                const auto scriptsFolder = GetLuaScriptsFolder();

                if (!scriptsFolder.empty())
                {
                    std::error_code error;

                    std::filesystem::create_directories(
                        scriptsFolder,
                        error
                    );

                    if (!error)
                    {
						const std::string folder = scriptsFolder.string();
						ShellExecuteA(nullptr, "open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                    }
                }
            }

            std::string displayedStatus;
            bool displayedSuccess = false;
            bool displayedDownloading = false;
            {
                std::scoped_lock lock(statusMutex);

                displayedStatus = statusMessage;
                displayedSuccess = downloadSuccess;
                displayedDownloading = downloading;
            }

            if (!displayedStatus.empty())
            {
                ImGui::Separator();

                if (displayedSuccess)
                {
					ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", displayedStatus.c_str());
                }
                else if (displayedDownloading)
                {
					ImGui::TextWrapped("%s", displayedStatus.c_str());
                }
                else
                {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", displayedStatus.c_str());
                }
            }
        }));

        menu->AddItem(std::move(downloaderGroup));
        return menu;
    }
}
