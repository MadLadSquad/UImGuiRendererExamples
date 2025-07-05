#include "metal_texture.hpp"
#ifndef __APPLE__
void UImGuiRendererExamples::MetalTexture::init(UImGui::TextureData& dt, UImGui::String location, bool bFiltered) noexcept
{
}

void UImGuiRendererExamples::MetalTexture::load(UImGui::TextureData& dt, void* data, UImGui::FVector2 size, uint32_t depth, bool bFreeImageData, const TFunction<void(void*)>& freeFunc) noexcept
{
}

uintptr_t UImGuiRendererExamples::MetalTexture::get(UImGui::TextureData& dt) noexcept
{
    return 0;
}

void UImGuiRendererExamples::MetalTexture::clear(UImGui::TextureData& dt) noexcept
{
}
#endif
