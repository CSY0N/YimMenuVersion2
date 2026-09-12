#include "D3D12Hook.hpp"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

namespace YimMenu
{
    thread_local bool g_InsideD3D12Hook = false;

    bool D3D12Hook::RawVtablePatch::Install(void** vtable, size_t index, void* replacement)
    {
        if (!vtable || !replacement) return false;

        if (Installed) return true;

        Slot = &vtable[index];
        OriginalFn = *Slot;

        if (!OriginalFn)
        {
            Slot = nullptr;
            return false;
        }

        DWORD oldProtect{};
        if (!VirtualProtect(Slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            Slot = nullptr;
            OriginalFn = nullptr;
            return false;
        }

        *Slot = replacement;

        DWORD ignored{};
        VirtualProtect(Slot, sizeof(void*), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), Slot, sizeof(void*));

        Installed = true;
        return true;
    }

    void D3D12Hook::RawVtablePatch::Uninstall()
    {
        if (!Installed || !Slot) return;

        DWORD oldProtect{};
        if (VirtualProtect(Slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            *Slot = OriginalFn;

            DWORD ignored{};
            VirtualProtect(Slot, sizeof(void*), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), Slot, sizeof(void*));
        }

        Slot = nullptr;
        OriginalFn = nullptr;
        Installed = false;
    }

