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
#include "fly_camera.h"

#include "glm/gtc/matrix_transform.hpp"

namespace
{

float CalculateHaltonNumber(uint32_t index, uint32_t base)
{
    float f = 1.0f, result = 0.0f;
    for(uint32_t i = index; i > 0;)
    {
        f /= base;
        result = result + f * (i % base);
        i = (uint32_t)(i / (float)base);
    }
    return result;
}

} //! unnamed namespace

FlyCamera CreateFlyCamera(GfxContext gfx, glm::vec3 const &eye, glm::vec3 const &center)
{
    FlyCamera fly_camera = {};

    float const aspect_ratio = gfxGetBackBufferWidth(gfx) / (float)gfxGetBackBufferHeight(gfx);

    fly_camera.eye    = eye;
    fly_camera.center = center;
    fly_camera.up     = glm::vec3(0.0f, 1.0f, 0.0f);

    fly_camera.view      = glm::lookAt(eye, center, fly_camera.up);
    fly_camera.proj      = glm::perspective(0.6f, aspect_ratio, 1e-1f, 1e4f);
    fly_camera.view_proj = fly_camera.proj * fly_camera.view;

    fly_camera.prev_view      = fly_camera.view;
    fly_camera.prev_proj      = fly_camera.proj;
    fly_camera.prev_view_proj = fly_camera.view_proj;

    return fly_camera;
}

void UpdateFlyCamera(GfxContext gfx, GfxWindow window, FlyCamera &fly_camera)
{
    // Update camera history
    fly_camera.prev_view = fly_camera.view;
    fly_camera.prev_proj = fly_camera.proj;

    // TODO: animate the camera... (gboisse)

    // Update projection aspect ratio
    float const aspect_ratio = gfxGetBackBufferWidth(gfx) / (float)gfxGetBackBufferHeight(gfx);

    fly_camera.proj = glm::perspective(0.6f, aspect_ratio, 1e-1f, 1e4f);

    // Update projection jitter for anti-aliasing
    static uint32_t jitter_index;

    jitter_index = (jitter_index + 1) & 15; // 16 samples TAA

    float const jitter_x = (2.0f * CalculateHaltonNumber(jitter_index + 1, 2) - 1.0f) / gfxGetBackBufferWidth(gfx);
    float const jitter_y = (2.0f * CalculateHaltonNumber(jitter_index + 1, 3) - 1.0f) / gfxGetBackBufferHeight(gfx);

    fly_camera.proj[2][0]      = jitter_x;
    fly_camera.proj[2][1]      = jitter_y;
    fly_camera.prev_proj[2][0] = jitter_x;  // patch previous projection matrix so subpixel jitter doesn't generate velocity values
    fly_camera.prev_proj[2][1] = jitter_y;

    fly_camera.view_proj       = fly_camera.proj      * fly_camera.view;
    fly_camera.prev_view_proj  = fly_camera.prev_proj * fly_camera.prev_view;
}
