#include "metal.hpp"
#ifndef __EMSCRIPTEN__
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_metal.h>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void* token);

struct MetalRendererData
{
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    MTLRenderPassDescriptor* renderPassDescriptor;
    CAMetalLayer* layer;
    id<MTLCommandBuffer> commandBuffer;
    id<CAMetalDrawable> drawable;
    id<MTLRenderCommandEncoder> renderEncoder;
};

#define DATA CAST(MetalRendererData*, data)

void UImGuiRendererExamples::MetalRenderer::parseCustomConfig(YAML::Node& config) noexcept
{

}

void UImGuiRendererExamples::MetalRenderer::setupWindowIntegration() noexcept
{
    data = new MetalRendererData{};
    UImGui::RendererUtils::setupManually();
}

void UImGuiRendererExamples::MetalRenderer::setupPostWindowCreation() noexcept
{
    DATA->device = MTLCreateSystemDefaultDevice();
    DATA->commandQueue = [DATA->device newCommandQueue];
}

void UImGuiRendererExamples::MetalRenderer::init(UImGui::RendererInternalMetadata& metadata) noexcept
{
    metadata.vendorString = [DATA->device.name UTF8String];
    metadata.gpuName = [DATA->device.name UTF8String];

    Logger::log("Running on device - ", ULOG_LOG_TYPE_NOTE, metadata.gpuName);
}

void UImGuiRendererExamples::MetalRenderer::renderStart(double deltaTime) noexcept
{
    pool = objc_autoreleasePoolPush();
    int width, height;
    glfwGetFramebufferSize(UImGui::Window::getInternal(), &width, &height);
    DATA->layer.drawableSize = CGSizeMake(width, height);
    DATA->drawable = [DATA->layer nextDrawable];

    DATA->commandBuffer = [DATA->commandQueue commandBuffer];

    const auto& colours = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    DATA->renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(colours.x * colours.w, colours.y * colours.w, colours.z * colours.w, colours.w);
    DATA->renderPassDescriptor.colorAttachments[0].texture = DATA->drawable.texture;
    DATA->renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    DATA->renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

    DATA->renderEncoder = [DATA->commandBuffer renderCommandEncoderWithDescriptor:DATA->renderPassDescriptor];
    [DATA->renderEncoder pushDebugGroup:@"ImGui demo"];
}

void UImGuiRendererExamples::MetalRenderer::renderEnd(double deltaTime) noexcept
{
    [DATA->renderEncoder popDebugGroup];
    [DATA->renderEncoder endEncoding];

    [DATA->commandBuffer presentDrawable:DATA->drawable];
    [DATA->commandBuffer commit];
    objc_autoreleasePoolPop(pool);
}

void UImGuiRendererExamples::MetalRenderer::destroy() noexcept
{
    delete DATA;
}

void UImGuiRendererExamples::MetalRenderer::ImGuiNewFrame() noexcept
{
    ImGui_ImplMetal_NewFrame(DATA->renderPassDescriptor);
    UImGui::RendererUtils::beginImGuiFrame();
}

void UImGuiRendererExamples::MetalRenderer::ImGuiShutdown() noexcept
{
    ImGui_ImplMetal_Shutdown();
}

void UImGuiRendererExamples::MetalRenderer::ImGuiInit() noexcept
{
    ImGui_ImplGlfw_InitForOther(UImGui::Window::getInternal(), true);
    ImGui_ImplMetal_Init(DATA->device);

    NSWindow *nswin = glfwGetCocoaWindow(UImGui::Window::getInternal());
    DATA->layer = [CAMetalLayer layer];
    DATA->layer.device = DATA->device;
    DATA->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    nswin.contentView.layer = DATA->layer;
    nswin.contentView.wantsLayer = YES;

    DATA->renderPassDescriptor = [MTLRenderPassDescriptor new];
}

void UImGuiRendererExamples::MetalRenderer::ImGuiRenderData() noexcept
{
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), DATA->commandBuffer, DATA->renderEncoder);
}

void UImGuiRendererExamples::MetalRenderer::waitOnGPU() noexcept
{

}
#endif