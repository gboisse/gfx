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
#pragma once

#include "gfx_scene.h"

struct Mesh
{
    uint32_t count;
    uint32_t first_index;
    uint32_t base_vertex;
    uint32_t padding;
};

struct GpuScene
{
    std::vector<Mesh> meshes;

    GfxBuffer mesh_buffer;
    GfxBuffer index_buffer;
    GfxBuffer vertex_buffer;
    GfxBuffer instance_buffer;
    GfxBuffer material_buffer;
    GfxBuffer transform_buffer;
    GfxBuffer previous_transform_buffer;
    GfxBuffer upload_transform_buffers[kGfxConstant_BackBufferCount];

    std::vector<GfxTexture> textures;

    GfxSamplerState texture_sampler;
};

GpuScene UploadSceneToGpuMemory(GfxContext gfx, GfxScene scene);
void ReleaseGpuScene(GfxContext gfx, GpuScene const &gpu_scene);

void UpdateGpuScene(GfxContext gfx, GfxScene scene, GpuScene &gpu_scene);
void BindGpuScene(GfxContext gfx, GfxProgram program, GpuScene const &gpu_scene);
