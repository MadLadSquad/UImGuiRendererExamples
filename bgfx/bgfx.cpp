#include "bgfx.hpp"
#include "./imgui.h"
#include "bgfx/bgfx.h"

UImGuiRendererExamples::BGFXRenderer::BGFXRenderer() noexcept
{
    type = UIMGUI_RENDERER_API_TYPE_HINT_OTHER;
}

void UImGuiRendererExamples::BGFXRenderer::parseCustomConfig(YAML::Node& config) noexcept
{

}

void UImGuiRendererExamples::BGFXRenderer::setupWindowIntegration() noexcept
{
    UImGui::RendererUtils::setupManually();
}

void UImGuiRendererExamples::BGFXRenderer::setupPostWindowCreation() noexcept
{
    const auto windowSize = UImGui::Window::getWindowSize();

    bgfx::Init init{};
    init.type = bgfx::RendererType::Count;
    init.vendorId = 0;
    init.platformData.nwh = UImGui::Window::Platform::getNativeWindowHandle();
    init.platformData.ndt = UImGui::Window::Platform::getNativeDisplay();
    init.platformData.type = UImGui::Window::Platform::getCurrentWindowPlatform() == UIMGUI_WINDOW_PLATFORM_WAYLAND
                                                                                                                    ? bgfx::NativeWindowHandleType::Wayland
                                                                                                                    : bgfx::NativeWindowHandleType::Default;

    init.resolution.width  = CAST(int, windowSize.x);
    init.resolution.height = CAST(int, windowSize.y);
    init.resolution.reset  = 0;
    // Fix broken window transparency on X11 and Wayland
    if (UImGui::Window::Platform::getCurrentWindowPlatform() == UIMGUI_WINDOW_PLATFORM_WAYLAND || UImGui::Window::Platform::getCurrentWindowPlatform() == UIMGUI_WINDOW_PLATFORM_X11)
        UImGui::Window::setWindowSurfaceTransparent(false);

    bgfx::init(init);
}

void UImGuiRendererExamples::BGFXRenderer::init(UImGui::RendererInternalMetadata& metadata) noexcept
{
    UImGui::Window::pushWindowResizeCallback([](const int w, const int h) -> void
    {
        bgfx::reset(CAST(uint32_t, w), CAST(uint32_t, h), ImGui_Implbgfx_GetResetFlags());
        bgfx::setViewRect(0, 0, 0, bgfx::BackbufferRatio::Equal);
    });
}

void UImGuiRendererExamples::BGFXRenderer::renderStart(const double deltaTime) noexcept
{
    const auto col = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, ImGui::ColorConvertFloat4ToU32(col), 1.0f, 0);
}

void UImGuiRendererExamples::BGFXRenderer::renderEnd(double deltaTime) noexcept
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
