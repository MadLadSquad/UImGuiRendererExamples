// Derived from this Gist by Richard Gale:
//     https://gist.github.com/RichardGale/6e2b74bc42b3005e08397236e4be0fd0

// ImGui BFFX binding
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture
// identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See
// main.cpp for an example of using this. If you use this binding you'll need to
// call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(),
// ImGui::Render() and ImGui_ImplXXXX_Shutdown(). If you are new to ImGui, see
// examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "imgui.h"
#include <imgui.h>

#include "bgfx/bgfx.h"
#include "bgfx/embedded_shader.h"
#include "bx/math.h"
#include "bx/timer.h"

// Data
static uint8_t g_View = 255;
static bgfx::ProgramHandle g_ShaderHandle = BGFX_INVALID_HANDLE;
static bgfx::UniformHandle g_AttribLocationTex = BGFX_INVALID_HANDLE;
static bgfx::VertexLayout g_VertexLayout;

bool ImGui_Implbgfx_UpdateTexture(ImTextureData* tex);

void ImGui_Implbgfx_RenderDrawLists(ImDrawData* draw_data)
{
    const ImGuiIO& io = ImGui::GetIO();
    const int fb_width  = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    const int fb_height = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;

    if (draw_data->Textures)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_Implbgfx_UpdateTexture(tex);

    const bgfx::Caps* caps = bgfx::getCaps();

    const float L = draw_data->DisplayPos.x;
    const float T = draw_data->DisplayPos.y;
    const float R = L + draw_data->DisplaySize.x;
    const float B = T + draw_data->DisplaySize.y;

    float ortho[16];
    bx::mtxOrtho(ortho, L, R, B, T, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);

    bgfx::setViewTransform(g_View, nullptr, ortho);
    bgfx::setViewRect(g_View, 0, 0, static_cast<uint16_t>(fb_width), static_cast<uint16_t>(fb_height));

    constexpr uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,BGFX_STATE_BLEND_INV_SRC_ALPHA);

    const ImVec2 clip_off   = draw_data->DisplayPos;
    const ImVec2 clip_scale = draw_data->FramebufferScale;

    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        const auto vtx_count = static_cast<uint32_t>(cmd_list->VtxBuffer.size());
        const auto idx_count = static_cast<uint32_t>(cmd_list->IdxBuffer.size());

        // Allocate transient buffers (skip list if not enough space)
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer  tib;
        if ((vtx_count != bgfx::getAvailTransientVertexBuffer(vtx_count, g_VertexLayout)) || (idx_count != bgfx::getAvailTransientIndexBuffer(idx_count)))
            break;

        bgfx::allocTransientVertexBuffer(&tvb, vtx_count, g_VertexLayout);
        bgfx::allocTransientIndexBuffer(&tib, idx_count);

        // Copy ImGui vertices / indices verbatim
        memcpy(tvb.data, cmd_list->VtxBuffer.begin(), vtx_count * sizeof(ImDrawVert));
        memcpy(tib.data, cmd_list->IdxBuffer.begin(), idx_count * sizeof(ImDrawIdx));

        // Walk the draw-commands for this list
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
                continue;
            }

            ImVec2 clip_min(
                (pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y
            );
            ImVec2 clip_max(
                (pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y
            );

            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                continue;

            const auto sc_x = static_cast<uint16_t>(bx::max(clip_min.x, 0.0f));
            const auto sc_y = static_cast<uint16_t>(bx::max(clip_min.y, 0.0f));
            bgfx::setScissor(sc_x, sc_y,
                static_cast<uint16_t>(bx::min(clip_max.x, 65535.0f) - sc_x),
                static_cast<uint16_t>(bx::min(clip_max.y, 65535.0f) - sc_y)
            );

            // Bind state & resources and draw
            bgfx::setState(state);

            const bgfx::TextureHandle th = { static_cast<uint16_t>((static_cast<intptr_t>(pcmd->TexRef.GetTexID()) & 0xffff)) };
            bgfx::setTexture(0, g_AttribLocationTex, th);

            bgfx::setVertexBuffer(0, &tvb, 0, vtx_count);
            bgfx::setIndexBuffer (&tib, pcmd->IdxOffset, pcmd->ElemCount);
            bgfx::submit(g_View, g_ShaderHandle);
        }
    }
}

