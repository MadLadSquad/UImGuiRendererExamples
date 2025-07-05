#pragma once
#include <Framework/Framework.hpp>

namespace UImGuiRendererExamples
{
    class MetalTexture final : public UImGui::GenericTexture
    {
    public:
        MetalTexture() noexcept = default;
        virtual void init(UImGui::TextureData& dt, UImGui::String location, bool bFiltered) noexcept override;

        virtual void load(UImGui::TextureData& dt, void* data, UImGui::FVector2 size, uint32_t depth, bool bFreeImageData,
                                const TFunction<void(void*)>& freeFunc) noexcept override;

        // Event Safety - Post-begin
        virtual uintptr_t get(UImGui::TextureData& dt) noexcept override;

        // Cleans up the image data
        // Event Safety - All initiated
        virtual void clear(UImGui::TextureData& dt) noexcept override;
        virtual ~MetalTexture() noexcept override = default;
    };
}