#pragma once
#include <Framework/Framework.hpp>
#ifdef _WIN32
    #include <imgui.h>
    #include <d3d12.h>
    #include <dxgi1_4.h>
#endif

namespace UImGuiRendererExamples
{
    class DX12Renderer final : public UImGui::GenericRenderer
    {
    public:
        DX12Renderer() noexcept;

        void parseCustomConfig(const ryml::ConstNodeRef&) noexcept override;
        void saveCustomConfig(ryml::NodeRef& config) noexcept override;

        void setupWindowIntegration() noexcept override;
        void setupPostWindowCreation() noexcept override;

        void init(UImGui::RendererInternalMetadata& metadata) noexcept override;
        void renderStart(double deltaTime) noexcept override;
        void renderEnd(double deltaTime) noexcept override;
        void destroy() noexcept override;

        void ImGuiNewFrame() noexcept override;
        void ImGuiShutdown() noexcept override;
        void ImGuiInit() noexcept override;
        void ImGuiRenderData() noexcept override;

        // Only called on Vulkan, because there we need to wait for resources to be used before freeing resources,
        // like textures
        void waitOnGPU() noexcept override;

        ~DX12Renderer() noexcept override = default;
    private:
#ifdef _WIN32
        friend class DX12Texture;
    public:
        struct DX12DescriptorHeapAllocator
        {
            ID3D12DescriptorHeap* Heap = nullptr;
            D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
            D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
            D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
            UINT                        HeapHandleIncrement;
            ImVector<int>               FreeIndices;

            void Create(ID3D12Device** device, ID3D12DescriptorHeap* heap);
            void Destroy();
            void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle);
            void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle);
        };
        static constexpr int APP_NUM_BACK_BUFFERS = 2;
        static constexpr int APP_NUM_FRAMES_IN_FLIGHT = 2;
        static constexpr int APP_SRV_HEAP_SIZE = 64;
    private:
        struct FrameContext
        {
            ID3D12CommandAllocator* CommandAllocator;
            UINT64 FenceValue;
        };

        FrameContext frameContext[APP_NUM_FRAMES_IN_FLIGHT] = {};
        UINT frameIndex = 0;

        ID3D12DescriptorHeap* rtvDescHeap = nullptr;
        ID3D12DescriptorHeap* srvDescHeap = nullptr;

        ID3D12CommandQueue* commandQueue = nullptr;
        ID3D12GraphicsCommandList* commandList = nullptr;

        ID3D12Fence* fence = nullptr;
        HANDLE fenceEvent = nullptr;
        UINT64 fenceLastSignaledValue = 0;
        
        IDXGISwapChain3* swapchain = nullptr;
        HANDLE swapchainWaitable = nullptr;
        
        ID3D12Resource* mainRenderTargetResource[APP_NUM_BACK_BUFFERS] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE mainRenderTargetDescriptor[APP_NUM_BACK_BUFFERS] = {};

        DX12DescriptorHeapAllocator descriptorHeapAllocator;
        ID3D12Device* deviceHandle = nullptr;
        int sampleCount = 1;

        bool CreateDeviceD3D(HWND hWnd) noexcept;
        void CleanupDeviceD3D() noexcept;
        void CreateRenderTarget() noexcept;
        void CleanupRenderTarget() noexcept;
        void WaitForLastSubmittedFrame() noexcept;

        FrameContext* WaitForNextFrameResources() noexcept;
#endif
    };
}
