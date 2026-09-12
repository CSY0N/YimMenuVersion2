// Credits to:
// https://github.com/praydog/REFramework/blob/master/src/D3D12Hook.cpp
// https://github.com/praydog/REFramework/blob/master/src/D3D12Hook.hpp

#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include <functional>
#include <mutex>
#include <utility>

namespace YimMenu
{
    class D3D12Hook
    {
    public:
        static bool Init() { return GetInstance().InitImpl(); }
        static bool Destroy(bool unloading = false) { return GetInstance().DestroyImpl(unloading); }
        static void OnPresent(std::function<void()> fn) { GetInstance().m_OnPresent = std::move(fn); }
        static void OnResizeBuffers(std::function<void()> fn) { GetInstance().m_OnResizeBuffers = std::move(fn); }
        static void OnDeviceReset(std::function<void()> fn) { GetInstance().m_OnDeviceReset = std::move(fn); }
        static ID3D12Device4* GetDevice() { return GetInstance().m_Device; }
        static IDXGISwapChain3* GetSwapChain() { return GetInstance().m_SwapChain; }
        static ID3D12CommandQueue* GetCommandQueue() { return GetInstance().m_CommandQueue; }
        static HWND GetWindow() { return GetInstance().m_Window; }

    private:
        struct RawVtablePatch
        {
            bool Install(void** vtable, size_t index, void* replacement);
            void Uninstall();

            template <typename T>
            T Original() const
            {
                return reinterpret_cast<T>(OriginalFn);
            }

            void** Slot = nullptr;
            void* OriginalFn = nullptr;
            bool Installed = false;
        };

        static D3D12Hook& GetInstance()
        {
            static D3D12Hook instance;
            return instance;
        }

        bool InitImpl();
        bool DestroyImpl(bool unloading);
        static HRESULT STDMETHODCALLTYPE Present(IDXGISwapChain3* swapChain, UINT syncInterval, UINT flags);
        static HRESULT STDMETHODCALLTYPE ResizeBuffers(IDXGISwapChain3* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);
        static HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(IDXGIFactory2* factory, IUnknown* device, HWND hwnd, const DXGI_SWAP_CHAIN_DESC1* desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc, IDXGIOutput* restrictToOutput, IDXGISwapChain1** swapChain);
        
        std::function<void()> m_OnPresent{};
        std::function<void()> m_OnResizeBuffers{};
        std::function<void()> m_OnDeviceReset{};
        ID3D12Device4* m_Device = nullptr;
        ID3D12CommandQueue* m_CommandQueue = nullptr;
        IDXGISwapChain3* m_SwapChain = nullptr;
        HWND m_Window = nullptr;
        RawVtablePatch m_PresentPatch{};
        RawVtablePatch m_ResizeBuffersPatch{};
        RawVtablePatch m_CreateSwapChainForHwndPatch{};
        bool m_Hooked = false;
        std::recursive_mutex m_HookMutex{};
    };
}
