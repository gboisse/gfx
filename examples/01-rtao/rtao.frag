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

#define GOLDEN_RATIO 1.61803398874989484820f
#define PI           3.14159265358979323846f

float AoRadius;
uint  FrameIndex;

Texture2D ColorBuffer;
Texture2D AccumBuffer;
Texture2D AlbedoBuffer;

StructuredBuffer<uint> SobolBuffer;
StructuredBuffer<uint> RankingBuffer;
StructuredBuffer<uint> ScramblingBuffer;

RaytracingAccelerationStructure Scene;

SamplerState TextureSampler;

struct Params
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float3 world    : POSITION;
    float2 uv       : TEXCOORDS;
};

// https://perso.liris.cnrs.fr/david.coeurjolly/publications/heitz19.html
float samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(in int pixel_i, in int pixel_j, in int sample_index, in int sample_dimension)
{
    // wrap arguments
    pixel_i          = (pixel_i & 127);
    pixel_j          = (pixel_j & 127);
    sample_index     = (sample_index & 255);
    sample_dimension = (sample_dimension & 255);

    // xor index based on optimized ranking
    int ranked_sample_index = sample_index ^ RankingBuffer[sample_dimension + (pixel_i + pixel_j * 128) * 8];

    // fetch value in sequence
    int value = SobolBuffer[sample_dimension + ranked_sample_index * 256];

    // If the dimension is optimized, xor sequence value based on optimized scrambling
    value = value ^ ScramblingBuffer[(sample_dimension % 8) + (pixel_i + pixel_j * 128) * 8];

    // convert to float and return
    return (0.5f + value) / 256.0f;
}

// https://blog.demofox.org/2017/10/31/animating-noise-for-integration-over-time/
float2 BlueNoise_Sample2D(in uint2 pixel, in uint sample_index, in uint dimension_offset)
{
    float2 s = float2(samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(pixel.x, pixel.y, 0, dimension_offset + 0),
                      samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(pixel.x, pixel.y, 0, dimension_offset + 1));

    return fmod(s + (sample_index & 255) * GOLDEN_RATIO, 1.0f);
}

// https://graphics.pixar.com/library/OrthonormalB/paper.pdf
void GetOrthoVectors(in float3 n, out float3 b1, out float3 b2)
{
    float sign = (n.z >= 0.0f ? 1.0f : -1.0f);
    float a = -1.0f / (sign + n.z);
    float b = n.x * n.y * a;

    b1 = float3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
    b2 = float3(b, sign + n.y * n.y * a, -n.y);
}

float3 MapToHemisphere(in float2 s, in float3 n, in float e)
{
    float3 u, v;
    GetOrthoVectors(n, u, v);

    float r1 = s.x;
    float r2 = s.y;

    float sinpsi = sin(2.0f * PI * r1);
    float cospsi = cos(2.0f * PI * r1);
    float costheta = pow(1.0f - r2, 1.0f / (e + 1.0f));
    float sintheta = sqrt(1.0f - costheta * costheta);

    return normalize(u * sintheta * cospsi + v * sintheta * sinpsi + n * costheta);
}

float4 Trace(in Params params) : SV_Target
{
    float2 s = BlueNoise_Sample2D(params.position.xy, FrameIndex, 0);

    RayDesc ray_desc;
    ray_desc.Direction = MapToHemisphere(s, params.normal, 1.0f);
    ray_desc.Origin    = params.world;
    ray_desc.TMin      = 1e-3f;
    ray_desc.TMax      = AoRadius;

    RayQuery<RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> ray_query;
    ray_query.TraceRayInline(Scene, RAY_FLAG_NONE, 0xFFu, ray_desc);
    ray_query.Proceed();

    float  ao_value = (ray_query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.0f : 1.0f);
    float3 albedo   = AlbedoBuffer.Sample(TextureSampler, params.uv).xyz;

    return float4(albedo * ao_value, 1.0f);
}

float4 Accumulate(in float4 pos : SV_Position) : SV_Target
{
    return ColorBuffer.Load(int3(pos.xy, 0));
}

float4 Resolve(in float4 pos : SV_Position) : SV_Target
{
    float4 color_sample = AccumBuffer.Load(int3(pos.xy, 0));
    float3 color_value  = color_sample.xyz / max(color_sample.w - 1.0f, 1.0f);

    return float4(sqrt(color_value), 1.0f);
}
