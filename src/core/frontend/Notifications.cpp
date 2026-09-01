#include "Notifications.hpp"
#include "core/backend/FiberPool.hpp"
#include "game/frontend/Menu.hpp"
#include "core/util/Joaat.hpp"
#include <algorithm>
#include <utility>

namespace YimMenu
{
	static ImVec4 GetNotificationColor(NotificationType type)
	{
		switch (type)
		{
		case NotificationType::Success:
			return ImVec4(0.20f, 0.85f, 0.35f, 1.00f);
		case NotificationType::Warning:
			return ImVec4(1.00f, 0.65f, 0.15f, 1.00f);
		case NotificationType::Error:
			return ImVec4(1.00f, 0.25f, 0.25f, 1.00f);
		case NotificationType::Info:
		default:
			return ImVec4(0.25f, 0.65f, 1.00f, 1.00f);
		}
	}

	static const char* GetNotificationIcon(NotificationType type)
	{
		switch (type)
		{
		case NotificationType::Info:
			return "\xef\x81\x9a";
		case NotificationType::Success:
			return "\xef\x81\x98";
		case NotificationType::Warning:
			return "\xef\x81\xb1";
		case NotificationType::Error:
			return "\xef\x81\x97";
		default:
			return "";
		}
	}

	Notification Notifications::ShowImpl(const std::string& title, const std::string& message, NotificationType type, int duration, std::function<void()> contextFunction, const std::string& contextFunctionName)
	{
		if (title.empty() || message.empty())
			return {};

		const std::string key = title + message;
		const auto identifier = Joaat(key);
		const auto now = std::chrono::system_clock::now();

		std::scoped_lock lock(m_Mutex);

		auto it = m_Notifications.find(key);

		if (it != m_Notifications.end())
		{
			it->second.m_CreatedOn = now;
			it->second.m_Duration = duration;
			it->second.m_Erasing = false;
			return it->second;
		}

		Notification notification{};

		notification.m_Title = title;
		notification.m_Message = message;
		notification.m_Type = type;
		notification.m_CreatedOn = now;
		notification.m_Duration = duration;
		notification.m_Identifier = identifier;
		notification.m_AnimationOffset = -m_CardSizeX;
		notification.m_Erasing = false;

		if (contextFunction)
		{
			notification.m_ContextFunc = std::move(contextFunction);
			notification.m_ContextFuncName = contextFunctionName.empty() ? "Context Function" : contextFunctionName;
		}

		m_Notifications.emplace(key, notification);

		return notification;
	}

	bool Notifications::EraseImpl(const Notification& notification)
	{
		std::scoped_lock lock(m_Mutex);

		for (auto& [_, current] : m_Notifications)
		{
			if (current.m_Identifier != notification.m_Identifier)
				continue;

			current.m_Erasing = true;
			return true;
		}

		return false;
	}

	static void DrawNotification(Notification& notification, int position)
	{
		constexpr float cardWidth = 380.f;
		constexpr float cardHeight = 92.f;

		const float x = 14.f;
		const float y = 14.f + static_cast<float>(position) * (cardHeight + 8.f);

		const ImVec4 typeColor = GetNotificationColor(notification.m_Type);

		ImGui::SetNextWindowSize(ImVec2(cardWidth, cardHeight), ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(x + notification.m_AnimationOffset, y), ImGuiCond_Always);

		const std::string windowName = "##Notification_" + std::to_string(notification.m_Identifier);

		constexpr ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.f, 11.f));

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.045f, 0.045f, 0.050f, 0.97f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(typeColor.x, typeColor.y, typeColor.z, 0.35f));

		ImGui::Begin(windowName.c_str(), nullptr, flags);

		const ImVec2 windowPos = ImGui::GetWindowPos();
		const ImVec2 windowSize = ImGui::GetWindowSize();

		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now() - notification.m_CreatedOn).count();

		const float progress = notification.m_Duration > 0
			? std::clamp(1.f - static_cast<float>(elapsed) / static_cast<float>(notification.m_Duration), 0.f, 1.f)
			: 0.f;

		if (Menu::Font::g_AwesomeFont)
			ImGui::PushFont(Menu::Font::g_AwesomeFont);

		ImGui::PushStyleColor(ImGuiCol_Text, typeColor);
		ImGui::TextUnformatted(GetNotificationIcon(notification.m_Type));
		ImGui::PopStyleColor();

		if (Menu::Font::g_AwesomeFont)
			ImGui::PopFont();

		ImGui::SameLine(0.f, 9.f);

		if (Menu::Font::g_DefaultFont)
			ImGui::PushFont(Menu::Font::g_DefaultFont);

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.f));
		ImGui::TextUnformatted(notification.m_Title.c_str());
		ImGui::PopStyleColor();

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.73f, 1.f));
		ImGui::TextWrapped("%s", notification.m_Message.c_str());
		ImGui::PopStyleColor();

		if (Menu::Font::g_DefaultFont)
			ImGui::PopFont();

		if (notification.m_ContextFunc)
		{
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Text, typeColor);

			if (ImGui::Selectable(notification.m_ContextFuncName.c_str()))
			{
				auto function = notification.m_ContextFunc;

				FiberPool::Push([function] {
					function();
				});
			}

			ImGui::PopStyleColor();
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const float progressWidth = windowSize.x * progress;

		drawList->AddRectFilled(
			ImVec2(windowPos.x, windowPos.y + windowSize.y - 3.f),
			ImVec2(windowPos.x + progressWidth, windowPos.y + windowSize.y),
			ImGui::ColorConvertFloat4ToU32(typeColor),
			0.f
		);

		ImGui::End();

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(2);
	}

	void Notifications::DrawImpl()
	{
		std::scoped_lock lock(m_Mutex);

		int position = 0;

		for (auto it = m_Notifications.begin(); it != m_Notifications.end();)
		{
			auto& notification = it->second;

			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now() - notification.m_CreatedOn).count();

			if (!notification.m_Erasing && elapsed >= notification.m_Duration)
				notification.m_Erasing = true;

			if (notification.m_Erasing)
			{
				notification.m_AnimationOffset -= m_CardAnimationSpeed;

				if (notification.m_AnimationOffset <= -m_CardSizeX)
				{
					it = m_Notifications.erase(it);
					continue;
				}
			}
			else
			{
				notification.m_AnimationOffset = std::min(notification.m_AnimationOffset + m_CardAnimationSpeed, 0.f);
			}

			DrawNotification(notification, position++);

			++it;
		}
	}
}