static void uploadRect(ImTextureData* texture, const int x, const int y, const int w, const int h)
{
    if (w <= 0 || h <= 0)
        return;

    const void* src = texture->GetPixelsAt(x, y);
    const bgfx::Memory* mem = bgfx::makeRef(src, texture->GetPitch() * h);

    bgfx::updateTexture2D(
        { static_cast<uint16_t>(texture->GetTexID()) },
        0,
        0,
        static_cast<uint16_t>(x),
        static_cast<uint16_t>(y),
        static_cast<uint16_t>(w),
        static_cast<uint16_t>(h),
        mem,
        static_cast<uint16_t>(texture->GetPitch())
    );
}

bool ImGui_Implbgfx_UpdateTexture(ImTextureData* tex)
{
    if (tex->Status == ImTextureStatus_WantCreate)
    {
        const auto texture = bgfx::createTexture2D(
            static_cast<uint16_t>(tex->Width),
            static_cast<uint16_t>(tex->Height),
            false,
            1,
            bgfx::TextureFormat::RGBA8,
            0,
            nullptr
        );

        bgfx::updateTexture2D(
            texture,
            0,
            0,
            0,
            0,
            static_cast<uint16_t>(tex->Width),
            static_cast<uint16_t>(tex->Height),
            bgfx::copy(tex->Pixels, tex->BytesPerPixel * tex->Width * tex->Height),
            tex->GetPitch()
        );

        tex->SetTexID(texture.idx);
        tex->SetStatus(ImTextureStatus_OK);
        return true;
    }
    if (tex->Status == ImTextureStatus_WantUpdates)
    {
        const bgfx::TextureHandle handle{ static_cast<uint16_t>(tex->GetTexID()) };
        if (!bgfx::isValid(handle))
            return false;

        uploadRect(tex, tex->UpdateRect.x, tex->UpdateRect.y, tex->UpdateRect.w, tex->UpdateRect.h);
        for (const auto& a : tex->Updates)
            uploadRect(tex, a.x, a.y, a.w, a.h);

        tex->SetStatus(ImTextureStatus_OK);
        return true;
    }
    if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames > 0)
    {
        const bgfx::TextureHandle h{ static_cast<uint16_t>(tex->GetTexID()) };
        if (bgfx::isValid(h))
            bgfx::destroy(h);
        tex->SetTexID(ImTextureID_Invalid);
        tex->SetStatus(ImTextureStatus_Destroyed);
        return true;
    }
    return false;
}

#include "fs_ocornut_imgui.bin.h"
#include "vs_ocornut_imgui.bin.h"

static const bgfx::EmbeddedShader s_embeddedShaders[] =
{
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER_END()
};

bool ImGui_Implbgfx_CreateDeviceObjects()
{
    const bgfx::RendererType::Enum type = bgfx::getRendererType();
    g_ShaderHandle = bgfx::createProgram(
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui"),
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui"),
        true
    );

    g_VertexLayout
        .begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    g_AttribLocationTex = bgfx::createUniform("g_AttribLocationTex", bgfx::UniformType::Sampler);

    return true;
}

void ImGui_Implbgfx_InvalidateDeviceObjects()
{
    for (const auto& a : ImGui::GetPlatformIO().Textures)
        bgfx::destroy(bgfx::TextureHandle{ .idx = static_cast<unsigned short>(a->GetTexID()) });
    bgfx::destroy(g_AttribLocationTex);
    bgfx::destroy(g_ShaderHandle);
}

void ImGui_Implbgfx_Init(const int view)
{
    g_View = static_cast<uint8_t>(view & 0xff);

    auto& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_bgfx";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
}

void ImGui_Implbgfx_Shutdown()
{
    ImGui_Implbgfx_InvalidateDeviceObjects();
}

void ImGui_Implbgfx_NewFrame()
{
    static bool bFirst = true;
    if (bFirst)
    {
        ImGui_Implbgfx_CreateDeviceObjects();
        bFirst = false;
    }
}