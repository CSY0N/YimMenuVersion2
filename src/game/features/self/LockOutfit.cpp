#include "core/commands/LoopedCommand.hpp"
#include "core/util/Joaat.hpp"
#include "game/backend/NativeHooks.hpp"
#include "game/backend/Self.hpp"
#include "game/gta/invoker/Invoker.hpp"
#include "game/gta/Natives.hpp"
#include <array>

namespace YimMenu::Features
{
	static void PedComponentRestrictionHook(rage::scrNativeCallContext* ctx);
	class LockOutfit : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		struct Component
		{
			int m_Drawable{};
			int m_Texture{};
			int m_Palette{};
		};

		struct Prop
		{
			int m_Drawable{-1};
			int m_Texture{};
		};

		static constexpr int COMPONENT_COUNT = 12;
		static constexpr int PROP_COUNT = 8;

		std::array<Component, COMPONENT_COUNT> m_Components{};
		std::array<Prop, PROP_COUNT> m_Props{};

		Hash m_Model{};
		bool m_Captured{};

		static void InstallHook()
		{
			static const bool installed = [] { NativeHooks::AddHook(NativeHooks::ALL_SCRIPTS, NativeIndex::DOES_CURRENT_PED_COMPONENT_HAVE_RESTRICTION_TAG, &PedComponentRestrictionHook); return true; }();
			(void)installed;
		}

		void Capture()
		{
			auto ped = Self::GetPed();
			if (!ped)
				return;

			const int handle = ped.GetHandle();
			m_Model = ENTITY::GET_ENTITY_MODEL(handle);

			for (int i = 0; i < COMPONENT_COUNT; ++i)
				m_Components[i] = {PED::GET_PED_DRAWABLE_VARIATION(handle, i), PED::GET_PED_TEXTURE_VARIATION(handle, i), PED::GET_PED_PALETTE_VARIATION(handle, i)};

			for (int i = 0; i < PROP_COUNT; ++i)
			{
				const int drawable = PED::GET_PED_PROP_INDEX(handle, i, true);
				m_Props[i] = {drawable, drawable == -1 ? 0 : PED::GET_PED_PROP_TEXTURE_INDEX(handle, i)};
			}

			m_Captured = true;
		}

		void RestoreComponent(int handle, int index) const
		{
			const auto& component = m_Components[index];
			PED::SET_PED_COMPONENT_VARIATION(handle, index, component.m_Drawable, component.m_Texture, component.m_Palette);
		}

		void RestoreProp(int handle, int index) const
		{
			const auto& prop = m_Props[index];

			if (prop.m_Drawable == -1)
			{
				PED::CLEAR_PED_PROP(handle, index, 0);
				return;
			}

			PED::SET_PED_PROP_INDEX(handle, index, prop.m_Drawable, prop.m_Texture, true, true);
		}

		bool ComponentChanged(int handle, int index) const
		{
			const auto& component = m_Components[index];

			return PED::GET_PED_DRAWABLE_VARIATION(handle, index) != component.m_Drawable ||
			       PED::GET_PED_TEXTURE_VARIATION(handle, index) != component.m_Texture ||
			       PED::GET_PED_PALETTE_VARIATION(handle, index) != component.m_Palette;
		}

		bool PropChanged(int handle, int index) const
		{
			const auto& prop = m_Props[index];
			const int drawable = PED::GET_PED_PROP_INDEX(handle, index, true);

			if (drawable != prop.m_Drawable)
				return true;

			return drawable != -1 && PED::GET_PED_PROP_TEXTURE_INDEX(handle, index) != prop.m_Texture;
		}

		void RestoreOutfit(int handle)
		{
			for (int i = 0; i < COMPONENT_COUNT; ++i)
				if (ComponentChanged(handle, i))
					RestoreComponent(handle, i);

			for (int i = 0; i < PROP_COUNT; ++i)
				if (PropChanged(handle, i))
					RestoreProp(handle, i);
		}

		void OnEnable() override
		{
			InstallHook();
			m_Captured = false;
			Capture();
		}

		void OnTick() override
		{
			auto ped = Self::GetPed();
			if (!ped)
				return;

			const int handle = ped.GetHandle();

			if (!m_Captured || ENTITY::GET_ENTITY_MODEL(handle) != m_Model)
			{
				Capture();
				return;
			}

			RestoreOutfit(handle);
		}

		void OnDisable() override
		{
			m_Captured = false;
			m_Model = 0;
		}
	};

	static LockOutfit _LockOutfit{"lockoutfit", "Lock Outfit", "Locks your current outfit and allows wardrobe access while wearing a duffel bag"};
	static void PedComponentRestrictionHook(rage::scrNativeCallContext* ctx)
	{
		const int ped = ctx->GetArg<int>(0);
		const int component = ctx->GetArg<int>(1);
		const Hash tag = ctx->GetArg<Hash>(2);
		if (_LockOutfit.GetState() && ped == PLAYER::PLAYER_PED_ID() && component == 5 && tag == Joaat("DUFFEL_BAG"))
		{
			ctx->SetReturnValue<BOOL>(FALSE);
			return;
		}
		NativeInvoker::GetNativeHandler(NativeIndex::DOES_CURRENT_PED_COMPONENT_HAVE_RESTRICTION_TAG)(ctx);
	}
}
