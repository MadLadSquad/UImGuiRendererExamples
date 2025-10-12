#pragma once
#include <Framework/Framework.hpp>

namespace UImGuiRendererExamples
{
    class BGFXTexture final : public UImGui::GenericTexture
    {
    public:
        BGFXTexture() noexcept = default;
        void init(UImGui::TextureData& dt, UImGui::String location, bool bFiltered) noexcept override;

        void load(UImGui::TextureData& dt, void* data, UImGui::FVector2 size, uint32_t depth, bool bFreeImageData, const TFunction<void(void*)>& freeFunc) noexcept override;

        // Event Safety - Post-begin
        uintptr_t get(UImGui::TextureData& dt) noexcept override;

        // Cleans up the image data
        // Event Safety - All initiated
        void clear(UImGui::TextureData& dt) noexcept override;
        ~BGFXTexture() noexcept override = default;
    };
}