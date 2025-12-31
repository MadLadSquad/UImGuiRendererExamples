#include "metal.hpp"

UImGuiRendererExamples::MetalRenderer::MetalRenderer() noexcept
{
    type = UIMGUI_RENDERER_API_TYPE_HINT_METAL;
}

#ifndef __APPLE__
void UImGuiRendererExamples::MetalRenderer::parseCustomConfig(const ryml::ConstNodeRef&) noexcept{}
void UImGuiRendererExamples::MetalRenderer::saveCustomConfig(ryml::NodeRef& config) noexcept{}

void UImGuiRendererExamples::MetalRenderer::setupWindowIntegration() noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::setupPostWindowCreation() noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::init(UImGui::RendererInternalMetadata& metadata) noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::renderStart(double deltaTime) noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::renderEnd(double deltaTime) noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::destroy() noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::ImGuiNewFrame() noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::ImGuiShutdown() noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::ImGuiInit() noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::ImGuiRenderData() noexcept
{
}

void UImGuiRendererExamples::MetalRenderer::waitOnGPU() noexcept
{
}

void* UImGuiRendererExamples::MetalRenderer::getDevice() noexcept
{
    return nullptr;
}
#endif
