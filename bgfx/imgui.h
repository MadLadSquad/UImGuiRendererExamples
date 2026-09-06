// Derived from this Gist by Richard Gale:
//     https://gist.github.com/RichardGale/6e2b74bc42b3005e08397236e4be0fd0

// ImGui BGFX binding

// You can copy and use unmodified imgui_impl_* files in your project. See
// main.cpp for an example of using this. If you use this binding you'll need to
// call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(),
// ImGui::Render() and ImGui_ImplXXXX_Shutdown(). If you are new to ImGui, see
// examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#pragma once
#include <cstdint>

void ImGui_Implbgfx_Init(int view, int msaaSamples, bool bUsingVSync) noexcept;
void ImGui_Implbgfx_Shutdown() noexcept;
void ImGui_Implbgfx_NewFrame() noexcept;
void ImGui_Implbgfx_RenderDrawLists(struct ImDrawData* draw_data) noexcept;

// bgfx splits what used to be one flag word in two: BGFX_RESET_* are device and frame globals, while
// BGFX_SWAP_CHAIN_* describe a single surface and live on bgfx::SwapChain::flags. Passing a per-surface
// flag to bgfx::reset is not an error but is silently dropped, with only a BX_WARN to show for it, so the
// two sets are built separately here and must be passed to the matching place.
//
// The Make* forms take their inputs explicitly because the renderer needs both flag words for bgfx::init,
// which happens before the imgui backend exists; the no-argument forms read what ImGui_Implbgfx_Init was
// given and are only valid once the backend is up.
[[nodiscard]] uint32_t ImGui_Implbgfx_MakeResetFlags(bool bUsingVSync) noexcept;
[[nodiscard]] uint32_t ImGui_Implbgfx_MakeSwapChainFlags(int msaaSamples) noexcept;

[[nodiscard]] uint32_t ImGui_Implbgfx_GetResetFlags() noexcept;
[[nodiscard]] uint32_t ImGui_Implbgfx_GetSwapChainFlags() noexcept;

// Use if you want to reset your rendering device without losing ImGui state.
void ImGui_Implbgfx_InvalidateDeviceObjects() noexcept;
bool ImGui_Implbgfx_CreateDeviceObjects() noexcept;