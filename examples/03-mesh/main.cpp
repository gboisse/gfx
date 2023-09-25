/****************************************************************************
MIT License

Copyright (c) 2023 Guillaume Boissé

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
#include "gfx_window.h"

int32_t main()
{
    GfxWindow window = gfxCreateWindow(1280, 720, "gfx - Mesh shaders");
    GfxContext gfx = gfxCreateContext(window);

    GfxProgram program = gfxCreateProgram(gfx, "mesh");
    GfxKernel kernel  = gfxCreateMeshKernel(gfx, program);

    for(float time = 0.0f; !gfxWindowIsCloseRequested(window); time += 0.1f)
    {
        gfxWindowPumpEvents(window);

        float color[] = { 0.5f * cosf(time) + 0.5f,
                          0.5f * sinf(time) + 0.5f,
                          1.0f };
        gfxProgramSetParameter(gfx, program, "Color", color);

        gfxCommandBindKernel(gfx, kernel);
        gfxCommandDrawMesh(gfx, 1, 1, 1);

        gfxFrame(gfx);
    }

    gfxDestroyKernel(gfx, kernel);
    gfxDestroyProgram(gfx, program);

    gfxDestroyContext(gfx);
    gfxDestroyWindow(window);

    return 0;
}
