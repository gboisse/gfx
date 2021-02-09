/****************************************************************************
MIT License

Copyright (c) 2021 Guillaume Boissé

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
****************************************************************************/
#pragma once

#include "gfx.h"
#include "imgui.h"

//!
//! ImGui initialization/termination.
//!

GfxResult gfxImGuiInitialize(GfxContext gfx);
GfxResult gfxImGuiTerminate();
GfxResult gfxImGuiRender();

bool gfxImGuiIsInitialized();

//!
//! Implementation details.
//!

#ifdef GFX_IMPLEMENTATION_DEFINE

#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_widgets.cpp"
#include "examples/imgui_impl_win32.cpp"

class GfxImGuiInternal
{
    GFX_NON_COPYABLE(GfxImGuiInternal);

    static uint32_t constexpr kConstant_Magic = 0x1E2D3C4Bu;

    uint32_t const magic_ = kConstant_Magic;

    GfxContext gfx_ = {};
    GfxTexture font_buffer_ = {};
    GfxSamplerState font_sampler_ = {};
    GfxBuffer index_buffers_[kGfxConstant_BackBufferCount] = {};
    GfxBuffer vertex_buffers_[kGfxConstant_BackBufferCount] = {};
    GfxProgram imgui_program_ = {};
    GfxKernel imgui_kernel_ = {};

public:
    GfxImGuiInternal() {}
    ~GfxImGuiInternal() { terminate(); }

    GfxResult initialize(GfxContext const &gfx)
    {
        if(!gfx)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot initialize ImGui using an invalid context object");
        gfx_ = gfx; // keep reference to context

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize.x = (float)gfxGetBackBufferWidth(gfx_);
        io.DisplaySize.y = (float)gfxGetBackBufferHeight(gfx_);
        io.UserData = this; // set magic number

        uint8_t *font_data;
        int32_t font_width, font_height;
        io.Fonts->GetTexDataAsRGBA32(&font_data, &font_width, &font_height);
        GfxBuffer font_buffer = gfxCreateBuffer(gfx_, font_width * font_height * 4, font_data, kGfxCpuAccess_Write);
        font_buffer_ = gfxCreateTexture2D(gfx_, font_width, font_height, DXGI_FORMAT_R8G8B8A8_UNORM);
        font_sampler_ = gfxCreateSamplerState(gfx_, D3D12_FILTER_MIN_MAG_MIP_POINT);
        if(!font_buffer || !font_buffer_ || !font_sampler_)
        {
            gfxDestroyBuffer(gfx_, font_buffer);
            return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to create ImGui font buffer");
        }
        font_buffer_.setName("gfx_ImGuiFontBuffer");
        io.Fonts->TexID = (ImTextureID)&font_buffer_;
        gfxCommandCopyBufferToTexture(gfx_, font_buffer_, font_buffer);
        GFX_TRY(gfxDestroyBuffer(gfx_, font_buffer));

        GfxDrawState imgui_draw_state;
        GfxProgramDesc imgui_program_desc = {};
        imgui_program_desc.vs =
            "float4x4 ProjectionMatrix;\r\n"
            "\r\n"
            "struct Vertex\r\n"
            "{\r\n"
            "    float2 pos : POSITION;\r\n"
            "    float2 uv  : TEXCOORD;\r\n"
            "    uint   col : COLOR;\r\n"
            "};\r\n"
            "\r\n"
            "struct Pixel\r\n"
            "{\r\n"
            "    float4 pos : SV_Position;\r\n"
            "    float2 uv  : TEXCOORD;\r\n"
            "    float4 col : COLOR;\r\n"
            "};\r\n"
            "\r\n"
            "Pixel main(in Vertex input)\r\n"
            "{\r\n"
            "    Pixel output;\r\n"
            "    const float4 col = float4(\r\n"
            "        ((input.col >> 0 ) & 0xFFu) / 255.0f,\r\n"
            "        ((input.col >> 8 ) & 0xFFu) / 255.0f,\r\n"
            "        ((input.col >> 16) & 0xFFu) / 255.0f,\r\n"
            "        ((input.col >> 24) & 0xFFu) / 255.0f);\r\n"
            "    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f));\r\n"
            "    output.uv  = input.uv;\r\n"
            "    output.col = col;\r\n"
            "    return output;\r\n"
            "}\r\n";
        imgui_program_desc.ps =
            "Texture2D FontBuffer;\r\n"
            "SamplerState FontSampler;\r\n"
            "\r\n"
            "struct Pixel\r\n"
            "{\r\n"
            "    float4 pos : SV_Position;\r\n"
            "    float2 uv  : TEXCOORD;\r\n"
            "    float4 col : COLOR;\r\n"
            "};\r\n"
            "\r\n"
            "float4 main(in Pixel input) : SV_Target\r\n"
            "{\r\n"
            "    return input.col * FontBuffer.SampleLevel(FontSampler, input.uv, 0.0f);\r\n"
            "}\r\n";
        imgui_program_ = gfxCreateProgram(gfx_, imgui_program_desc, "gfx_ImGuiProgram");
        GFX_TRY(gfxDrawStateEnableAlphaBlending(imgui_draw_state)); // enable alpha blending
        GFX_TRY(gfxDrawStateSetCullMode(imgui_draw_state, D3D12_CULL_MODE_NONE));
        imgui_kernel_ = gfxCreateGraphicsKernel(gfx_, imgui_program_, imgui_draw_state);
        if(!imgui_program_ || !imgui_kernel_)
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create program to draw ImGui");

        io.KeyMap[ImGuiKey_Tab]        = VK_TAB;
        io.KeyMap[ImGuiKey_LeftArrow]  = VK_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow]    = VK_UP;
        io.KeyMap[ImGuiKey_DownArrow]  = VK_DOWN;
        io.KeyMap[ImGuiKey_PageUp]     = VK_PRIOR;
        io.KeyMap[ImGuiKey_PageDown]   = VK_NEXT;
        io.KeyMap[ImGuiKey_Home]       = VK_HOME;
        io.KeyMap[ImGuiKey_End]        = VK_END;
        io.KeyMap[ImGuiKey_Insert]     = VK_INSERT;
        io.KeyMap[ImGuiKey_Delete]     = VK_DELETE;
        io.KeyMap[ImGuiKey_Backspace]  = VK_BACK;
        io.KeyMap[ImGuiKey_Space]      = VK_SPACE;
        io.KeyMap[ImGuiKey_Enter]      = VK_RETURN;
        io.KeyMap[ImGuiKey_Escape]     = VK_ESCAPE;
        io.KeyMap[ImGuiKey_A]          = 0x41;  // https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
        io.KeyMap[ImGuiKey_C]          = 0x43;
        io.KeyMap[ImGuiKey_V]          = 0x56;
        io.KeyMap[ImGuiKey_X]          = 0x58;
        io.KeyMap[ImGuiKey_Y]          = 0x59;
        io.KeyMap[ImGuiKey_Z]          = 0x5A;

