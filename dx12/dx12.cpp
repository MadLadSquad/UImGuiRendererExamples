#include "dx12.hpp"
#ifdef _WIN32
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>

#define _DEBUG
#ifdef _DEBUG
	#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
	#include <dxgidebug.h>
	#pragma comment(lib, "dxguid.lib")
#endif

void UImGuiRendererExamples::DX12Renderer::parseCustomConfig(const ryml::ConstNodeRef&) noexcept{}
void UImGuiRendererExamples::DX12Renderer::saveCustomConfig(ryml::NodeRef& config) noexcept{}

void UImGuiRendererExamples::DX12Renderer::setupWindowIntegration() noexcept
{
    UImGui::RendererUtils::setupManually();
}

void UImGuiRendererExamples::DX12Renderer::setupPostWindowCreation() noexcept
{
    if (!CreateDeviceD3D(static_cast<HWND>(UImGui::Window::Platform::getNativeWindowHandle())))
        CleanupDeviceD3D();
}

#define GET_RENDERER_LAMBDA reinterpret_cast<UImGuiRendererExamples::DX12Renderer*>(UImGui::RendererUtils::getRenderer())
#define MSAA_SUPPORTED()

void UImGuiRendererExamples::DX12Renderer::init(UImGui::RendererInternalMetadata& metadata) noexcept
{
    sampleCount = UImGui::Renderer::data().msaaSamples;
    if (sampleCount >= 16)
        sampleCount = 16;
    else if (sampleCount >= 8)
        sampleCount = 8;
    else if (sampleCount >= 4)
        sampleCount = 4;
    else if (sampleCount >= 2)
        sampleCount = 2;
    else
        sampleCount = 1;

    UImGui::Window::pushWindowResizeCallback([](int x, int y) -> void {
        auto* renderer = GET_RENDERER_LAMBDA;
        if (renderer->deviceHandle != nullptr && !UImGui::Window::getWindowIconified())
        {
            renderer->WaitForLastSubmittedFrame();
            renderer->CleanupRenderTarget();
            HRESULT result = renderer->swapchain->ResizeBuffers(0, (UINT)LOWORD(x), (UINT)HIWORD(y), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
            assert(SUCCEEDED(result) && "Failed to resize swapchain.");
            renderer->CreateRenderTarget();
        }
    });
}

void UImGuiRendererExamples::DX12Renderer::renderStart(double deltaTime) noexcept
{
}

void UImGuiRendererExamples::DX12Renderer::renderEnd(double deltaTime) noexcept
{
    FrameContext* frameCtx = WaitForNextFrameResources();
    UINT backBufferIdx = swapchain->GetCurrentBackBufferIndex();
    frameCtx->CommandAllocator->Reset();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = mainRenderTargetResource[backBufferIdx];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    commandList->Reset(frameCtx->CommandAllocator, nullptr);
    commandList->ResourceBarrier(1, &barrier);

    // Render Dear ImGui graphics
    const auto& clear = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    const float clear_color_with_alpha[4] = { clear.x * clear.w, clear.y * clear.w, clear.z * clear.w, clear.w };
    commandList->ClearRenderTargetView(mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
    commandList->OMSetRenderTargets(1, &mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
    commandList->SetDescriptorHeaps(1, &srvDescHeap);
    
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);
    commandList->Close();

    commandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&commandList);

    // Present
    HRESULT hr = swapchain->Present(UImGui::Renderer::data().bUsingVSync, 0);

    UINT64 fenceValue = fenceLastSignaledValue + 1;
    commandQueue->Signal(fence, fenceValue);
    fenceLastSignaledValue = fenceValue;
    frameCtx->FenceValue = fenceValue;
}

void UImGuiRendererExamples::DX12Renderer::destroy() noexcept
{
    CleanupDeviceD3D();
}

void UImGuiRendererExamples::DX12Renderer::ImGuiNewFrame() noexcept
{
    ImGui_ImplDX12_NewFrame();
    UImGui::RendererUtils::beginImGuiFrame();
}

void UImGuiRendererExamples::DX12Renderer::ImGuiShutdown() noexcept
{
    ImGui_ImplDX12_Shutdown();
}

void UImGuiRendererExamples::DX12Renderer::ImGuiInit() noexcept
{
    UImGui::RendererUtils::ImGuiInitOther();

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = deviceHandle;
    init_info.CommandQueue = commandQueue;
    init_info.NumFramesInFlight = APP_NUM_FRAMES_IN_FLIGHT;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
    // (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
    init_info.SrvDescriptorHeap = srvDescHeap;
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return GET_RENDERER_LAMBDA->descriptorHeapAllocator.Alloc(out_cpu_handle, out_gpu_handle); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return GET_RENDERER_LAMBDA->descriptorHeapAllocator.Free(cpu_handle, gpu_handle); };
    
    ImGui_ImplDX12_Init(&init_info);
}

