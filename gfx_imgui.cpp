/****************************************************************************
MIT License

Copyright (c) 2024 Guillaume Boissé

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

#include "gfx_imgui.h"
#include "gfx_internal_types.h"

#ifdef GFX_IMGUI_SOURCE
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 6011)  // Dereferencing NULL pointer
#    pragma warning(disable : 28182) // Dereferencing NULL pointer
#    pragma warning(disable : 6239)  // expression always evaluates to the result of <expression>
#endif
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"
#include "backends/imgui_impl_win32.cpp"
#ifdef _MSC_VER
#    pragma warning(pop)
#endif
#else
#include "imgui_impl_win32.h"
#endif

class GfxImGuiInternal
{
    GFX_NON_COPYABLE(GfxImGuiInternal);

    static uint32_t constexpr kConstant_Magic = 0x1E2D3C4Bu;
    static uint32_t constexpr kKernelRenderToBackBuffer = 0;
    static uint32_t constexpr kKernelRenderToTexture = 1;

    uint32_t const magic_ = kConstant_Magic;

    GfxContext gfx_ = {};
    GfxTexture font_buffer_ = {};
    GfxSamplerState font_sampler_ = {};
    GfxBuffer *index_buffers_ = nullptr;
    GfxBuffer *vertex_buffers_ = nullptr;
    GfxProgram imgui_program_ = {};
    GfxKernel imgui_kernels_[2] = {};
    DXGI_COLOR_SPACE_TYPE backbuffer_colour_space_;
    float reference_white_adjust_ = 1.0f;
    char **font_filenames_ = nullptr;
    uint32_t font_count_ = 0;
    ImFontConfig *font_configs_ = nullptr;
    float dpi_scale_ = 1.0f;
    bool dpi_scale_changed_  = false;

public:
    GfxImGuiInternal() {}
    ~GfxImGuiInternal() { terminate(); }

    GfxResult initializeKernels()
    {
        for (auto& kernel : imgui_kernels_)
            gfxDestroyKernel(gfx_, kernel);
        gfxDestroyProgram(gfx_, imgui_program_);
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
            "    output.col.xyz = select(col.xyz < 0.03929337067685376f, col.xyz / 12.92f, pow((col.xyz + 0.055010718947587f) / 1.055010718947587f, 2.4f));\r\n"
            "    return output;\r\n"
            "}\r\n";
        imgui_program_desc.ps =
            "Texture2D FontBuffer;\r\n"
            "SamplerState FontSampler;\r\n"
            "float ReferenceWhiteAdjust;\r\n"
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
            "    float4 col = input.col * FontBuffer.SampleLevel(FontSampler, input.uv, 0.0f);\r\n"
            "#ifdef OUTPUT_SRGB\r\n"
            "    col.xyz = select(col.xyz < 0.003041282560128f, 12.92f * col.xyz, 1.055010718947587f * pow(col.xyz, 1.0f / 2.4f) - 0.055010718947587f);\r\n"
            "#elif defined(OUTPUT_HDR10)\r\n"
            "    col.xyz *= ReferenceWhiteAdjust * (1.0f / 10000.0f);\r\n"
            "    const float3x3 mat = float3x3(0.6274178028f, 0.3292815089f, 0.04330066592f, 0.06909923255f, 0.919541657f, 0.01135913096f, 0.01639600657f, 0.08803547174f, 0.89556849f);\r\n"
            "    col.xyz = mul(mat, col.xyz);\r\n"
            "    float3 powM1 = pow(col.xyz, 0.1593017578125f);\r\n"
            "    col.xyz = pow((0.8359375f + 18.8515625f * powM1) / (1.0f + 18.6875f * powM1), 78.84375f);\r\n"
            "#else\r\n"
            "    col.xyz *= ReferenceWhiteAdjust * (1.0f / 80.0f);\r\n"
            "#endif\r\n"
            "    return col;\r\n"
            "}\r\n";
        imgui_program_ = gfxCreateProgram(gfx_, imgui_program_desc, "gfx_ImGuiProgram");
        GFX_TRY(gfxDrawStateEnableAlphaBlending(imgui_draw_state)); // enable alpha blending
        GFX_TRY(gfxDrawStateSetCullMode(imgui_draw_state, D3D12_CULL_MODE_NONE));

        // Create kKernelRenderToBackBuffer kernel
        {
            backbuffer_colour_space_ = gfxGetBackBufferColorSpace(gfx_);
            std::vector<char const *> defines;
            if (backbuffer_colour_space_ == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
            {
                defines.push_back("OUTPUT_HDR10");
            }
            else if (backbuffer_colour_space_ == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
            {
                defines.push_back("OUTPUT_SRGB");
            }
            imgui_kernels_[kKernelRenderToBackBuffer] = gfxCreateGraphicsKernel(
                gfx_, imgui_program_, imgui_draw_state, nullptr, defines.data(), (uint32_t)defines.size());
        }
        // Create kKernelRenderToTexture kernel
        {
            // Always output in sRGB when rendering to texture.
            char const *defines = "OUTPUT_SRGB";

            // Blend state for render to texture requires more precision on alpha than 2 bits. So we can't use
            // DXGI_FORMAT_R10G10B10A2_UNORM.
            GFX_TRY(
                gfxDrawStateSetBlendMode(imgui_draw_state, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA,
                    D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD));
            GFX_TRY(gfxDrawStateSetColorTarget(
                imgui_draw_state, 0, DXGI_FORMAT_R8G8B8A8_UNORM)); // see gfx_imgui.cpp
            imgui_kernels_[kKernelRenderToTexture] =
                gfxCreateGraphicsKernel(gfx_, imgui_program_, imgui_draw_state, nullptr, &defines, 1U);
        }

        if (!imgui_program_ || !imgui_kernels_[kKernelRenderToBackBuffer]
            || !imgui_kernels_[kKernelRenderToTexture])
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create program to draw ImGui");
        GfxDisplayDesc display_desc = gfxGetDisplayDescription(gfx_);
        reference_white_adjust_     = display_desc.reference_sdr_white_level;

        return kGfxResult_NoError;
    }

    GfxResult initializeScale()
    {
        gfxDestroyTexture(gfx_, font_buffer_);
        gfxDestroySamplerState(gfx_, font_sampler_);

        ImGui::StyleColorsDark();
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(dpi_scale_);

        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->Clear();
        for(uint32_t i = 0; i < font_count_; ++i)
        {
            if(i == 0 && font_configs_ != nullptr && font_configs_[0].MergeMode)
            {
                ImFontConfig defaultConfig = {};
                defaultConfig.SizePixels = 13.0f * dpi_scale_;
                io.Fonts->AddFontDefault(&defaultConfig);
            }
            if(font_configs_ != nullptr)
            {
                float const pre_dpi_size = font_configs_[i].SizePixels;
                ImVec2 const pre_dpi_glyph_offset = font_configs_[i].GlyphOffset;
                float const pre_dpi_glyph_extra_advance_x = font_configs_[i].GlyphExtraAdvanceX;
                float const pre_dpi_glyph_min_advance_x   = font_configs_[i].GlyphMinAdvanceX;
                float const pre_dpi_glyph_max_advance_x   = font_configs_[i].GlyphMaxAdvanceX;
                font_configs_[i].SizePixels *= dpi_scale_;
                font_configs_[i].GlyphOffset.x *= dpi_scale_;
                font_configs_[i].GlyphOffset.y *= dpi_scale_;
                font_configs_[i].GlyphExtraAdvanceX *= dpi_scale_;
                font_configs_[i].GlyphMinAdvanceX *= dpi_scale_;
                font_configs_[i].GlyphMaxAdvanceX *= dpi_scale_;
                float const font_size = (font_configs_[i].SizePixels > 0.0f) ? font_configs_[i].SizePixels : 16.0f * dpi_scale_;
                io.Fonts->AddFontFromFileTTF(font_filenames_[i], font_size, &font_configs_[i]);
                font_configs_[i].SizePixels = pre_dpi_size;
                font_configs_[i].GlyphOffset = pre_dpi_glyph_offset;
                font_configs_[i].GlyphExtraAdvanceX = pre_dpi_glyph_extra_advance_x;
                font_configs_[i].GlyphMinAdvanceX   = pre_dpi_glyph_min_advance_x;
                font_configs_[i].GlyphMaxAdvanceX   = pre_dpi_glyph_max_advance_x;
            }
            else
            {
                float const font_size = 16.0f * dpi_scale_;
                io.Fonts->AddFontFromFileTTF(font_filenames_[i], font_size, nullptr);
            }
        }
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

        return kGfxResult_NoError;
    }

    GfxResult initialize(GfxContext const &gfx, char const **font_filenames, uint32_t font_count, ImFontConfig const *font_configs, ImGuiConfigFlags flags)
    {
        if(!gfx)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot initialize ImGui using an invalid context object");
        gfx_ = gfx; // keep reference to context

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= flags | ImGuiConfigFlags_IsSRGB; // config flags
        io.DisplaySize.x = (float)gfxGetBackBufferWidth(gfx_);
        io.DisplaySize.y = (float)gfxGetBackBufferHeight(gfx_);
        io.UserData = this; // set magic number

        if(font_count > 0)
        {
            font_filenames_ = (char **)gfxMalloc(font_count * sizeof(char *));
            font_count_     = font_count;
            for(uint32_t i = 0; i < font_count; ++i)
            {
                font_filenames_[i] = (char *)gfxMalloc(strlen(font_filenames[i]) + 1);
                strcpy(font_filenames_[i], font_filenames[i]);
            }
            if(font_configs != nullptr)
            {
                font_configs_ = (ImFontConfig *)gfxMalloc(font_count * sizeof(ImFontConfig));
                for(uint32_t i = 0; i < font_count; ++i)
                {
                    font_configs_[i] = font_configs[i];
                }
            }
        }

        HWND window = gfxGetWindowHandle(gfx_);
        dpi_scale_ = 1.0f;
        if(window != nullptr)
        {
            UINT dpi = GetDpiForWindow(window);
            dpi_scale_ = (float)dpi / (float)USER_DEFAULT_SCREEN_DPI;
        }
        GFX_TRY(initializeScale());

        GFX_TRY(initializeKernels());

        index_buffers_ = (GfxBuffer *)gfxMalloc(gfxGetBackBufferCount(gfx_) * sizeof(GfxBuffer));
        vertex_buffers_ = (GfxBuffer *)gfxMalloc(gfxGetBackBufferCount(gfx_) * sizeof(GfxBuffer));
        for(uint32_t i = 0; i < gfxGetBackBufferCount(gfx_); ++i)
        {
            new(&index_buffers_[i]) GfxBuffer();
            new(&vertex_buffers_[i]) GfxBuffer();
        }
        GFX_TRY(gfxProgramSetParameter(gfx_, imgui_program_, "FontSampler", font_sampler_));
        ImGui::NewFrame();  // can now start submitting ImGui commands

        return kGfxResult_NoError;
    }

    GfxResult terminate()
    {
        if(ImGui::GetCurrentContext() != nullptr)
        {
            if(ImGui::GetIO().BackendPlatformUserData != nullptr)
                ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        if(font_count_ > 0)
        {
            for(uint32_t i = 0; i < font_count_; ++i)
            {
                gfxFree(font_filenames_[i]);
            }
            gfxFree(font_filenames_);
            font_filenames_ = nullptr;
            gfxFree(font_configs_);
            font_configs_ = nullptr;
            font_count_ = 0;
        }
        if(gfx_)
        {
            GFX_TRY(gfxDestroyTexture(gfx_, font_buffer_));
            GFX_TRY(gfxDestroySamplerState(gfx_, font_sampler_));
            if(index_buffers_ != nullptr)
                for(uint32_t i = 0; i < gfxGetBackBufferCount(gfx_); ++i)
                {
                    GFX_TRY(gfxDestroyBuffer(gfx_, index_buffers_[i]));
                    index_buffers_[i].~GfxBuffer();
                }
            if(vertex_buffers_ != nullptr)
                for(uint32_t i = 0; i < gfxGetBackBufferCount(gfx_); ++i)
                {
                    GFX_TRY(gfxDestroyBuffer(gfx_, vertex_buffers_[i]));
                    vertex_buffers_[i].~GfxBuffer();
                }
            GFX_TRY(gfxDestroyProgram(gfx_, imgui_program_));
            for (auto& kernel : imgui_kernels_)
                GFX_TRY(gfxDestroyKernel(gfx_, kernel));
            gfxFree(vertex_buffers_);
            gfxFree(index_buffers_);
        }
        return kGfxResult_NoError;
    }

    GfxResult render(GfxTexture optionalDrawToTexture)
    {
        char buffer[256];
        ImGuiIO &io = ImGui::GetIO();
        ImGui::Render();    // implicit ImGui::EndFrame()
        ImDrawData const *draw_data = ImGui::GetDrawData();
        uint32_t const buffer_index = gfxGetBackBufferIndex(gfx_);
        DXGI_COLOR_SPACE_TYPE colour_space = gfxGetBackBufferColorSpace(gfx_);
        if (colour_space != backbuffer_colour_space_)
        {
            initializeKernels();
        }

        if(dpi_scale_changed_)
        {
            initializeScale();
            dpi_scale_changed_ = false;
        }

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
            gfxProgramSetParameter(gfx_, imgui_program_, "ReferenceWhiteAdjust", reference_white_adjust_);

            if (optionalDrawToTexture)
            {
                gfxCommandBindKernel(gfx_, imgui_kernels_[kKernelRenderToTexture]);
                gfxTextureSetResourceState(gfx_, optionalDrawToTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
                gfxCommandBindColorTarget(gfx_, 0, optionalDrawToTexture);
            }
            else
            {
                gfxCommandBindKernel(gfx_, imgui_kernels_[kKernelRenderToBackBuffer]);
            }
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
                    else if(cmd->ClipRect.x != cmd->ClipRect.z &&
                            cmd->ClipRect.y != cmd->ClipRect.w)
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

        ImGui_ImplWin32_NewFrame();
        io.DisplaySize.x = (float)gfxGetBackBufferWidth(gfx_);
        io.DisplaySize.y = (float)gfxGetBackBufferHeight(gfx_);
        ImGui::NewFrame();  // can start recording new commands again

        return kGfxResult_NoError;
    }

    void setDPIScale(float scale)
    {
        dpi_scale_changed_ = true;
        dpi_scale_         = scale;
    }

    static inline GfxImGuiInternal *GetGfxImGui() { if(ImGui::GetCurrentContext() == nullptr) return nullptr; GfxImGuiInternal *gfx_imgui = static_cast<GfxImGuiInternal *>(ImGui::GetIO().UserData); return (gfx_imgui != nullptr && gfx_imgui->magic_ == kConstant_Magic ? gfx_imgui : nullptr); }
};

GfxResult gfxImGuiInitialize(GfxContext gfx, char const **font_filenames, uint32_t font_count,
    ImFontConfig const *font_configs, ImGuiConfigFlags flags)
{
    GfxResult result;
    GfxImGuiInternal *gfx_imgui = new GfxImGuiInternal();
    if(!gfx_imgui) return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to initialize ImGui");
    result = gfx_imgui->initialize(gfx, font_filenames, font_count, font_configs, flags);
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

GfxResult gfxImGuiRender(GfxTexture optionalDrawToTexture)
{
    GfxImGuiInternal *gfx_imgui = GfxImGuiInternal::GetGfxImGui();
    if(!gfx_imgui) return kGfxResult_NoError;   // nothing to render
    return gfx_imgui->render(optionalDrawToTexture);
}

GfxResult gfxImGuiSetDPIScale(float scale)
{
    GfxImGuiInternal *gfx_imgui = GfxImGuiInternal::GetGfxImGui();
    if(!gfx_imgui) return kGfxResult_NoError;   // nothing to update
    gfx_imgui->setDPIScale(scale);
    return kGfxResult_NoError;
}

bool gfxImGuiIsInitialized()
{
    GfxImGuiInternal *gfx_imgui = GfxImGuiInternal::GetGfxImGui();
    return (gfx_imgui != nullptr ? true : false);
}
