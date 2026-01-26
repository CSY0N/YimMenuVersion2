#include "Updater.hpp"
#include "core/backend/ScriptMgr.hpp"
#include "core/frontend/Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include <winhttp.h>
#include <thread>
#include <format>
#pragma comment(lib, "winhttp.lib")
#define CURRENT_VERSION "Csyon'sYimMenuV2"
#define GITHUB_API L"https://api.github.com/repos/CSY0N/YimMenuVersion2/releases/latest"
namespace YimMenu::Updater
{
	static std::string HttpGet(const wchar_t* url)
	{
		std::string result;

		HINTERNET hSession = WinHttpOpen(
		    L"Updater",
		    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		    WINHTTP_NO_PROXY_NAME,
		    WINHTTP_NO_PROXY_BYPASS,
		    0);

		if (!hSession)
			return {};

		URL_COMPONENTS uc{};
		uc.dwStructSize = sizeof(uc);

		wchar_t host[256]{};
		wchar_t path[1024]{};

		uc.lpszHostName = host;
		uc.dwHostNameLength = _countof(host);
		uc.lpszUrlPath = path;
		uc.dwUrlPathLength = _countof(path);

		if (!WinHttpCrackUrl(url, 0, 0, &uc))
		{
			WinHttpCloseHandle(hSession);
			return {};
		}

		HINTERNET hConnect = WinHttpConnect(
		    hSession,
		    uc.lpszHostName,
		    uc.nPort,
		    0);

		if (!hConnect)
		{
			WinHttpCloseHandle(hSession);
			return {};
		}

		HINTERNET hRequest = WinHttpOpenRequest(
		    hConnect,
		    L"GET",
		    uc.lpszUrlPath,
		    nullptr,
		    WINHTTP_NO_REFERER,
		    WINHTTP_DEFAULT_ACCEPT_TYPES,
		    uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);

		if (!hRequest)
		{
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return {};
		}

		WinHttpAddRequestHeaders(
		    hRequest,
		    L"User-Agent: Updater\r\n",
		    -1,
		    WINHTTP_ADDREQ_FLAG_ADD);

		if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hRequest, nullptr))
		{
			DWORD size = 0;
			while (WinHttpQueryDataAvailable(hRequest, &size) && size > 0)
			{
				std::string buffer(size, '\0');
				DWORD read = 0;

				if (WinHttpReadData(hRequest, buffer.data(), size, &read))
				{
					buffer.resize(read);
					result += buffer;
				}
			}
		}

		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);

		return result;
	}
	static std::string ExtractTagName(const std::string& json)
	{
		const std::string key = "\"tag_name\"";
		auto pos = json.find(key);
		if (pos == std::string::npos)
			return {};

		pos = json.find(':', pos);
		if (pos == std::string::npos)
			return {};

		pos = json.find('"', pos);
		if (pos == std::string::npos)
			return {};

		const auto start = pos + 1;
		const auto end = json.find('"', start);
		if (end == std::string::npos)
			return {};

		return json.substr(start, end - start);
	}
	void CheckForUpdates()
	{
		std::thread([] {
			const auto response = HttpGet(GITHUB_API);
			if (response.empty())
				return;

			const auto latestTag = ExtractTagName(response);
			if (latestTag.empty())
				return;

			if (latestTag != CURRENT_VERSION)
			{
					Notifications::Show("YimMenuV2 Update Available",
					std::format("New version available: {}\nVisit GitHub releases.",latestTag),
				    NotificationType::Warning);
			}
			else
			{
				g_log.send("INFO", "YimMenuV2 is up to date");
			}
		}).detach();
	}
}