void UImGuiRendererExamples::DX12Renderer::ImGuiRenderData() noexcept
{

}

void UImGuiRendererExamples::DX12Renderer::waitOnGPU() noexcept
{
    WaitForLastSubmittedFrame();
}

// Helper functions
bool UImGuiRendererExamples::DX12Renderer::CreateDeviceD3D(HWND hWnd) noexcept
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = UImGuiRendererExamples::DX12Renderer::APP_NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_2;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&deviceHandle)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        deviceHandle->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = UImGuiRendererExamples::DX12Renderer::APP_NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (deviceHandle->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = deviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < UImGuiRendererExamples::DX12Renderer::APP_NUM_BACK_BUFFERS; i++)
        {
            mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = APP_SRV_HEAP_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (deviceHandle->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvDescHeap)) != S_OK)
            return false;
        descriptorHeapAllocator.Create(&deviceHandle, srvDescHeap);
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (deviceHandle->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
        if (deviceHandle->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (deviceHandle->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&commandList)) != S_OK ||
        commandList->Close() != S_OK)
        return false;

    if (deviceHandle->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)) != S_OK)
        return false;

    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
        return false;

    {
        IDXGIFactory4* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;

        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;

        if (dxgiFactory->CreateSwapChainForHwnd(commandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
            return false;

        if (swapChain1->QueryInterface(IID_PPV_ARGS(&swapchain)) != S_OK)
            return false;

        swapChain1->Release();
        dxgiFactory->Release();

        swapchain->SetMaximumFrameLatency(UImGuiRendererExamples::DX12Renderer::APP_NUM_BACK_BUFFERS);
        swapchainWaitable = swapchain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void UImGuiRendererExamples::DX12Renderer::CleanupDeviceD3D() noexcept
{
    CleanupRenderTarget();
    if (swapchain)
    { 
        swapchain->SetFullscreenState(false, nullptr);
        swapchain->Release(); 
        swapchain = nullptr;
    }
    
    if (swapchainWaitable != nullptr)
        CloseHandle(swapchainWaitable);

    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
    {
        if (frameContext[i].CommandAllocator) 
        { 
            frameContext[i].CommandAllocator->Release(); 
            frameContext[i].CommandAllocator = nullptr; 
        }
    }
    
    if (commandQueue)
    { 
        commandQueue->Release();
        commandQueue = nullptr;
    }
    
    if (commandList)
    { 
        commandList->Release();
        commandList = nullptr;
    }

    if (rtvDescHeap)
    { 
        rtvDescHeap->Release();
        rtvDescHeap = nullptr;
    }
    
    if (srvDescHeap)
    { 
        srvDescHeap->Release();
        srvDescHeap = nullptr;
    }

    if (fence) 
    { 
        fence->Release(); 
        fence = nullptr; 
    }
    
    if (fenceEvent) 
    { 
        CloseHandle(fenceEvent); 
        fenceEvent = nullptr; 
    }
    
    if (deviceHandle) 
    { 
        deviceHandle->Release(); 
        deviceHandle = nullptr; 
    }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void UImGuiRendererExamples::DX12Renderer::CreateRenderTarget() noexcept
{
    for (UINT i = 0; i < UImGuiRendererExamples::DX12Renderer::APP_NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        swapchain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        deviceHandle->CreateRenderTargetView(pBackBuffer, nullptr, mainRenderTargetDescriptor[i]);
        mainRenderTargetResource[i] = pBackBuffer;
    }
}

void UImGuiRendererExamples::DX12Renderer::CleanupRenderTarget() noexcept
{
    WaitForLastSubmittedFrame();

    for (UINT i = 0; i < UImGuiRendererExamples::DX12Renderer::APP_NUM_BACK_BUFFERS; i++)
    {
        if (mainRenderTargetResource[i])
        { 
            mainRenderTargetResource[i]->Release();
            mainRenderTargetResource[i] = nullptr;
        }
    }
}

void UImGuiRendererExamples::DX12Renderer::WaitForLastSubmittedFrame() noexcept
{
    FrameContext* frameCtx = &frameContext[frameIndex % APP_NUM_FRAMES_IN_FLIGHT];

    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtx->FenceValue = 0;
    if (fence->GetCompletedValue() >= fenceValue)
        return;

    fence->SetEventOnCompletion(fenceValue, fenceEvent);
    WaitForSingleObject(fenceEvent, INFINITE);
}

UImGuiRendererExamples::DX12Renderer::FrameContext* UImGuiRendererExamples::DX12Renderer::WaitForNextFrameResources() noexcept
{
    UINT nextFrameIndex = frameIndex + 1;
    frameIndex = nextFrameIndex;

    HANDLE waitableObjects[] = { swapchainWaitable, nullptr };
    DWORD numWaitableObjects = 1;

    FrameContext* frameCtx = &frameContext[nextFrameIndex % APP_NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtx->FenceValue = 0;
        fence->SetEventOnCompletion(fenceValue, fenceEvent);
        waitableObjects[1] = fenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

    return frameCtx;
}

void UImGuiRendererExamples::DX12Renderer::DX12DescriptorHeapAllocator::Create(ID3D12Device** device, ID3D12DescriptorHeap* heap)
{
    IM_ASSERT(Heap == nullptr && FreeIndices.empty());
    Heap = heap;
    D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
    HeapType = desc.Type;
    HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
    HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
    HeapHandleIncrement = (*device)->GetDescriptorHandleIncrementSize(HeapType);
    FreeIndices.reserve((int)desc.NumDescriptors);
    for (int n = desc.NumDescriptors; n > 0; n--)
        FreeIndices.push_back(n - 1);
}

void UImGuiRendererExamples::DX12Renderer::DX12DescriptorHeapAllocator::Destroy()
{
    Heap = nullptr;
    FreeIndices.clear();
}

void UImGuiRendererExamples::DX12Renderer::DX12DescriptorHeapAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
    IM_ASSERT(FreeIndices.Size > 0);
    int idx = FreeIndices.back();
    FreeIndices.pop_back();
    out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
    out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
}

void UImGuiRendererExamples::DX12Renderer::DX12DescriptorHeapAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
{
    int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
    int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
    IM_ASSERT(cpu_idx == gpu_idx);
    FreeIndices.push_back(cpu_idx);
}

#else
void UImGuiRendererExamples::DX12Renderer::parseCustomConfig(const ryml::ConstNodeRef&) noexcept{}
void UImGuiRendererExamples::DX12Renderer::saveCustomConfig(ryml::NodeRef& config) noexcept{}
void UImGuiRendererExamples::DX12Renderer::setupWindowIntegration() noexcept {}
void UImGuiRendererExamples::DX12Renderer::setupPostWindowCreation() noexcept {}
void UImGuiRendererExamples::DX12Renderer::init(UImGui::RendererInternalMetadata& metadata) noexcept {}
void UImGuiRendererExamples::DX12Renderer::renderStart(double deltaTime) noexcept {}
void UImGuiRendererExamples::DX12Renderer::renderEnd(double deltaTime) noexcept {}
void UImGuiRendererExamples::DX12Renderer::destroy() noexcept {}
void UImGuiRendererExamples::DX12Renderer::ImGuiNewFrame() noexcept {}
void UImGuiRendererExamples::DX12Renderer::ImGuiShutdown() noexcept {}
void UImGuiRendererExamples::DX12Renderer::ImGuiInit() noexcept {}
void UImGuiRendererExamples::DX12Renderer::ImGuiRenderData() noexcept {}
void UImGuiRendererExamples::DX12Renderer::waitOnGPU() noexcept{}
#endif

UImGuiRendererExamples::DX12Renderer::DX12Renderer() noexcept
{
    type = UIMGUI_RENDERER_API_TYPE_HINT_D3D;
}