        GFX_TRY(gfxProgramSetParameter(gfx_, imgui_program_, "FontSampler", font_sampler_));
        ImGui::NewFrame();  // can now start submitting ImGui commands

        return kGfxResult_NoError;
    }

    GfxResult terminate()
    {
        ImGui::DestroyContext();
        if(gfx_)
        {
            GFX_TRY(gfxDestroyTexture(gfx_, font_buffer_));
            GFX_TRY(gfxDestroySamplerState(gfx_, font_sampler_));
            for(uint32_t i = 0; i < ARRAYSIZE(index_buffers_); ++i)
                GFX_TRY(gfxDestroyBuffer(gfx_, index_buffers_[i]));
            for(uint32_t i = 0; i < ARRAYSIZE(vertex_buffers_); ++i)
                GFX_TRY(gfxDestroyBuffer(gfx_, vertex_buffers_[i]));
            GFX_TRY(gfxDestroyProgram(gfx_, imgui_program_));
            GFX_TRY(gfxDestroyKernel(gfx_, imgui_kernel_));
        }
        return kGfxResult_NoError;
    }

    GfxResult render()
    {
        char buffer[256];
        ImGuiIO &io = ImGui::GetIO();
        float const mouse_wheel  = io.MouseWheel;
        float const mouse_wheelh = io.MouseWheelH;
        ImGui::Render();    // implicit ImGui::EndFrame()
        ImDrawData const *draw_data = ImGui::GetDrawData();
        uint32_t const buffer_index = gfxGetBackBufferIndex(gfx_);

        if(draw_data->TotalVtxCount > 0)
        {
            GfxBuffer &index_buffer = index_buffers_[buffer_index];
            uint64_t const index_buffer_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
            if(index_buffer_size > index_buffer.getSize())
            {
                gfxDestroyBuffer(gfx_, index_buffer);   // release previous index memory
                index_buffer = gfxCreateBuffer(gfx_, GFX_ALIGN(index_buffer_size + ((index_buffer_size + 2) >> 1), 65536), nullptr, kGfxCpuAccess_Write);
                if(!index_buffer)
                    return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to allocate buffer of %d indices to draw ImGui", draw_data->TotalIdxCount);
                GFX_SNPRINTF(buffer, sizeof(buffer), "gfx_ImGuiIndexBuffer%u", buffer_index);
                index_buffer.setStride((uint32_t)sizeof(ImDrawIdx));
                index_buffer.setName(buffer);
            }
            ImDrawIdx *draw_idx = (ImDrawIdx *)gfxBufferGetData(gfx_, index_buffer);

            GfxBuffer &vertex_buffer = vertex_buffers_[buffer_index];
            uint64_t const vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
            if(vertex_buffer_size > vertex_buffer.getSize())
            {
                gfxDestroyBuffer(gfx_, vertex_buffer);  // release previous vertex memory
                vertex_buffer = gfxCreateBuffer(gfx_, GFX_ALIGN(vertex_buffer_size + ((vertex_buffer_size + 2) >> 1), 65536), nullptr, kGfxCpuAccess_Write);
                if(!vertex_buffer)
                    return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to allocate buffer of %d vertices to draw ImGui", draw_data->TotalVtxCount);
                GFX_SNPRINTF(buffer, sizeof(buffer), "gfx_ImGuiVertexBuffer%u", buffer_index);
                vertex_buffer.setStride((uint32_t)sizeof(ImDrawVert));
                vertex_buffer.setName(buffer);
            }
            ImDrawVert *draw_vtx = (ImDrawVert *)gfxBufferGetData(gfx_, vertex_buffer);

            for(int32_t i = 0; i < draw_data->CmdListsCount; ++i)
            {
                ImDrawList const *cmd_list = draw_data->CmdLists[i];
                memcpy(draw_idx, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                memcpy(draw_vtx, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
                draw_idx += cmd_list->IdxBuffer.Size;
                draw_vtx += cmd_list->VtxBuffer.Size;
            }

            float const L = 0.0f;
            float const R = io.DisplaySize.x;
            float const B = io.DisplaySize.y;
            float const T = 0.0f;
            float const projection_matrix[4][4] =
            {
                { 2.0f / (R - L),    0.0f,              0.0f, 0.0f },
                { 0.0f,              2.0f / (T - B),    0.0f, 0.0f },
                { 0.0f,              0.0f,              0.5f, 0.0f },
                { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f }
            };
            gfxProgramSetParameter(gfx_, imgui_program_, "ProjectionMatrix", projection_matrix);

            gfxCommandBindKernel(gfx_, imgui_kernel_);
            gfxCommandBindIndexBuffer(gfx_, index_buffer);
            gfxCommandBindVertexBuffer(gfx_, vertex_buffer);
            gfxCommandSetViewport(gfx_);    // draw to back buffer

            int32_t vtx_offset = 0;
            int32_t idx_offset = 0;
            for(int32_t i = 0; i < draw_data->CmdListsCount; ++i)
            {
                ImDrawList const *cmd_list = draw_data->CmdLists[i];
                for(int32_t j = 0; j < cmd_list->CmdBuffer.Size; ++j)
                {
                    ImDrawCmd const *cmd = &cmd_list->CmdBuffer[j];
                    if(cmd->UserCallback)
                        cmd->UserCallback(cmd_list, cmd);
                    else
                    {
                        GfxTexture const *font_buffer = (GfxTexture const *)cmd->TextureId;
                        if(font_buffer != nullptr)
                            gfxProgramSetParameter(gfx_, imgui_program_, "FontBuffer", *font_buffer);
                        gfxCommandSetScissorRect(gfx_, (int32_t)cmd->ClipRect.x,
                                                       (int32_t)cmd->ClipRect.y,
                                                       (int32_t)(cmd->ClipRect.z - cmd->ClipRect.x),
                                                       (int32_t)(cmd->ClipRect.w - cmd->ClipRect.y));
                        gfxCommandDrawIndexed(gfx_, cmd->ElemCount, 1, idx_offset, vtx_offset);
                    }
                    idx_offset += cmd->ElemCount;
                }
                vtx_offset += cmd_list->VtxBuffer.Size;
            }
            gfxCommandSetScissorRect(gfx_); // reset scissor test
        }

        io.MouseWheel  = mouse_wheel;
        io.MouseWheelH = mouse_wheelh;
        if(g_hWnd != 0) ImGui_ImplWin32_NewFrame();
        io.DisplaySize.x = (float)gfxGetBackBufferWidth(gfx_);
        io.DisplaySize.y = (float)gfxGetBackBufferHeight(gfx_);
        ImGui::NewFrame();  // can start recording new commands again
        io.MouseWheel = io.MouseWheelH = 0.0f;

        return kGfxResult_NoError;
    }

    static inline GfxImGuiInternal *GetGfxImGui() { if(ImGui::GetCurrentContext() == nullptr) return nullptr; GfxImGuiInternal *gfx_imgui = static_cast<GfxImGuiInternal *>(ImGui::GetIO().UserData); return (gfx_imgui != nullptr && gfx_imgui->magic_ == kConstant_Magic ? gfx_imgui : nullptr); }
};

GfxResult gfxImGuiInitialize(GfxContext gfx)
{
    GfxResult result;
    GfxImGuiInternal *gfx_imgui = new GfxImGuiInternal();
    if(!gfx_imgui) return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to initialize ImGui");
    result = gfx_imgui->initialize(gfx);
    if(result != kGfxResult_NoError)
    {
        delete gfx_imgui;
        return GFX_SET_ERROR(result, "Failed to initialize ImGui");
    }
    return kGfxResult_NoError;
}

GfxResult gfxImGuiTerminate()
{
    delete GfxImGuiInternal::GetGfxImGui();
    return kGfxResult_NoError;
}

GfxResult gfxImGuiRender()
{
    GfxImGuiInternal *gfx_imgui = GfxImGuiInternal::GetGfxImGui();
    if(!gfx_imgui) return kGfxResult_NoError;   // nothing to render
    return gfx_imgui->render();
}

bool gfxImGuiIsInitialized()
{
    GfxImGuiInternal *gfx_imgui = GfxImGuiInternal::GetGfxImGui();
    return (gfx_imgui != nullptr ? true : false);
}

#endif //! GFX_IMPLEMENTATION_DEFINE
