#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace YimMenu
{
	enum class NotificationType
	{
		Info,
		Success,
		Warning,
		Error
	};

	struct Notification
	{
		NotificationType m_Type = NotificationType::Info;

		std::string m_Title;
		std::string m_Message;

		std::chrono::time_point<std::chrono::system_clock> m_CreatedOn;

		int m_Duration = 5000;

		std::function<void()> m_ContextFunc;
		std::string m_ContextFuncName;

		float m_AnimationOffset = -380.f;
		bool m_Erasing = false;

		std::uint32_t m_Identifier = 0;
	};

	class Notifications
	{
	private:
		static constexpr float m_CardSizeX = 380.f;
		static constexpr float m_CardSizeY = 92.f;
		static constexpr float m_CardAnimationSpeed = 35.f;

		std::unordered_map<std::string, Notification> m_Notifications{};
		std::mutex m_Mutex;

		Notification ShowImpl(const std::string& title, const std::string& message, NotificationType type, int duration, std::function<void()> contextFunction, const std::string& contextFunctionName);

		void DrawImpl();
		bool EraseImpl(const Notification& notification);

		static Notifications& GetInstance()
		{
			static Notifications instance;
			return instance;
		}

	public:
		static Notification Show(const std::string& title, const std::string& message, NotificationType type = NotificationType::Info, int duration = 5000, std::function<void()> contextFunction = nullptr, const std::string& contextFunctionName = "")
		{
			return GetInstance().ShowImpl(title, message, type, duration, std::move(contextFunction), contextFunctionName);
		}

		static void Draw()
		{
			GetInstance().DrawImpl();
		}

		static bool Erase(const Notification& notification)
		{
			return GetInstance().EraseImpl(notification);
		}
	};
}