    bool D3D12Hook::InitImpl()
    {
        std::scoped_lock lock{m_HookMutex};

        if (m_Hooked) return true;

        HMODULE d3d12Module = GetModuleHandleA("d3d12.dll");
        if (!d3d12Module) d3d12Module = LoadLibraryA("d3d12.dll");

        if (!d3d12Module)
        {
            LOGF(FATAL, "D3D12Hook: failed to load d3d12.dll.");
            return false;
        }

        HMODULE dxgiModule = GetModuleHandleA("dxgi.dll");
        if (!dxgiModule) dxgiModule = LoadLibraryA("dxgi.dll");

        if (!dxgiModule)
        {
            LOGF(FATAL, "D3D12Hook: failed to load dxgi.dll.");
            return false;
        }

        using D3D12CreateDeviceFn = HRESULT(WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
        using CreateDXGIFactory1Fn = HRESULT(WINAPI*)(REFIID, void**);

        auto createDevice = reinterpret_cast<D3D12CreateDeviceFn>(GetProcAddress(d3d12Module, "D3D12CreateDevice"));

        auto createFactory = reinterpret_cast<CreateDXGIFactory1Fn>(GetProcAddress(dxgiModule, "CreateDXGIFactory1"));

        if (!createDevice || !createFactory)
        {
            LOGF(FATAL, "D3D12Hook: failed to resolve D3D12/DXGI exports.");
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D12Device> dummyDevice{};
        if (FAILED(createDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&dummyDevice))))
        {
            LOGF(FATAL, "D3D12Hook: failed to create dummy device.");
            return false;
        }

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> dummyQueue{};
        if (FAILED(dummyDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&dummyQueue))))
        {
            LOGF(FATAL, "D3D12Hook: failed to create dummy command queue.");
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGIFactory4> factory{};
        if (FAILED(createFactory(IID_PPV_ARGS(&factory))))
        {
            LOGF(FATAL, "D3D12Hook: failed to create DXGI factory.");
            return false;
        }

        void** factoryVtable = *reinterpret_cast<void***>(factory.Get());
        if (!factoryVtable)
        {
            LOGF(FATAL, "D3D12Hook: factory vtable is null.");
            return false;
        }

        if (!m_CreateSwapChainForHwndPatch.Install(factoryVtable, 15, reinterpret_cast<void*>(&D3D12Hook::CreateSwapChainForHwnd)))
        {
            LOGF(FATAL, "D3D12Hook: failed to hook CreateSwapChainForHwnd.");
            return false;
        }

        constexpr const char* className = "YimMenuD3D12HookWindow";

        WNDCLASSEXA wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DefWindowProcA;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.lpszClassName = className;

        ATOM atom = RegisterClassExA(&wc);
        if (!atom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            m_CreateSwapChainForHwndPatch.Uninstall();
            LOGF(FATAL, "D3D12Hook: failed to register dummy window.");
            return false;
        }

        HWND hwnd = CreateWindowExA(0, className, "D3D12 Hook Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

        if (!hwnd)
        {
            m_CreateSwapChainForHwndPatch.Uninstall();
            UnregisterClassA(className, wc.hInstance);
            LOGF(FATAL, "D3D12Hook: failed to create dummy window.");
            return false;
        }

        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = 100;
        desc.Height = 100;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> dummySwapChain1{};

        g_InsideD3D12Hook = true;
        HRESULT swapResult = factory->CreateSwapChainForHwnd(dummyQueue.Get(), hwnd, &desc, nullptr, nullptr, &dummySwapChain1);
        g_InsideD3D12Hook = false;

        if (FAILED(swapResult) || !dummySwapChain1)
        {
            DestroyWindow(hwnd);
            UnregisterClassA(className, wc.hInstance);
            m_CreateSwapChainForHwndPatch.Uninstall();
            LOGF(FATAL, "D3D12Hook: failed to create dummy swapchain.");
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGISwapChain3> dummySwapChain3{};
        if (FAILED(dummySwapChain1.As(&dummySwapChain3)) || !dummySwapChain3)
        {
            DestroyWindow(hwnd);
            UnregisterClassA(className, wc.hInstance);
            m_CreateSwapChainForHwndPatch.Uninstall();
            LOGF(FATAL, "D3D12Hook: IDXGISwapChain3 unavailable.");
            return false;
        }

        void** swapVtable = *reinterpret_cast<void***>(dummySwapChain3.Get());
        if (!swapVtable)
        {
            DestroyWindow(hwnd);
            UnregisterClassA(className, wc.hInstance);
            m_CreateSwapChainForHwndPatch.Uninstall();
            return false;
        }

        if (!m_PresentPatch.Install(swapVtable, 8, reinterpret_cast<void*>(&D3D12Hook::Present)))
        {
            DestroyWindow(hwnd);
            UnregisterClassA(className, wc.hInstance);
            m_CreateSwapChainForHwndPatch.Uninstall();
            LOGF(FATAL, "D3D12Hook: failed to hook Present.");
            return false;
        }

        if (!m_ResizeBuffersPatch.Install(swapVtable, 13, reinterpret_cast<void*>(&D3D12Hook::ResizeBuffers)))
        {
            m_PresentPatch.Uninstall();
            m_CreateSwapChainForHwndPatch.Uninstall();
            DestroyWindow(hwnd);
            UnregisterClassA(className, wc.hInstance);
            LOGF(FATAL, "D3D12Hook: failed to hook ResizeBuffers.");
            return false;
        }

        dummySwapChain3.Reset();
        dummySwapChain1.Reset();

        DestroyWindow(hwnd);
        UnregisterClassA(className, wc.hInstance);

        m_Hooked = true;
        LOGF(INFO, "D3D12 Hook initialized.");
        return true;
    }

    bool D3D12Hook::DestroyImpl(bool unloading)
    {
        std::scoped_lock lock{m_HookMutex};

        (void)unloading;

        m_PresentPatch.Uninstall();
        m_ResizeBuffersPatch.Uninstall();
        m_CreateSwapChainForHwndPatch.Uninstall();

        m_Device = nullptr;
        m_CommandQueue = nullptr;
        m_SwapChain = nullptr;
        m_Window = nullptr;
        m_Hooked = false;

        return true;
    }

    HRESULT STDMETHODCALLTYPE D3D12Hook::Present(IDXGISwapChain3* swapChain, UINT syncInterval, UINT flags)
    {
        auto& self = GetInstance();

        using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain3*, UINT, UINT);
        auto original = self.m_PresentPatch.Original<PresentFn>();

        if (!original) return E_FAIL;

        if (g_InsideD3D12Hook) return original(swapChain, syncInterval, flags);

        {
            std::scoped_lock lock{self.m_HookMutex};

            self.m_SwapChain = swapChain;

            DXGI_SWAP_CHAIN_DESC desc{};
            if (SUCCEEDED(swapChain->GetDesc(&desc)))
                self.m_Window = desc.OutputWindow;

            Microsoft::WRL::ComPtr<ID3D12Device4> device{};
            if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&device))))
                self.m_Device = device.Get();
        }

        if (self.m_OnPresent) self.m_OnPresent();

        return original(swapChain, syncInterval, flags);
    }

    HRESULT STDMETHODCALLTYPE D3D12Hook::ResizeBuffers(IDXGISwapChain3* swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags)
    {
        auto& self = GetInstance();

        using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

        auto original = self.m_ResizeBuffersPatch.Original<ResizeBuffersFn>();
        if (!original) return E_FAIL;

        if (self.m_OnResizeBuffers) self.m_OnResizeBuffers();

        {
            std::scoped_lock lock{self.m_HookMutex};
            self.m_SwapChain = nullptr;
            self.m_Window = nullptr;
            self.m_Device = nullptr;
        }

        return original(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
    }

    HRESULT STDMETHODCALLTYPE D3D12Hook::CreateSwapChainForHwnd(IDXGIFactory2* factory, IUnknown* device, HWND hwnd, const DXGI_SWAP_CHAIN_DESC1* desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc, IDXGIOutput* restrictToOutput, IDXGISwapChain1** swapChain)
    {
        auto& self = GetInstance();

        using CreateSwapChainForHwndFn = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);

        auto original = self.m_CreateSwapChainForHwndPatch.Original<CreateSwapChainForHwndFn>();
        if (!original) return E_FAIL;

        if (g_InsideD3D12Hook) return original(factory, device, hwnd, desc, fullscreenDesc, restrictToOutput, swapChain);

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue{};
        if (device && SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&queue))) && queue)
        {
            const auto queueDesc = queue->GetDesc();

            if (queueDesc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
            {
                bool firstCapture = false;

                {
                    std::scoped_lock lock{self.m_HookMutex};
                    firstCapture = self.m_CommandQueue == nullptr;
                    self.m_CommandQueue = queue.Get();
                }

                if (firstCapture) LOGF(INFO, "D3D12Hook: command queue captured.");
            }
        }

        HRESULT result = original(factory, device, hwnd, desc, fullscreenDesc, restrictToOutput, swapChain);

        if (SUCCEEDED(result) && swapChain && *swapChain)
        {
            Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3{};

            if (SUCCEEDED((*swapChain)->QueryInterface(IID_PPV_ARGS(&swapChain3))))
            {
                std::scoped_lock lock{self.m_HookMutex};
                self.m_SwapChain = swapChain3.Get();
                self.m_Window = hwnd;
            }
        }

        return result;
    }
}
