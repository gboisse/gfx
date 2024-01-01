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
#include "gfx_window.h"
#include "gfx_scene.h"
#include "gfx_imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp.cpp"

// Need to match shader-side structure
struct Vertex
{
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec2 uv;
};

float CalculateHaltonNumber(uint32_t index, uint32_t base)
{
    float f      = 1.0f;
    float result = 0.0f;

    for(uint32_t i = index; i > 0;)
    {
        f /= base;
        result = result + f * (i % base);
        i = (uint32_t)(i / (float)base);
    }

    return result;
}

int32_t main()
{
    GfxWindow  window = gfxCreateWindow(1280, 720, "gfx - RTAO");
    GfxContext gfx    = gfxCreateContext(window);
    GfxScene   scene  = gfxCreateScene();
    gfxImGuiInitialize(gfx);

    // Upload the scene to GPU memory
    GfxArray<GfxBuffer> index_buffers;
    GfxArray<GfxBuffer> vertex_buffers;
    GfxArray<GfxTexture> albedo_buffers;

    gfxSceneImport(scene, "data/sponza.obj");   // import our scene

    uint32_t const mesh_count     = gfxSceneGetMeshCount(scene);
    uint32_t const material_count = gfxSceneGetMaterialCount(scene);
    uint32_t const instance_count = gfxSceneGetInstanceCount(scene);

    for(uint32_t i = 0; i < mesh_count; ++i)
    {
        GfxConstRef<GfxMesh> mesh_ref = gfxSceneGetMeshHandle(scene, i);

        uint32_t const index_count  = (uint32_t)mesh_ref->indices.size();
        uint32_t const vertex_count = (uint32_t)mesh_ref->vertices.size();

        std::vector<Vertex> vertices(vertex_count);

        for(uint32_t v = 0; v < vertex_count; ++v)
        {
            vertices[v].position = glm::vec4(mesh_ref->vertices[v].position, 1.0f);
            vertices[v].normal   = glm::vec4(mesh_ref->vertices[v].normal,   0.0f);
            vertices[v].uv       = glm::vec2(mesh_ref->vertices[v].uv);
        }

        GfxBuffer index_buffer  = gfxCreateBuffer<uint32_t>(gfx, index_count, mesh_ref->indices.data());
        GfxBuffer vertex_buffer = gfxCreateBuffer<Vertex>(gfx, vertex_count, vertices.data());

        index_buffers.insert(mesh_ref, index_buffer);
        vertex_buffers.insert(mesh_ref, vertex_buffer);
    }

    for(uint32_t i = 0; i < material_count; ++i)
    {
        GfxConstRef<GfxMaterial> material_ref = gfxSceneGetMaterialHandle(scene, i);

        GfxTexture albedo_buffer;

        if(material_ref->albedo_map)
        {
            GfxImage const &albedo_map = *material_ref->albedo_map;
            uint32_t const mip_count   = gfxCalculateMipCount(albedo_map.width, albedo_map.height);

            albedo_buffer = gfxCreateTexture2D(gfx, albedo_map.width, albedo_map.height, albedo_map.format, mip_count);

            GfxBuffer upload_buffer = gfxCreateBuffer(gfx, albedo_map.width * albedo_map.height * albedo_map.channel_count, albedo_map.data.data());
            gfxCommandCopyBufferToTexture(gfx, albedo_buffer, upload_buffer);
            gfxCommandGenerateMips(gfx, albedo_buffer);
            gfxDestroyBuffer(gfx, upload_buffer);
        }

        albedo_buffers.insert(material_ref, albedo_buffer);
    }

    GfxAccelerationStructure rt_scene = gfxCreateAccelerationStructure(gfx);

    for(uint32_t i = 0; i < instance_count; ++i)
    {
        GfxConstRef<GfxInstance> instance_ref = gfxSceneGetInstanceHandle(scene, i);
        GfxRaytracingPrimitive   rt_mesh      = gfxCreateRaytracingPrimitive(gfx, rt_scene);

        gfxRaytracingPrimitiveBuild(gfx, rt_mesh, index_buffers[instance_ref->mesh], vertex_buffers[instance_ref->mesh]);
    }

    gfxAccelerationStructureUpdate(gfx, rt_scene);

    // Create our raytracing render targets
    GfxTexture color_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT);
    GfxTexture accum_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_R32G32B32A32_FLOAT);
    GfxTexture depth_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_D32_FLOAT);

    // And our blue noise sampler
    GfxBuffer sobol_buffer      = gfxCreateBuffer(gfx, sizeof(sobol_256spp_256d), sobol_256spp_256d);
    GfxBuffer ranking_buffer    = gfxCreateBuffer(gfx, sizeof(rankingTile), rankingTile);
    GfxBuffer scrambling_buffer = gfxCreateBuffer(gfx, sizeof(scramblingTile), scramblingTile);

    // Compile our ambient occlusion shaders
    GfxDrawState trace_draw_state;
    gfxDrawStateSetColorTarget(trace_draw_state, 0, color_buffer.getFormat());
    gfxDrawStateSetDepthStencilTarget(trace_draw_state, depth_buffer.getFormat());

    GfxDrawState accumulate_draw_state;
    gfxDrawStateSetColorTarget(accumulate_draw_state, 0, accum_buffer.getFormat());
    gfxDrawStateSetBlendMode(accumulate_draw_state, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD);

    GfxProgram rtao_program      = gfxCreateProgram(gfx, "rtao");
    GfxKernel  trace_kernel      = gfxCreateGraphicsKernel(gfx, rtao_program, trace_draw_state, "Trace");
    GfxKernel  accumulate_kernel = gfxCreateGraphicsKernel(gfx, rtao_program, accumulate_draw_state, "Accumulate");
    GfxKernel  resolve_kernel    = gfxCreateGraphicsKernel(gfx, rtao_program, "Resolve");

    gfxProgramSetParameter(gfx, rtao_program, "ColorBuffer", color_buffer);
    gfxProgramSetParameter(gfx, rtao_program, "AccumBuffer", accum_buffer);

    gfxProgramSetParameter(gfx, rtao_program, "SobolBuffer", sobol_buffer);
    gfxProgramSetParameter(gfx, rtao_program, "RankingBuffer", ranking_buffer);
    gfxProgramSetParameter(gfx, rtao_program, "ScramblingBuffer", scrambling_buffer);

    gfxProgramSetParameter(gfx, rtao_program, "Scene", rt_scene);

    // Enable anisotropic texture filtering
    GfxSamplerState texture_sampler = gfxCreateSamplerState(gfx, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    gfxProgramSetParameter(gfx, rtao_program, "TextureSampler", texture_sampler);

    // Run application loop
    glm::mat4 previous_view_projection_matrix;

    float previous_ao_radius = 0.0f, ao_radius = 5.0f;

    for(uint32_t frame_index = 0; !gfxWindowIsCloseRequested(window); ++frame_index)
    {
        gfxWindowPumpEvents(window);

        // Show the GUI options
        if(ImGui::Begin("gfx - rtao", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            if(gfxIsRaytracingSupported(gfx))
            {
                ImGui::DragFloat("AO radius", &ao_radius, 1e-2f, 0.0f, 10.0f, "%.2f");
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.0f, 1.0f));
                ImGui::Text("Raytracing isn't supported on device `%s'", gfx.getName());
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();

        // Calculate our camera matrices
        float const aspect_ratio = gfxGetBackBufferWidth(gfx) / (float)gfxGetBackBufferHeight(gfx);

        glm::mat4 view_matrix       = glm::lookAt(glm::vec3(15.0f, 2.5f, 0.0f), glm::vec3(0.0f, 2.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection_matrix = glm::perspective(0.6f, aspect_ratio, 1e-1f, 1e4f);

        if(ao_radius != previous_ao_radius || projection_matrix * view_matrix != previous_view_projection_matrix)
        {
            gfxCommandClearTexture(gfx, accum_buffer);

            previous_ao_radius = ao_radius;

            previous_view_projection_matrix = projection_matrix * view_matrix;
        }

        // Apply anti-aliasing jitter
        projection_matrix[2][0] = (2.0f * CalculateHaltonNumber(frame_index & 255, 2) - 1.0f) / gfxGetBackBufferWidth(gfx);
        projection_matrix[2][1] = (2.0f * CalculateHaltonNumber(frame_index & 255, 3) - 1.0f) / gfxGetBackBufferHeight(gfx);

        glm::mat4 view_projection_matrix = projection_matrix * view_matrix;

        gfxProgramSetParameter(gfx, rtao_program, "AoRadius", ao_radius);
        gfxProgramSetParameter(gfx, rtao_program, "FrameIndex", frame_index);

        gfxProgramSetParameter(gfx, rtao_program, "ViewProjectionMatrix", view_projection_matrix);

        // Perform our occlusion tracing
        gfxCommandBindColorTarget(gfx, 0, color_buffer);
        gfxCommandBindDepthStencilTarget(gfx, depth_buffer);
        gfxCommandBindKernel(gfx, trace_kernel);

        gfxCommandClearTexture(gfx, color_buffer);
        gfxCommandClearTexture(gfx, depth_buffer);

        for(uint32_t i = 0; i < instance_count; ++i)
        {
            GfxInstance const &instance = gfxSceneGetInstances(scene)[i];

            if(instance.material)
                gfxProgramSetParameter(gfx, rtao_program, "AlbedoBuffer", albedo_buffers[instance.material]);
            else
                gfxProgramSetParameter(gfx, rtao_program, "AlbedoBuffer", GfxTexture());    // will return black

            gfxCommandBindIndexBuffer(gfx, index_buffers[instance.mesh]);
            gfxCommandBindVertexBuffer(gfx, vertex_buffers[instance.mesh]);

            gfxCommandDrawIndexed(gfx, (uint32_t)instance.mesh->indices.size());
        }

        // Accumulate the occlusion values
        gfxCommandBindColorTarget(gfx, 0, accum_buffer);
        gfxCommandBindKernel(gfx, accumulate_kernel);

        gfxCommandDraw(gfx, 3);

        // And resolve into the back buffer
        gfxCommandBindKernel(gfx, resolve_kernel);

        gfxCommandDraw(gfx, 3);

        // Finally, we submit the frame
        gfxImGuiRender();
        gfxFrame(gfx);
    }

    // Release resources on termination
    gfxDestroyTexture(gfx, color_buffer);
    gfxDestroyTexture(gfx, accum_buffer);
    gfxDestroyTexture(gfx, depth_buffer);

    gfxDestroyBuffer(gfx, sobol_buffer);
    gfxDestroyBuffer(gfx, ranking_buffer);
    gfxDestroyBuffer(gfx, scrambling_buffer);

    gfxDestroyKernel(gfx, trace_kernel);
    gfxDestroyKernel(gfx, accumulate_kernel);
    gfxDestroyKernel(gfx, resolve_kernel);
    gfxDestroyProgram(gfx, rtao_program);

    gfxDestroySamplerState(gfx, texture_sampler);
    gfxDestroyAccelerationStructure(gfx, rt_scene);

    for(uint32_t i = 0; i < index_buffers.size(); ++i)
        gfxDestroyBuffer(gfx, index_buffers.data()[i]);
    for(uint32_t i = 0; i < vertex_buffers.size(); ++i)
        gfxDestroyBuffer(gfx, vertex_buffers.data()[i]);
    for(uint32_t i = 0; i < albedo_buffers.size(); ++i)
        gfxDestroyTexture(gfx, albedo_buffers.data()[i]);

    gfxImGuiTerminate();
    gfxDestroyScene(scene);
    gfxDestroyContext(gfx);
    gfxDestroyWindow(window);

    return 0;
}
