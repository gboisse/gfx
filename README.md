# gfx

**gfx** is a minimalist and easy to use graphics API built on top of **Direct3D12**/**HLSL** intended for rapid prototyping.

It supports:

- Full shader reflection; no need for root signatures, pipeline states, descriptor tables, register indexing, etc.
- Internal management of descriptor heaps, resource views, memory allocation, etc.
- "Garbage collection" defers the release of freed resources to ensure the GPU is done with it; you can create and destroy anything at any time without worrying about synchronization.
- Automatic placement of pipeline barriers and resource transitions.
- Runtime shader reloading; simply call the `gfxKernelReloadAll()` function.
- Basic GPU-based parallel primitives (min/max/sum scans and reductions as well as key-only/key-value pair sorting for various data types).
- DXR-1.1 raytracing (i.e. using `RayQuery<>` object); also known as [inline raytracing](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#inline-raytracing).

On top of `gfx.h`, three optional layers are available:

- `gfx_imgui.h`: adds [Dear ImGui](https://github.com/ocornut/imgui) support.
- `gfx_scene.h`: supports loading `.obj` files (`.gltf` coming soon).
- `gfx_window.h`: window creation and basic event management.

## Usage

Include the header corresponding to the desired level of functionality.

In one `.cpp` file, define `#define GFX_IMPLEMENTATION_DEFINE` and include said header to include the implementation details.

## Motivation

This project was born mostly out of frustration while researching rendering techniques using explicit APIs such as **Direct3D12** or **Vulkan**.

These APIs intend to deliver higher performance for complex workloads on both the CPU and the GPU by giving more responsibility to the developer; unfortunately this is to the detriment of productivity especially during the initial prototyping/research phase due to their high level of complexity.

**gfx** aims to provide an efficient API that's easy to learn, pleasant to use and quick to get things set up, while offering rapid iterations; it allows to get that "start from scratch" efficiency while still offering access to the basic necessary functionality (low-level access to GPU work scheduling, drawing UI overlays, etc.).

## Drawing a triangle

Some example code for drawing a simple colored triangle:

```cpp
// main.cpp

#define GFX_IMPLEMENTATION_DEFINE
#include "gfx_window.h"

int main()
{
    auto window = gfxCreateWindow(1280, 720);
    auto gfx = gfxCreateContext(window);

    float vertices[] = {  0.5f, -0.5f, 0.0f,
                          0.0f,  0.7f, 0.0f,
                         -0.5f, -0.5f, 0.0f };
    auto vertex_buffer = gfxCreateBuffer(gfx, sizeof(vertices), vertices);

    auto program = gfxCreateProgram(gfx, "triangle");
    auto kernel = gfxCreateGraphicsKernel(gfx, program);

    for (auto time = 0.0f; !gfxWindowIsCloseRequested(window); time += 0.1f)
    {
        gfxWindowPumpEvents(window);

        float color[] = { 0.5f * cosf(time) + 0.5f,
                          0.5f * sinf(time) + 0.5f,
                          1.0f };
        gfxProgramSetParameter(gfx, program, "Color", color);

        gfxCommandBindKernel(gfx, kernel);
        gfxCommandBindVertexBuffer(gfx, vertex_buffer);

        gfxCommandDraw(gfx, 3);

        gfxFrame(gfx);
    }

    gfxDestroyContext(gfx);
    gfxDestroyWindow(window);

    return 0;
}
```

Here's the corresponding vertex shader:

```cpp
// triangle.vert

float4 main(float3 pos : Position) : SV_Position
{
    return float4(pos, 1.0f);
}
```

And pixel shader:

```cpp
// triangle.frag

float3 Color;

float4 main() : SV_Target
{
    return float4(Color, 1.0f);
}
```
