#include "metal_texture.hpp"
#include "metal.hpp"
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

struct MetalTextureData
{
    id<MTLDevice>* device;
    id<MTLTexture> texture;
};

void UImGuiRendererExamples::MetalTexture::init(UImGui::TextureData& dt, UImGui::String location, bool bFiltered) noexcept
{
    defaultInit(dt, location, bFiltered);
    dt.context = new MetalTextureData{
        reinterpret_cast<id<MTLDevice>*>(
            reinterpret_cast<MetalRenderer*>(
                UImGui::RendererUtils::getRenderer()
            )->getDevice()
        )
    };
}

void UImGuiRendererExamples::MetalTexture::load(UImGui::TextureData& dt, void* data, UImGui::FVector2 size, uint32_t depth, bool bFreeImageData, const TFunction<void(void*)>& freeFunc) noexcept
{
    beginLoad(dt, &data, size);
    MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: MTLPixelFormatRGBA8Unorm
                                                                                    width: CAST(NSInteger, size.x)
                                                                                   height: CAST(NSInteger, size.y)
                                                                                   mipmapped: NO];
    desc.usage       = MTLTextureUsageShaderRead;
    desc.storageMode = MTLStorageModeManaged;

    auto* texData = static_cast<MetalTextureData*>(dt.context);
    texData->texture = [*texData->device newTextureWithDescriptor:desc];

    MTLRegion region =
    {
        { 0,                        0,               0             },
        { CAST(NSUInteger, size.x), CAST(NSUInteger, size.y), 1    }
    };
    const NSUInteger bytesPerRow = CAST(NSUInteger, size.x) * 4;
    [texData->texture replaceRegion:region
                        mipmapLevel:0
                          withBytes:data
                        bytesPerRow:bytesPerRow];

    endLoad(dt, data, bFreeImageData, freeFunc);
}

uintptr_t UImGuiRendererExamples::MetalTexture::get(UImGui::TextureData& dt) noexcept
{
    return (__bridge ImTextureID)static_cast<MetalTextureData*>(dt.context)->texture;
}

void UImGuiRendererExamples::MetalTexture::clear(UImGui::TextureData& dt) noexcept
{
    delete static_cast<MetalTextureData*>(dt.context);
    defaultClear(dt);
}
