#include "bgfx.hpp"
#include "./imgui.h"
#include "bgfx/bgfx.h"
#include <cstdio>

UImGuiRendererExamples::BGFXRenderer::BGFXRenderer() noexcept
{
    type = UIMGUI_RENDERER_API_TYPE_HINT_OTHER;
}

void UImGuiRendererExamples::BGFXRenderer::parseCustomConfig(const ryml::ConstNodeRef&) noexcept{}
void UImGuiRendererExamples::BGFXRenderer::saveCustomConfig(ryml::NodeRef&) noexcept{}

void UImGuiRendererExamples::BGFXRenderer::setupWindowIntegration() noexcept
{
    UImGui::RendererUtils::setupManually();
}

void UImGuiRendererExamples::BGFXRenderer::setupPostWindowCreation() noexcept
{
    const auto windowSize = UImGui::Window::getWindowSize();
    const auto rendererData = UImGui::Renderer::data();

    bgfx::Init init{};
    init.type = bgfx::RendererType::Count;
    init.vendorId = 0;
    init.platformData.type = UImGui::Window::Platform::getCurrentWindowPlatform() == UIMGUI_WINDOW_PLATFORM_WAYLAND
                                                                                                                    ? bgfx::NativeWindowHandleType::Wayland
                                                                                                                    : bgfx::NativeWindowHandleType::Default;

    // The window this surface draws to, and everything that is a property of that surface, now belongs to
    // the swap chain rather than to PlatformData and the old Init::resolution. PlatformData keeps only what
    // is genuinely device-wide: an externally created context/queue and the handle type above.
    init.swapChain.nwh    = UImGui::Window::Platform::getNativeWindowHandle();
    init.swapChain.ndt    = UImGui::Window::Platform::getNativeDisplay();
    init.swapChain.width  = CAST(uint32_t, windowSize.x);
    init.swapChain.height = CAST(uint32_t, windowSize.y);
    init.swapChain.flags  = ImGui_Implbgfx_MakeSwapChainFlags(CAST(int, rendererData.msaaSamples));

    // Device and frame globals. Configured here rather than left at zero and applied on the first resize,
    // so that v-sync is honoured from the very first frame.
    init.reset = ImGui_Implbgfx_MakeResetFlags(rendererData.bUsingVSync);

    // Fix broken window transparency on X11 and Wayland
    if (UImGui::Window::Platform::getCurrentWindowPlatform() == UIMGUI_WINDOW_PLATFORM_WAYLAND || UImGui::Window::Platform::getCurrentWindowPlatform() == UIMGUI_WINDOW_PLATFORM_X11)
        UImGui::Window::setWindowSurfaceTransparent(false);

    bgfx::init(init);
}

void UImGuiRendererExamples::BGFXRenderer::init(UImGui::RendererInternalMetadata& metadata) noexcept
{
    // Fill what bgfx is able to tell us. bgfx deliberately abstracts the device away and exposes no GPU
    // name or driver version string at all - only the PCI ids and the name of the backend it picked - so
    // gpuName reports the device id and driverVersion is left empty rather than inventing a value.
    const auto* caps = bgfx::getCaps();
    metadata.apiVersion = bgfx::getRendererName(bgfx::getRendererType());

    if (caps != nullptr)
    {
        switch (caps->vendorId)
        {
        case BGFX_PCI_ID_AMD:                   metadata.vendorString = "AMD"; break;
        case BGFX_PCI_ID_APPLE:                 metadata.vendorString = "Apple"; break;
        case BGFX_PCI_ID_INTEL:                 metadata.vendorString = "Intel"; break;
        case BGFX_PCI_ID_NVIDIA:                metadata.vendorString = "NVIDIA"; break;
        case BGFX_PCI_ID_MICROSOFT:             metadata.vendorString = "Microsoft"; break;
        case BGFX_PCI_ID_ARM:                   metadata.vendorString = "ARM"; break;
        case BGFX_PCI_ID_SOFTWARE_RASTERIZER:   metadata.vendorString = "Software rasterizer"; break;
        default:                                metadata.vendorString = "Unknown"; break;
        }

        char deviceId[16]{};
        snprintf(deviceId, sizeof(deviceId), "0x%04x", caps->deviceId);
        metadata.gpuName = UImGui::FString("Device ") + deviceId;
    }

    UImGui::Window::pushWindowResizeCallback([](const int w, const int h) -> void
    {
        // Resizing the main window is now a swap-chain description rather than a pair of arguments. Only
        // the fields below are given a value; every other one stays neutral and keeps whatever bgfx::init
        // established, and nwh/ndt are ignored outright for the main window since bgfx owns them.
        bgfx::SwapChain swapChain{};
        swapChain.width  = CAST(uint32_t, w);
        swapChain.height = CAST(uint32_t, h);
        swapChain.flags  = ImGui_Implbgfx_GetSwapChainFlags();

        bgfx::reset(ImGui_Implbgfx_GetResetFlags(), &swapChain);
        bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);
    });
}

void UImGuiRendererExamples::BGFXRenderer::renderStart(const double) noexcept
{
    const auto col = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, ImGui::ColorConvertFloat4ToU32(col), 1.0f, 0);
}

void UImGuiRendererExamples::BGFXRenderer::renderEnd(double) noexcept
{
    bgfx::frame();
}

void UImGuiRendererExamples::BGFXRenderer::destroy() noexcept
{
    bgfx::shutdown();
}

void UImGuiRendererExamples::BGFXRenderer::ImGuiNewFrame() noexcept
{
    ImGui_Implbgfx_NewFrame();
    UImGui::RendererUtils::beginImGuiFrame();
}

void UImGuiRendererExamples::BGFXRenderer::ImGuiShutdown() noexcept
{
    ImGui_Implbgfx_Shutdown();
}

void UImGuiRendererExamples::BGFXRenderer::ImGuiInit() noexcept
{
    UImGui::RendererUtils::ImGuiInitOther();
    UImGui::RendererUtils::ImGuiInstallCallbacks();
    const auto rendererData = UImGui::Renderer::data();
    ImGui_Implbgfx_Init(0, static_cast<int>(rendererData.msaaSamples), rendererData.bUsingVSync);
}

void UImGuiRendererExamples::BGFXRenderer::ImGuiRenderData() noexcept
{
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
}

void UImGuiRendererExamples::BGFXRenderer::waitOnGPU() noexcept
{

}
