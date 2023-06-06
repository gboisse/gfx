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

int main()
{
    auto window = gfxCreateWindow(1280, 720, "gfx - Hello, compute!", kGfxCreateWindowFlag_HideWindow );
    auto gfx = gfxCreateContext(window);

    int n = 128;
    auto src = gfxCreateBuffer(gfx, sizeof(int) * n, 0);
    auto dst = gfxCreateBuffer(gfx, sizeof(int) * n, 0);
    auto dstHostAccess = gfxCreateBuffer(gfx, sizeof(int) * n, 0, kGfxCpuAccess_Read);
    {
        auto staging = gfxCreateBuffer(gfx, sizeof(int) * n, 0, kGfxCpuAccess_Write);
        int* host = gfxBufferGetData<int>(gfx, staging);
        for (int i = 0; i < n; i++)
            host[i] = i;
        auto e = gfxCommandCopyBuffer(gfx, src, staging);
        e = gfxFrame(gfx);
        e = gfxDestroyBuffer(gfx, staging);
    }
    GfxResult e = kGfxResult_NoError;
    {
        auto program = gfxCreateProgram(gfx, "simple");
        auto kernel = gfxCreateComputeKernel(gfx, program);

        e = gfxCommandBindKernel(gfx, kernel);
        uint32_t const* num_threads = gfxKernelGetNumThreads(gfx, kernel);
        e = gfxProgramSetBuffer(gfx, program, "InputBuffer", src);
        e = gfxProgramSetBuffer(gfx, program, "OutputBuffer", dst);
        e = gfxCommandDispatch(gfx, 1, 1, 1);
        e = gfxCommandCopyBuffer(gfx, dstHostAccess, dst);
        e = gfxFrame(gfx);
        int* host = gfxBufferGetData<int>(gfx, dstHostAccess);
        bool success = true;
        for (int i = 0; i < n; i++)
        {
            if (host[i] != i)
            {
                printf("error %d (%d)\n", i, host[i]);
                success = false;
            }
        }
        printf("%s\n", success ? "success" : "failure");
    }
    e = gfxDestroyBuffer(gfx, src);
    e = gfxDestroyBuffer(gfx, dst);
    e = gfxDestroyBuffer(gfx, dstHostAccess);
    e = gfxDestroyContext(gfx);
    e = gfxDestroyWindow(window);

    return 0;
}
