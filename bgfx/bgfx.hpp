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
        virtual void parseCustomConfig(YAML::Node& config) noexcept;

        virtual void setupWindowIntegration() noexcept;
        virtual void setupPostWindowCreation() noexcept;

        virtual void init(UImGui::RendererInternalMetadata& metadata) noexcept;
        virtual void renderStart(double deltaTime) noexcept;
        virtual void renderEnd(double deltaTime) noexcept;
        virtual void destroy() noexcept;

        virtual void ImGuiNewFrame() noexcept;
        virtual void ImGuiShutdown() noexcept;
        virtual void ImGuiInit() noexcept;
        virtual void ImGuiRenderData() noexcept;

        // Only called on Vulkan, because there we need to wait for resources to be used before freeing resources,
        // like textures
        virtual void waitOnGPU() noexcept;

        virtual ~BGFXRenderer() noexcept = default;
    private:
    };
}
