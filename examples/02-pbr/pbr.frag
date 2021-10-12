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

uint g_InstanceId;

Texture2D   g_ColorBuffer;
TextureCube g_IrradianceBuffer;

SamplerState g_LinearSampler;

#include "../common/gpu_scene.hlsli"

struct Params
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
};

struct Result
{
    float4 color    : SV_Target0;
    float2 velocity : SV_Target1;
};

Result main(in Params params)
{
    // Load our material
    Instance instance = g_InstanceBuffer[g_InstanceId];
    Mesh     mesh     = g_MeshBuffer[instance.mesh_id];
    Material material = g_MaterialBuffer[mesh.material_id];

    // Load and sample our texture maps
    uint albedo_map      = asuint(material.albedo.w);
    uint roughness_map   = asuint(material.metallicity_roughness.w);
    uint metallicity_map = asuint(material.metallicity_roughness.y);

    if(albedo_map != uint(-1))
    {
        material.albedo.xyz = g_Textures[albedo_map].Sample(g_TextureSampler, params.uv).xyz;
    }

    if(roughness_map != uint(-1))
    {
        material.metallicity_roughness.z = g_Textures[roughness_map].Sample(g_TextureSampler, params.uv).x;
    }

    if(metallicity_map != uint(-1))
    {
        material.metallicity_roughness.x = g_Textures[metallicity_map].Sample(g_TextureSampler, params.uv).x;
    }

    // Post process our material properties
    //material.albedo.xyz            = pow(material.albedo.xyz, 2.2f);
    material.metallicity_roughness.x = saturate(material.metallicity_roughness.x);
    material.metallicity_roughness.z = clamp(material.metallicity_roughness.z, 0.01f, 1.0f);

    // And shade :)
    float3 irradiance = g_IrradianceBuffer.Sample(g_LinearSampler, params.normal).xyz;

    float3 color = material.albedo.xyz * irradiance;

    // Tonemap the color output
    color /= 1.0f + color;
    color  = saturate(color);
    color  = pow(color, 1.0f / 2.2f);
    color  = color * color * (3.0f - 2.0f * color);

    // Populate our multiple render targets (i.e., MRT)
    Result result;
    result.color    = float4(color, 1.0f);
    result.velocity = float2(0.0f, 0.0f);

    return result;
}
