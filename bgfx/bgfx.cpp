#include "bgfx.hpp"
#include "./imgui.h"
#include "bgfx/bgfx.h"

#ifndef __EMSCRIPTEN__
#ifdef __APPLE__
    #define GLFW_EXPOSE_NATIVE_COCOA
#elifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
#else
    #if __has_include(<wayland-client.h>)
        #define GLFW_EXPOSE_NATIVE_WAYLAND
    #endif
    #if __has_include(<X11/Xatom.h>)
        #define GLFW_EXPOSE_NATIVE_X11
    #endif
#endif
#include <GLFW/glfw3native.h>
#endif

static void* glfwNativeWindowHandle(GLFWwindow* _window)
{
#ifdef __APPLE__
    return glfwGetCocoaWindow(_window);
#elifdef _WIN32
    return glfwGetWin32Window(_window);
#elifdef __EMSCRIPTEN__
    return (void*)"#canvas";
#else
    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND)
    {
        return glfwGetWaylandWindow(_window);
    }

    return (void*)(uintptr_t)glfwGetX11Window(_window);
#endif
}

static bgfx::NativeWindowHandleType::Enum getNativeWindowHandleType()
{
#if !_WIN32 && !__APPLE__ && !__EMSCRIPTEN__
    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND)
    {
        return bgfx::NativeWindowHandleType::Wayland;
    }
#endif

    return bgfx::NativeWindowHandleType::Default;
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
    const auto windowSize = UImGui::Window::windowSize();

    bgfx::Init init{};
    init.type = bgfx::RendererType::Count;
    init.vendorId = 0;
    init.platformData.nwh = glfwNativeWindowHandle(UImGui::Window::getInternal());
    init.platformData.ndt = nullptr;
    init.platformData.type = getNativeWindowHandleType();
    init.resolution.width  = CAST(int, windowSize.x);
    init.resolution.height = CAST(int, windowSize.y);
    init.resolution.reset  = 0;

    bgfx::init(init);

    // Enable debug text.
    //bgfx::setDebug(true);
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
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, IM_COL32(col.x, col.y, col.z, col.w), 1.0f, 0);
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
    ImGui_ImplGlfw_InitForOther(UImGui::Window::getInternal(), true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(UImGui::Window::getInternal(), "canvas");
#endif
    const auto rendererData = UImGui::Renderer::data();
    ImGui_Implbgfx_Init(0, rendererData.msaaSamples, rendererData.bUsingVSync);
}

void UImGuiRendererExamples::BGFXRenderer::ImGuiRenderData() noexcept
{
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
}

void UImGuiRendererExamples::BGFXRenderer::waitOnGPU() noexcept
{

}
