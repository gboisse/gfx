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

// https://github.com/graphitemaster/normals_revisited
float3 TransformDirection(in float4x4 transform, in float3 direction)
{
    float4x4 result;
    #define minor(m, r0, r1, r2, c0, c1, c2)                            \
        (m[c0][r0] * (m[c1][r1] * m[c2][r2] - m[c1][r2] * m[c2][r1]) -  \
         m[c1][r0] * (m[c0][r1] * m[c2][r2] - m[c0][r2] * m[c2][r1]) +  \
         m[c2][r0] * (m[c0][r1] * m[c1][r2] - m[c0][r2] * m[c1][r1]))
    result[0][0] =  minor(transform, 1, 2, 3, 1, 2, 3);
    result[1][0] = -minor(transform, 1, 2, 3, 0, 2, 3);
    result[2][0] =  minor(transform, 1, 2, 3, 0, 1, 3);
    result[3][0] = -minor(transform, 1, 2, 3, 0, 1, 2);
    result[0][1] = -minor(transform, 0, 2, 3, 1, 2, 3);
    result[1][1] =  minor(transform, 0, 2, 3, 0, 2, 3);
    result[2][1] = -minor(transform, 0, 2, 3, 0, 1, 3);
    result[3][1] =  minor(transform, 0, 2, 3, 0, 1, 2);
    result[0][2] =  minor(transform, 0, 1, 3, 1, 2, 3);
    result[1][2] = -minor(transform, 0, 1, 3, 0, 2, 3);
    result[2][2] =  minor(transform, 0, 1, 3, 0, 1, 3);
    result[3][2] = -minor(transform, 0, 1, 3, 0, 1, 2);
    result[0][3] = -minor(transform, 0, 1, 2, 1, 2, 3);
    result[1][3] =  minor(transform, 0, 1, 2, 0, 2, 3);
    result[2][3] = -minor(transform, 0, 1, 2, 0, 1, 3);
    result[3][3] =  minor(transform, 0, 1, 2, 0, 1, 2);
    return mul(result, float4(direction, 0.0f)).xyz;
    #undef minor    // cleanup
}

#endif //! GPU_SCENE_HLSLI
