#include "CustomLabelText.hpp"

namespace YimMenu
{
	void CustomLabelText::InitImpl()
	{
		AddLabelImpl(0x1B2EA75B, "Grand Theft Auto V Story"); // Title Story Mode
		AddLabelImpl(0xABB00DEB, "Grand Theft Auto V");       // Title Pause Menu
		AddLabelImpl(Joaat("PM_ENTER_MP"), "Play Grand Theft Auto Online with YimMenu");
		AddLabelImpl(Joaat("PM_EXIT_GAME"), "Quit Grand Theft Auto V with YimMenu");
		AddLabelImpl(Joaat("PM_GO"), "Play with YimMenu");
		AddLabelImpl(Joaat("HUD_MPREENTER"), "Joining a new session with YimMenu");
		AddLabelImpl(Joaat("PM_FRIEND_FM"), "Closed Friend Session with YimMenu");
		AddLabelImpl(Joaat("PM_INF_EXIT"), "Quit to desktop with YimMenu");
		AddLabelImpl(Joaat("PM_CREWS"), "Join a Crew Only Session with YimMenu");
		AddLabelImpl(Joaat("PM_INF_PGOT"), "Join Online with YimMenu");
		AddLabelImpl(Joaat("PM_INVO_FM"), "Join an Invite Only Session with YimMenu");
		AddLabelImpl(Joaat("LOADING_SPLAYER_L"), "Loading Story Mode with YimMenu");
		AddLabelImpl(Joaat("PM_NCREW_FM"), "Join a Crew Session with YimMenu");
		AddLabelImpl(Joaat("PM_CREW_FM"), "Join a Closed Crew Session with YimMenu");
		AddLabelImpl(Joaat("PM_SOLO_FM"), "Join a Solo Session with YimMenu");
		AddLabelImpl(Joaat("PM_FRESES"), "Join a Friends Only Session with YimMenu");
		AddLabelImpl(Joaat("PM_CHOOSE_CHAR"), "Choose your personnage with YimMenu");
		AddLabelImpl(Joaat("PM_INF_PGOT"), "Join Online with YimMenu");
	}

	bool CustomLabelText::AddLabelImpl(joaat_t hash, const std::string_view text)
	{
		const auto size = text.size() + 1;
		auto buffer = std::make_unique<char[]>(size);
		memcpy(buffer.get(), text.data(), size);
		return m_LabelOverwrites.insert({hash, std::move(buffer)}).second;
	}

	bool CustomLabelText::AddLabelImpl(joaat_t hash, CustomLabelText_t&& cb)
	{
		return m_Labels.insert({hash, std::move(cb)}).second;
	}
}