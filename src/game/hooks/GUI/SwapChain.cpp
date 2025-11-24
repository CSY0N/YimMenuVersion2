#include "core/hooking/DetourHook.hpp"
#include "core/renderer/Renderer.hpp"
#include "game/hooks/Hooks.hpp"

namespace YimMenu::Hooks
{
	HRESULT SwapChain::Present(IDXGISwapChain* that, UINT syncInterval, UINT flags)
	{
		if (g_Running && !Renderer::IsResizing())
		{
			Renderer::DX12OnPresent();
		}
		return BaseHook::Get<SwapChain::Present, DetourHook<decltype(&SwapChain::Present)>>()->Original()(that, syncInterval, flags);
	}

	HRESULT SwapChain::ResizeBuffers(IDXGISwapChain* that, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
	{
		if (g_Running)
		{
			Renderer::DX12PreResize();
			const auto result =
			    BaseHook::Get<SwapChain::ResizeBuffers, DetourHook<decltype(&SwapChain::ResizeBuffers)>>()->Original()(that, bufferCount, width, height, newFormat, swapChainFlags);
			Renderer::DX12PostResize();
			return result;
		}
		return BaseHook::Get<SwapChain::ResizeBuffers, DetourHook<decltype(&SwapChain::ResizeBuffers)>>()->Original()(that, bufferCount, width, height, newFormat, swapChainFlags);
	}
}