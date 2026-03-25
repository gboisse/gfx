/****************************************************************************
MIT License

Copyright (c) 2026 Guillaume Boissé

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

#include "gfx_core.h"

#include <cstdlib>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

char const *gfxResultGetString(GfxResult result)
{
    switch(result)
    {
    case kGfxResult_NoError:
        return "No error";
    case kGfxResult_InvalidParameter:
        return "Invalid parameter";
    case kGfxResult_InvalidOperation:
        return "Invalid operation";
    case kGfxResult_OutOfMemory:
        return "Out of memory";
    case kGfxResult_InternalError:
        return "Internal error";
    case kGfxResult_DeviceError:
        return "Device error";
    default:
        GFX_ASSERT(0);
        break;  // unknown error
    }
    return "Unknown error";
}

void GFX_PRINTLN_IMPL(char const *file_name, uint32_t line_number, char const *format, ...)
{
    va_list args;
    va_start(args, format);
    GFX_ASSERT(file_name != nullptr);
    int32_t           size = snprintf(nullptr, 0, "%s(%-4u): %s", file_name, line_number, format);
    std::vector<char> body(size + 1);
    snprintf(body.data(), body.size(), "%s(%-4u): %s", file_name, line_number, format);
    size = vsnprintf(nullptr, 0, body.data(), args);
    std::vector<char> message(size + 2);
    vsnprintf(message.data(), message.size(), body.data(), args);
    va_end(args);
    message[message.size() - 2] = '\n';
    OutputDebugStringA(message.data());
    puts(message.data());
}

void GFX_PRINT_ERROR_IMPL(GfxResult result, char const *file_name, uint32_t line_number, char const *format, ...)
{
    va_list args;
    va_start(args, format);
    GFX_ASSERT(file_name != nullptr);
    int32_t size = snprintf(nullptr, 0, "%s(%-4u): error: %s (0x%x: %s)", file_name, line_number, format, (uint32_t)result, gfxResultGetString(result));
    std::vector<char> body(size + 1);
    snprintf(body.data(), body.size(), "%s(%-4u): error: %s (0x%x: %s)", file_name, line_number, format, (uint32_t)result, gfxResultGetString(result));
    size = vsnprintf(nullptr, 0, body.data(), args);
    std::vector<char> message(size + 2);
    vsnprintf(message.data(), message.size(), body.data(), args);
    va_end(args);
    message[message.size() - 2] = '\n';
    OutputDebugStringA(message.data());
    puts(message.data());
}

GfxResult GFX_SET_ERROR_IMPL(GfxResult result, char const *file_name, uint32_t line_number, char const *format, ...)
{
    va_list args;
    va_start(args, format);
    GFX_ASSERT(file_name != nullptr);
    int32_t size = snprintf(nullptr, 0, "%s(%-4u): error: %s (0x%x: %s)", file_name, line_number, format, (uint32_t)result, gfxResultGetString(result));
    std::vector<char> body(size + 1);
    snprintf(body.data(), body.size(), "%s(%-4u): error: %s (0x%x: %s)", file_name, line_number, format, (uint32_t)result, gfxResultGetString(result));
    size = vsnprintf(nullptr, 0, body.data(), args);
    std::vector<char> message(size + 2);
    vsnprintf(message.data(), message.size(), body.data(), args);
    va_end(args);
    message[message.size() - 2] = '\n';
    OutputDebugStringA(message.data());
    puts(message.data());
    return result;
}
