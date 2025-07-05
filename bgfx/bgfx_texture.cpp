#include "bgfx_texture.hpp"
#include <bgfx/bgfx.h>

void UImGuiRendererExamples::BGFXTexture::init(UImGui::TextureData& dt, UImGui::String location, bool bFiltered) noexcept
{
    defaultInit(dt, location, bFiltered);
}

void UImGuiRendererExamples::BGFXTexture::load(UImGui::TextureData& dt, void* data, UImGui::FVector2 size, uint32_t depth, bool bFreeImageData, const TFunction<void(void*)>& freeFunc) noexcept
{
    beginLoad(dt, &data, size);
    const bgfx::Memory* mem = bgfx::copy(data, size.x * size.y * 4);
    dt.id = bgfx::createTexture2D(
        size.x,
        size.y,
        false,
        1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE            |
        BGFX_SAMPLER_U_CLAMP         |
        BGFX_SAMPLER_V_CLAMP         |
        BGFX_SAMPLER_MIN_ANISOTROPIC |
        BGFX_SAMPLER_MAG_ANISOTROPIC,
        mem
    ).idx;
    endLoad(dt, data, bFreeImageData, freeFunc);
}

uintptr_t UImGuiRendererExamples::BGFXTexture::get(UImGui::TextureData& dt) noexcept
{
    return dt.id;
}

void UImGuiRendererExamples::BGFXTexture::clear(UImGui::TextureData& dt) noexcept
{
    const bgfx::TextureHandle handle{ static_cast<uint16_t>(dt.id) };
    if (bgfx::isValid(handle))
        bgfx::destroy(handle);
    defaultClear(dt);
}
