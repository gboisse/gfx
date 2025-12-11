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
#ifndef GPU_SCENE_HLSLI
#define GPU_SCENE_HLSLI

struct Material
{
    float4 albedo;
    float4 metallicity_roughness;
    float4 ao_normal_emissivity;
};

struct Mesh
{
    uint count;
    uint first_index;
    uint base_vertex;
    uint padding;
};

struct Instance
{
    uint mesh_id;
    uint material_id;
};

struct Vertex
{
    float4 position : POSITION;
    float4 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

StructuredBuffer<Mesh>     g_MeshBuffer;
StructuredBuffer<uint>     g_IndexBuffer;
StructuredBuffer<Vertex>   g_VertexBuffer;
StructuredBuffer<Instance> g_InstanceBuffer;
StructuredBuffer<Material> g_MaterialBuffer;
StructuredBuffer<float4x4> g_TransformBuffer;
StructuredBuffer<float4x4> g_PreviousTransformBuffer;

Texture2D g_Textures[] : register(space99); // different space to avoid issues with bindless allocating all available texture registers...

SamplerState g_TextureSampler;

float3 TransformNormal(in float4x4 transform, in float3 normal)
{
    float3x3 result = float3x3(
        cross(transform[1].xyz, transform[2].xyz),
        cross(transform[2].xyz, transform[0].xyz),
        cross(transform[0].xyz, transform[1].xyz));
    float3 det = transform[0].xyz * result[0];
    result *= sign(det.x + det.y + det.z);
    return mul(result, normal);
}

#endif //! GPU_SCENE_HLSLI
