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
#include "gfx_window.h"
#include "gfx_scene.h"
#include "gfx_imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp.cpp"

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
    GfxWindow  window = gfxCreateWindow(1280, 720, "gfx - rtao");
    GfxContext gfx    = gfxCreateContext(window);
    GfxScene   scene  = gfxCreateScene();
    gfxImGuiInitialize(gfx);

    // Upload the scene to GPU memory
    GfxArray<GfxBuffer> index_buffers;
    GfxArray<GfxBuffer> vertex_buffers;

    gfxSceneImport(scene, "cornellbox.obj");    // import our scene

    uint32_t const mesh_count     = gfxSceneGetMeshCount(scene);
    uint32_t const instance_count = gfxSceneGetInstanceCount(scene);

    for(uint32_t i = 0; i < mesh_count; ++i)
    {
        GfxConstRef<GfxMesh> mesh_ref = gfxSceneGetMeshHandle(scene, i);

        GfxBuffer index_buffer  = gfxCreateBuffer<uint32_t>(gfx, (uint32_t)mesh_ref->indices.size(), mesh_ref->indices.data());
        GfxBuffer vertex_buffer = gfxCreateBuffer<GfxVertex>(gfx, (uint32_t)mesh_ref->vertices.size(), mesh_ref->vertices.data());

        index_buffers.insert(mesh_ref, index_buffer);
        vertex_buffers.insert(mesh_ref, vertex_buffer);
    }

    GfxAccelerationStructure rt_scene = gfxCreateAccelerationStructure(gfx);

    for(uint32_t i = 0; i < instance_count; ++i)
    {
        GfxConstRef<GfxInstance> instance_ref = gfxSceneGetInstanceHandle(scene, i);
        GfxRaytracingPrimitive   rt_mesh      = gfxCreateRaytracingPrimitive(gfx, rt_scene);

        gfxRaytracingPrimitiveBuild(gfx, rt_mesh, index_buffers[instance_ref->mesh], vertex_buffers[instance_ref->mesh], sizeof(GfxVertex));
    }

    gfxAccelerationStructureUpdate(gfx, rt_scene);

    // Create our raytracing render targets
    GfxTexture ao_buffer    = gfxCreateTexture2D(gfx, DXGI_FORMAT_R32_FLOAT);
    GfxTexture accum_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_R32G32_FLOAT);
    GfxTexture depth_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_D32_FLOAT);

    // And our blue noise sampler
    GfxBuffer sobol_buffer      = gfxCreateBuffer(gfx, sizeof(sobol_256spp_256d), sobol_256spp_256d);
    GfxBuffer ranking_buffer    = gfxCreateBuffer(gfx, sizeof(rankingTile), rankingTile);
    GfxBuffer scrambling_buffer = gfxCreateBuffer(gfx, sizeof(scramblingTile), scramblingTile);

    // Compile our ambient occlusion shaders
    GfxDrawState trace_draw_state;
    gfxDrawStateSetColorTarget(trace_draw_state, 0, ao_buffer);
    gfxDrawStateSetDepthStencilTarget(trace_draw_state, depth_buffer);

    GfxDrawState accumulate_draw_state;
    gfxDrawStateSetColorTarget(accumulate_draw_state, 0, accum_buffer);
    gfxDrawStateSetBlendMode(accumulate_draw_state, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD);

    GfxProgram rtao_program      = gfxCreateProgram(gfx, "rtao");
    GfxKernel  trace_kernel      = gfxCreateGraphicsKernel(gfx, rtao_program, trace_draw_state, "Trace");
    GfxKernel  accumulate_kernel = gfxCreateGraphicsKernel(gfx, rtao_program, accumulate_draw_state, "Accumulate");
    GfxKernel  resolve_kernel    = gfxCreateGraphicsKernel(gfx, rtao_program, "Resolve");

    gfxProgramSetParameter(gfx, rtao_program, "AoBuffer", ao_buffer);
    gfxProgramSetParameter(gfx, rtao_program, "AccumBuffer", accum_buffer);

    gfxProgramSetParameter(gfx, rtao_program, "SobolBuffer", sobol_buffer);
    gfxProgramSetParameter(gfx, rtao_program, "RankingBuffer", ranking_buffer);
    gfxProgramSetParameter(gfx, rtao_program, "ScramblingBuffer", scrambling_buffer);

    gfxProgramSetParameter(gfx, rtao_program, "Scene", rt_scene);

    // Run application loop
    glm::mat4 previous_view_projection_matrix;

    float previous_ao_radius = 0.0f, ao_radius = 0.5f;

    for(uint32_t frame_index = 0; !gfxWindowIsCloseRequested(window); ++frame_index)
    {
        gfxWindowPumpEvents(window);

        // Show the GUI options
        if(ImGui::Begin("gfx - rtao"))
        {
            ImGui::DragFloat("AO radius", &ao_radius, 2e-3f, 0.0f, 1.0f);
        }
        ImGui::End();

        // Calculate our camera matrices
        float const aspect_ratio = gfxGetBackBufferWidth(gfx) / (float)gfxGetBackBufferHeight(gfx);

        glm::mat4 view_matrix       = glm::lookAt(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection_matrix = glm::perspective(0.6f, aspect_ratio, 1e-1f, 1e4f);

        if (ao_radius != previous_ao_radius || projection_matrix * view_matrix != previous_view_projection_matrix)
        {
            gfxCommandClearTexture(gfx, accum_buffer);

            previous_ao_radius = ao_radius;

            previous_view_projection_matrix = projection_matrix * view_matrix;
        }

        // Apply anti-aliasing jitter
        projection_matrix[2][0] = (2.0f * CalculateHaltonNumber(frame_index, 2) - 1.0f) / gfxGetBackBufferWidth(gfx);
        projection_matrix[2][1] = (2.0f * CalculateHaltonNumber(frame_index, 3) - 1.0f) / gfxGetBackBufferHeight(gfx);

        glm::mat4 view_projection_matrix = projection_matrix * view_matrix;

        gfxProgramSetParameter(gfx, rtao_program, "AoRadius", ao_radius);
        gfxProgramSetParameter(gfx, rtao_program, "FrameIndex", frame_index);

        gfxProgramSetParameter(gfx, rtao_program, "ViewProjectionMatrix", view_projection_matrix);

        // Perform our occlusion tracing
        gfxCommandBindKernel(gfx, trace_kernel);

        gfxCommandClearTexture(gfx, ao_buffer);
        gfxCommandClearTexture(gfx, depth_buffer);

        for(uint32_t i = 0; i < instance_count; ++i)
        {
            GfxInstance const &instance = gfxSceneGetInstances(scene)[i];

            gfxCommandBindIndexBuffer(gfx, index_buffers[instance.mesh]);
            gfxCommandBindVertexBuffer(gfx, vertex_buffers[instance.mesh]);

            gfxCommandDrawIndexed(gfx, (uint32_t)instance.mesh->indices.size());
        }

        // Accumulate the occlusion values
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
    gfxDestroyTexture(gfx, ao_buffer);
    gfxDestroyTexture(gfx, accum_buffer);
    gfxDestroyTexture(gfx, depth_buffer);

    gfxDestroyBuffer(gfx, sobol_buffer);
    gfxDestroyBuffer(gfx, ranking_buffer);
    gfxDestroyBuffer(gfx, scrambling_buffer);

    gfxDestroyKernel(gfx, trace_kernel);
    gfxDestroyKernel(gfx, accumulate_kernel);
    gfxDestroyKernel(gfx, resolve_kernel);
    gfxDestroyProgram(gfx, rtao_program);

    gfxDestroyAccelerationStructure(gfx, rt_scene);

    for(uint32_t i = 0; i < index_buffers.size(); ++i)
        gfxDestroyBuffer(gfx, index_buffers.data()[i]);
    for(uint32_t i = 0; i < vertex_buffers.size(); ++i)
        gfxDestroyBuffer(gfx, vertex_buffers.data()[i]);

    gfxImGuiTerminate();
    gfxDestroyScene(scene);
    gfxDestroyContext(gfx);
    gfxDestroyWindow(window);

    return 0;
}
