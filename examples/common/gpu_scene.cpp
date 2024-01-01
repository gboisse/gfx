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
#include "gpu_scene.h"

namespace
{

struct Material
{
    glm::vec4 albedo;
    glm::vec4 metallicity_roughness;
    glm::vec4 ao_normal_emissivity;
};

struct Instance
{
    uint32_t mesh_id;
    uint32_t material_id;
};

struct Vertex
{
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec2 uv;
};

} //! unnamed namespace

GpuScene UploadSceneToGpuMemory(GfxContext gfx, GfxScene scene)
{
    GpuScene gpu_scene = {};

    // Load our materials
    std::vector<Material> materials;

    for(uint32_t i = 0; i < gfxSceneGetMaterialCount(scene); ++i)
    {
        GfxConstRef<GfxMaterial> material_ref = gfxSceneGetMaterialHandle(scene, i);

        Material material = {};
        material.albedo                = glm::vec4(glm::vec3(material_ref->albedo),     glm::uintBitsToFloat((uint32_t)material_ref->albedo_map));
        material.metallicity_roughness = glm::vec4(          material_ref->metallicity, glm::uintBitsToFloat((uint32_t)material_ref->metallicity_map),
                                                             material_ref->roughness,   glm::uintBitsToFloat((uint32_t)material_ref->roughness_map));
        material.ao_normal_emissivity  = glm::vec4(glm::uintBitsToFloat((uint32_t)material_ref->ao_map),
                                                   glm::uintBitsToFloat((uint32_t)material_ref->normal_map),
                                                   glm::uintBitsToFloat((uint32_t)material_ref->emissivity_map), 0.0f);

        uint32_t const material_id = (uint32_t)material_ref;

        if(material_id >= materials.size())
        {
            materials.resize(material_id + 1);
        }

        materials[material_id] = material;
    }

    gpu_scene.material_buffer = gfxCreateBuffer<Material>(gfx, (uint32_t)materials.size(), materials.data());

    // Load our meshes
    uint32_t first_index = 0;
    uint32_t base_vertex = 0;

    for(uint32_t i = 0; i < gfxSceneGetMeshCount(scene); ++i)
    {
        GfxConstRef<GfxMesh> mesh_ref = gfxSceneGetMeshHandle(scene, i);

        Mesh mesh = {};
        mesh.count       = (uint32_t)mesh_ref->indices.size();
        mesh.first_index = first_index;
        mesh.base_vertex = base_vertex;

        uint32_t const mesh_id = (uint32_t)mesh_ref;

        if(mesh_id >= gpu_scene.meshes.size())
        {
            gpu_scene.meshes.resize(mesh_id + 1);
        }

        gpu_scene.meshes[mesh_id] = mesh;

        first_index += (uint32_t)mesh_ref->indices.size();
        base_vertex += (uint32_t)mesh_ref->vertices.size();
    }

    gpu_scene.mesh_buffer = gfxCreateBuffer<Mesh>(gfx, (uint32_t)gpu_scene.meshes.size(), gpu_scene.meshes.data());

    // Load our vertices
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for(uint32_t i = 0; i < gfxSceneGetMeshCount(scene); ++i)
    {
        GfxConstRef<GfxMesh> mesh_ref = gfxSceneGetMeshHandle(scene, i);

        std::vector<uint32_t> const &index_buffer = mesh_ref->indices;

        for(uint32_t index : index_buffer)
        {
            indices.push_back(index);
        }

        std::vector<GfxVertex> const &vertex_buffer = mesh_ref->vertices;

        for(GfxVertex vertex : vertex_buffer)
        {
            Vertex gpu_vertex = {};

            gpu_vertex.position = glm::vec4(vertex.position, 1.0f);
            gpu_vertex.normal   = glm::vec4(vertex.normal,   0.0f);
            gpu_vertex.uv       = glm::vec2(vertex.uv);

            vertices.push_back(gpu_vertex);
        }
    }

    gpu_scene.index_buffer  = gfxCreateBuffer<uint32_t>(gfx, (uint32_t)indices.size(), indices.data());
    gpu_scene.vertex_buffer = gfxCreateBuffer<Vertex>(gfx, (uint32_t)vertices.size(), vertices.data());

    // Load our instances
    std::vector<Instance>  instances;
    std::vector<glm::mat4> transforms;

    for(uint32_t i = 0; i < gfxSceneGetInstanceCount(scene); ++i)
    {
        GfxConstRef<GfxInstance> const instance_ref = gfxSceneGetInstanceHandle(scene, i);

        Instance instance    = {};
        instance.mesh_id     = (uint32_t)instance_ref->mesh;
        instance.material_id = (uint32_t)instance_ref->material;

        uint32_t const instance_id = (uint32_t)instance_ref;

        if(instance_id >= instances.size())
        {
            instances.resize(instance_id + 1);
            transforms.resize(instance_id + 1);
        }

        instances[instance_id]  = instance;
        transforms[instance_id] = instance_ref->transform;
    }

    gpu_scene.instance_buffer           = gfxCreateBuffer<Instance>(gfx, (uint32_t)instances.size(), instances.data());
    gpu_scene.transform_buffer          = gfxCreateBuffer<glm::mat4>(gfx, (uint32_t)transforms.size(), transforms.data());
    gpu_scene.previous_transform_buffer = gfxCreateBuffer<glm::mat4>(gfx, (uint32_t)transforms.size(), transforms.data());

    for(GfxBuffer &upload_transform_buffer : gpu_scene.upload_transform_buffers)
    {
        upload_transform_buffer = gfxCreateBuffer<glm::mat4>(gfx, (uint32_t)transforms.size(), nullptr, kGfxCpuAccess_Write);
    }

    for(uint32_t i = 0; i < gfxSceneGetImageCount(scene); ++i)
    {
        GfxConstRef<GfxImage> const image_ref = gfxSceneGetImageHandle(scene, i);

        GfxTexture texture = gfxCreateTexture2D(gfx, image_ref->width, image_ref->height, image_ref->format, gfxCalculateMipCount(image_ref->width, image_ref->height));

        uint32_t const texture_size = image_ref->width * image_ref->height * image_ref->channel_count * image_ref->bytes_per_channel;

        GfxBuffer upload_texture_buffer = gfxCreateBuffer(gfx, texture_size, image_ref->data.data(), kGfxCpuAccess_Write);

        gfxCommandCopyBufferToTexture(gfx, texture, upload_texture_buffer);
        gfxDestroyBuffer(gfx, upload_texture_buffer);
        gfxCommandGenerateMips(gfx, texture);

        uint32_t const image_id = (uint32_t)image_ref;

        if(image_id >= gpu_scene.textures.size())
        {
            gpu_scene.textures.resize(image_id + 1);
        }

        gpu_scene.textures[image_id] = texture;
    }

    gpu_scene.texture_sampler = gfxCreateSamplerState(gfx, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    return gpu_scene;
}

void ReleaseGpuScene(GfxContext gfx, GpuScene const &gpu_scene)
{
    gfxDestroyBuffer(gfx, gpu_scene.mesh_buffer);
    gfxDestroyBuffer(gfx, gpu_scene.index_buffer);
    gfxDestroyBuffer(gfx, gpu_scene.vertex_buffer);
    gfxDestroyBuffer(gfx, gpu_scene.instance_buffer);
    gfxDestroyBuffer(gfx, gpu_scene.material_buffer);
    gfxDestroyBuffer(gfx, gpu_scene.transform_buffer);
    gfxDestroyBuffer(gfx, gpu_scene.previous_transform_buffer);

    for(GfxBuffer upload_transform_buffer : gpu_scene.upload_transform_buffers)
    {
        gfxDestroyBuffer(gfx, upload_transform_buffer);
    }

    for(GfxTexture texture : gpu_scene.textures)
    {
        gfxDestroyTexture(gfx, texture);
    }

    gfxDestroySamplerState(gfx, gpu_scene.texture_sampler);
}

void UpdateGpuScene(GfxContext gfx, GfxScene scene, GpuScene &gpu_scene)
{
    GfxBuffer upload_transform_buffer = gpu_scene.upload_transform_buffers[gfxGetBackBufferIndex(gfx)];

    glm::mat4 *transforms = gfxBufferGetData<glm::mat4>(gfx, upload_transform_buffer);

    uint32_t const instance_count = gfxSceneGetInstanceCount(scene);

    for(uint32_t i = 0; i < instance_count; ++i)
    {
        GfxConstRef<GfxInstance> const instance_ref = gfxSceneGetInstanceHandle(scene, i);

        uint32_t const instance_id = (uint32_t)instance_ref;

        transforms[instance_id] = instance_ref->transform;
    }

    gfxCommandCopyBuffer(gfx, gpu_scene.previous_transform_buffer, gpu_scene.transform_buffer);

    gfxCommandCopyBuffer(gfx, gpu_scene.transform_buffer, upload_transform_buffer);
}

void BindGpuScene(GfxContext gfx, GfxProgram program, GpuScene const &gpu_scene)
{
    gfxProgramSetParameter(gfx, program, "g_MeshBuffer", gpu_scene.mesh_buffer);
    gfxProgramSetParameter(gfx, program, "g_IndexBuffer", gpu_scene.index_buffer);
    gfxProgramSetParameter(gfx, program, "g_VertexBuffer", gpu_scene.vertex_buffer);
    gfxProgramSetParameter(gfx, program, "g_InstanceBuffer", gpu_scene.instance_buffer);
    gfxProgramSetParameter(gfx, program, "g_MaterialBuffer", gpu_scene.material_buffer);
    gfxProgramSetParameter(gfx, program, "g_TransformBuffer", gpu_scene.transform_buffer);
    gfxProgramSetParameter(gfx, program, "g_PreviousTransformBuffer", gpu_scene.previous_transform_buffer);

    gfxProgramSetParameter(gfx, program, "g_Textures", gpu_scene.textures.data(), (uint32_t)gpu_scene.textures.size());

    gfxProgramSetParameter(gfx, program, "g_TextureSampler", gpu_scene.texture_sampler);
}
