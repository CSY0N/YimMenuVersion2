#include "core/commands/LoopedCommand.hpp"
#include "game/gta/Stats.hpp"
#include "core/backend/ScriptMgr.hpp"

namespace YimMenu::Features
{
	class NightclubLoop : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;
        bool running = false;
        bool enabled = false;
        virtual void OnTick() override
        {
            if (!running && enabled)
            {
                g_log.send("INFO", "Nightclub Loop Tick");
                running = true;
                Stats::SetInt("MPX_CLUB_POPULARITY", 1000);
                Stats::SetInt("MPX_CLUB_PAY_TIME_LEFT", 1);
                ScriptMgr::Yield(std::chrono::seconds(5));
                running = false;
            }
        }
		virtual void OnEnable() override
		{
            enabled = true;
		}

		virtual void OnDisable() override
		{
            enabled = false;
		}
	};

	static NightclubLoop _NightclubLoop{"nightclubloop", "Nightclub Loop", "Free money nightclub safe money loop"};
}
