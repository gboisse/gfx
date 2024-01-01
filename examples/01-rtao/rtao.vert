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

float4x4 ViewProjectionMatrix;

// Need to match host-side structure
struct Vertex
{
    float4 position : POSITION;
    float4 normal   : NORMAL;
    float2 uv       : TEXCOORDS;
};

struct Params
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float3 world    : POSITION;
    float2 uv       : TEXCOORDS;
};

Params Trace(in Vertex vertex)
{
    Params params;
    params.position = mul(ViewProjectionMatrix, vertex.position);
    params.normal   = vertex.normal.xyz;
    params.world    = vertex.position.xyz;
    params.uv       = vertex.uv;
    return params;
}

float4 Accumulate(in uint idx : SV_VertexID) : SV_Position
{
    return 1.0f - float4(4.0f * (idx & 1), 4.0f * (idx >> 1), 1.0f, 0.0f);
}

float4 Resolve(in uint idx : SV_VertexID) : SV_Position
{
    return 1.0f - float4(4.0f * (idx & 1), 4.0f * (idx >> 1), 1.0f, 0.0f);
}
