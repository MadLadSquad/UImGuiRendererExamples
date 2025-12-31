#include "metal.hpp"
#ifndef __EMSCRIPTEN__
#include <imgui.h>
#include <imgui_impl_metal.h>

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <AppKit/AppKit.h>

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
    uint32_t samples;
    id<MTLTexture> msaaColor;
};

#define DATA CAST(MetalRendererData*, data)

void UImGuiRendererExamples::MetalRenderer::parseCustomConfig(const ryml::ConstNodeRef&) noexcept{}
void UImGuiRendererExamples::MetalRenderer::saveCustomConfig(ryml::NodeRef& config) noexcept{}

void UImGuiRendererExamples::MetalRenderer::setupWindowIntegration() noexcept
{
    data = new MetalRendererData{};
    UImGui::RendererUtils::setupManually();
}

#define CHECK_SAMPLE_SUPPORT(x) [DATA->device supportsTextureSampleCount:x]

void UImGuiRendererExamples::MetalRenderer::setupPostWindowCreation() noexcept
{
    DATA->device = MTLCreateSystemDefaultDevice();
    DATA->commandQueue = [DATA->device newCommandQueue];

    DATA->samples = UImGui::Renderer::data().msaaSamples;
    if (DATA->samples >= 16 && CHECK_SAMPLE_SUPPORT(16))
        DATA->samples = 16;
    else if (DATA->samples >= 8 && CHECK_SAMPLE_SUPPORT(8))
        DATA->samples = 8;
    else if (DATA->samples >= 4 && CHECK_SAMPLE_SUPPORT(4))
        DATA->samples = 4;
    else if (DATA->samples >= 2 && CHECK_SAMPLE_SUPPORT(2))
        DATA->samples = 2;
    else
        DATA->samples = 1;
}

void UImGuiRendererExamples::MetalRenderer::init(UImGui::RendererInternalMetadata& metadata) noexcept
{
    metadata.vendorString = [DATA->device.name UTF8String];
    metadata.gpuName = [DATA->device.name UTF8String];

    Logger::log("Running on device - ", ULOG_LOG_TYPE_NOTE, metadata.gpuName);
}

static void rebuildMSAA(MetalRendererData* data, const int width, const int height)
{
    if (data->samples == 1)
      return;

    if (data->msaaColor && CAST(double, data->msaaColor.width) == width && CAST(double, data->msaaColor.height) == height)
        return;

    MTLTextureDescriptor* td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:data->layer.pixelFormat
                                                           width:width
                                                          height:height
                                                       mipmapped:NO];
    td.textureType = MTLTextureType2DMultisample;
    td.sampleCount = data->samples;
    td.storageMode = MTLStorageModePrivate;
    td.usage = MTLTextureUsageRenderTarget;

    data->msaaColor = [data->device newTextureWithDescriptor:td];
}

void UImGuiRendererExamples::MetalRenderer::renderStart(double deltaTime) noexcept
{
    pool = objc_autoreleasePoolPush();

    auto size = UImGui::Window::getWindowSize();
    int width = CAST(int, size.x);
    int height = CAST(int, size.y);

    DATA->layer.drawableSize = CGSizeMake(width, height);
    DATA->layer.displaySyncEnabled = UImGui::Renderer::data().bUsingVSync;
    DATA->drawable = [DATA->layer nextDrawable];

    DATA->commandBuffer = [DATA->commandQueue commandBuffer];

    rebuildMSAA(DATA, width, height);
    id<MTLTexture> colorTex = (DATA->samples > 1) ? DATA->msaaColor : DATA->drawable.texture;

    const auto& colours = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    DATA->renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(colours.x * colours.w, colours.y * colours.w, colours.z * colours.w, colours.w);
    DATA->renderPassDescriptor.colorAttachments[0].texture = colorTex;
    DATA->renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    DATA->renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    if (DATA->samples > 1)
    {
        DATA->renderPassDescriptor.colorAttachments[0].resolveTexture = DATA->drawable.texture;
        DATA->renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
    }

    DATA->renderEncoder = [DATA->commandBuffer renderCommandEncoderWithDescriptor:DATA->renderPassDescriptor];
    [DATA->renderEncoder pushDebugGroup:@"ImGui demo"];
}

void UImGuiRendererExamples::MetalRenderer::renderEnd(double deltaTime) noexcept
{
    [DATA->renderEncoder popDebugGroup];
    [DATA->renderEncoder endEncoding];

    if (UImGui::Renderer::data().bUsingVSync)
    {
        [DATA->commandBuffer presentDrawable:DATA->drawable];
        [DATA->commandBuffer commit];
    }
    else
    {
        [DATA->commandBuffer commit];
        [DATA->commandBuffer waitUntilScheduled];
        [DATA->drawable present];
    }

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
    UImGui::RendererUtils::ImGuiInitOther();
    ImGui_ImplMetal_Init(DATA->device);

    NSWindow* nswin = static_cast<NSWindow*>(UImGui::Window::Platform::getNativeWindowHandle());
    DATA->layer = [CAMetalLayer layer];
    DATA->layer.device = DATA->device;
    DATA->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    DATA->layer.displaySyncEnabled = UImGui::Renderer::data().bUsingVSync;
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

void* UImGuiRendererExamples::MetalRenderer::getDevice() noexcept
{
    return CAST(void*, &DATA->device);
}
#endif
