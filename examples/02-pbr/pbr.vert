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
#include "../common/gpu_scene.hlsli"

uint     g_InstanceId;
float4x4 g_ViewProjection;
float4x4 g_PreviousViewProjection;

struct Params
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float3 world    : POSITION0;
    float4 current  : POSITION1;
    float4 previous : POSITION2;
};

Params main(in Vertex vertex)
{
    float4x4 transform          = g_TransformBuffer[g_InstanceId];
    float4x4 previous_transform = g_PreviousTransformBuffer[g_InstanceId];
    float4   position           = mul(transform, vertex.position);
    float4   previous_position  = mul(previous_transform, vertex.position);
    float3   normal             = TransformDirection(transform, vertex.normal.xyz);

    Params params;
    params.position = mul(g_ViewProjection, position);
    params.normal   = normal;
    params.uv       = vertex.uv;
    params.world    = position.xyz;
    params.current  = params.position;
    params.previous = mul(g_PreviousViewProjection, previous_position);

    return params;
}
