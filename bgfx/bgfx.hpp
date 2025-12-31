#pragma once
#include <Framework/Framework.hpp>
#pragma once
#include <Framework/Framework.hpp>

namespace UImGuiRendererExamples
{
    class BGFXRenderer final : public UImGui::GenericRenderer
    {
    public:
        BGFXRenderer() noexcept;

        void parseCustomConfig(const ryml::ConstNodeRef&) noexcept override;
        void saveCustomConfig(ryml::NodeRef& config) noexcept override;

        void setupWindowIntegration() noexcept override;
        void setupPostWindowCreation() noexcept override;

        void init(UImGui::RendererInternalMetadata& metadata) noexcept override;
        void renderStart(double deltaTime) noexcept override;
        void renderEnd(double deltaTime) noexcept override;
        void destroy() noexcept override;

        void ImGuiNewFrame() noexcept override;
        void ImGuiShutdown() noexcept override;
        void ImGuiInit() noexcept override;
        void ImGuiRenderData() noexcept override;

        // Only called on Vulkan, because there we need to wait for resources to be used before freeing resources,
        // like textures
        void waitOnGPU() noexcept override;

        ~BGFXRenderer() noexcept override = default;
    private:
    };
}
