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

float2 g_TexelSize;

Texture2D g_ColorBuffer;
Texture2D g_HistoryBuffer;
Texture2D g_ResolveBuffer;
Texture2D g_VelocityBuffer;
Texture2D g_DepthBuffer;

SamplerState g_LinearSampler;
SamplerState g_NearestSampler;

float SquareLength(in float2 value)
{
    return (value.x * value.x + value.y * value.y);
}

// https://www.gdcvault.com/play/1022970/Temporal-Reprojection-Anti-Aliasing-in
float2 FindClosestVelocity(in float2 uv, out bool is_sky_pixel)
{
    float2 velocity;
    float closest_depth = 9.9f;
    for(int y = -1; y <= 1; ++y)
        for(int x = -1; x <= 1; ++x)
        {
            float2 tex = uv + float2(x, y) * g_TexelSize;
            float depth = g_DepthBuffer.Sample(g_NearestSampler, tex).x;
            if(depth < closest_depth)
            {
                velocity = g_VelocityBuffer.Sample(g_NearestSampler, tex).xy;
                closest_depth = depth;
            }
        }
    is_sky_pixel = (closest_depth == 1.0f);
    return velocity;
}

float4 Reproject(in float4 pos : SV_Position) : SV_Target
{
    float2 uv = pos.xy * g_TexelSize;

    float3 vsum  = g_ColorBuffer.Sample(g_NearestSampler, uv).xyz;
    float3 vsum2 = vsum * vsum;
    float  wsum  = 1.0f;

    float3 nmin = vsum;
    float3 nmax = vsum;

    for(float y = -1.0f; y <= 1.0f; ++y)
    {
        for(float x = -1.0f; x <= 1.0f; ++x)
        {
            if(x != 0.0f || y != 0.0f)
            {
                float3 v = g_ColorBuffer.Sample(g_NearestSampler, uv + float2(x, y) * g_TexelSize).xyz;
                float  w = exp(-3.0f * SquareLength(float2(x, y)) / 4.0f);

                vsum2 += v * v * w;
                vsum  += v * w;
                wsum  += w;

                nmin = min(v, nmin);
                nmax = max(v, nmax);
            }
        }
    }

    // https://developer.download.nvidia.com/gameworks/events/GDC2016/msalvi_temporal_supersampling.pdf
    float3 ex  = vsum / wsum;
    float3 ex2 = vsum2 / wsum;
    float3 dev = sqrt(max(ex2 - ex * ex, 0.0f));

    bool   is_sky_pixel;
    float2 velocity = FindClosestVelocity(uv, is_sky_pixel);
    float  box_size = lerp(0.5f, 2.5f, is_sky_pixel ? 0.0f : smoothstep(0.02f, 0.0f, length(velocity)));

    nmin = max(ex - dev * box_size, nmin);
    nmax = min(ex + dev * box_size, nmax);

    // http://advances.realtimerendering.com/s2014/#_HIGH-QUALITY_TEMPORAL_SUPERSAMPLING
    float3 history         = g_HistoryBuffer.Sample(g_LinearSampler, uv - velocity).xyz;
    float3 clamped_history = clamp(history, nmin, nmax);
    float3 center          = g_ColorBuffer.Sample(g_NearestSampler, uv).xyz;
    float3 result          = lerp(clamped_history, center, 1.0f / 16.0f);

    return float4(result, 1.0f);
}

float3 RGBToYCoCg(in float3 rgb)
{
    return float3(
         0.25f * rgb.r + 0.5f * rgb.g + 0.25f * rgb.b,
         0.5f  * rgb.r - 0.5f * rgb.b,
        -0.25f * rgb.r + 0.5f * rgb.g - 0.25f * rgb.b);
}

float3 YCoCgToRGB(in float3 yCoCg)
{
    return float3(
        yCoCg.x + yCoCg.y - yCoCg.z,
        yCoCg.x + yCoCg.z,
        yCoCg.x - yCoCg.y - yCoCg.z);
}

// https://en.wikipedia.org/wiki/Unsharp_masking
float3 ApplySharpening(in float3 center, in float3 top, in float3 left, in float3 right, in float3 bottom)
{
    float3 result = RGBToYCoCg(center);

    float unsharpenMask;
    unsharpenMask  = 4.0f * result.x;
    unsharpenMask -= RGBToYCoCg(top).x;
    unsharpenMask -= RGBToYCoCg(bottom).x;
    unsharpenMask -= RGBToYCoCg(left).x;
    unsharpenMask -= RGBToYCoCg(right).x;

    result.x = clamp(result.x + 0.25f * unsharpenMask, 0.9f * result.x, 1.1f * result.x);

    return YCoCgToRGB(result);
}

float4 Resolve(in float4 pos : SV_Position) : SV_Target
{
    float2 uv = pos.xy * g_TexelSize;

    float3 color = ApplySharpening(g_ResolveBuffer.Sample(g_NearestSampler, uv).xyz,
                                   g_ResolveBuffer.Sample(g_NearestSampler, uv + float2( 0.0f,  1.0f) * g_TexelSize).xyz,
                                   g_ResolveBuffer.Sample(g_NearestSampler, uv + float2( 1.0f,  0.0f) * g_TexelSize).xyz,
                                   g_ResolveBuffer.Sample(g_NearestSampler, uv + float2(-1.0f,  0.0f) * g_TexelSize).xyz,
                                   g_ResolveBuffer.Sample(g_NearestSampler, uv + float2( 0.0f, -1.0f) * g_TexelSize).xyz);

    return float4(color, 1.0f);
}
