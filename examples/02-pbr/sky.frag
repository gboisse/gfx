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

float3   g_Eye;
float2   g_TexelSize;
float4x4 g_ViewProjectionInverse;

TextureCube g_EnvironmentBuffer;

SamplerState g_LinearSampler;

float4 main(in float4 pos : SV_Position) : SV_Target
{
    float2 uv  = pos.xy * g_TexelSize;
    float2 ndc = 2.0f * float2(uv.x, 1.0f - uv.y) - 1.0f;

    // Retrieve the ray direction and sample the environment
    float4 world = mul(g_ViewProjectionInverse, float4(ndc, 1.0f, 1.0f));
    world /= world.w;   // perspective divide

    float3 color = g_EnvironmentBuffer.SampleLevel(g_LinearSampler, world.xyz - g_Eye, 1.0f).xyz;

    // Tonemap the color output
    color *= 0.75f;
    color /= 1.0f + color;
    color  = saturate(color);
    color  = sqrt(color);
    color  = color * color * (3.0f - 2.0f * color);

    return float4(color, 1.0f);
}
