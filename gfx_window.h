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
#ifndef GFX_INCLUDE_GFX_WINDOW_H
#define GFX_INCLUDE_GFX_WINDOW_H

#include "gfx.h"

//!
//! Window management.
//!

class GfxWindow { friend class GfxWindowInternal; uint64_t handle; HWND hwnd; public:
                  inline bool operator ==(GfxWindow const &other) const { return handle == other.handle; }
                  inline bool operator !=(GfxWindow const &other) const { return handle != other.handle; }
                  inline GfxWindow() { memset(this, 0, sizeof(*this)); }
                  inline operator bool() const { return !!handle; }
                  inline HWND getHWND() const { return hwnd; }
                  inline operator HWND() const { return hwnd; } };

enum GfxCreateWindowFlag
{
    kGfxCreateWindowFlag_MaximizeWindow   = 1 << 0,
    kGfxCreateWindowFlag_NoResizeWindow   = 1 << 1,
    kGfxCreateWindowFlag_HideWindow       = 1 << 2,
    kGfxCreateWindowFlag_FullscreenWindow = 1 << 3,
    kGfxCreateWindowFlag_AcceptDrop       = 1 << 4
};
typedef uint32_t GfxCreateWindowFlags;

GfxWindow gfxCreateWindow(uint32_t window_width, uint32_t window_height, char const *window_title = nullptr, GfxCreateWindowFlags flags = 0);
GfxResult gfxDestroyWindow(GfxWindow window);

GfxResult gfxWindowPumpEvents(GfxWindow window);

bool gfxWindowIsKeyDown(GfxWindow window, uint32_t key_code);   // https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
bool gfxWindowIsKeyPressed(GfxWindow window, uint32_t key_code);
bool gfxWindowIsKeyReleased(GfxWindow window, uint32_t key_code);
bool gfxWindowIsCloseRequested(GfxWindow window);
bool gfxWindowIsMinimized(GfxWindow window);
bool gfxWindowIsMaximized(GfxWindow window);
bool gfxWindowRegisterDropCallback(GfxWindow window, void (*callback)(char const *, uint32_t, void *), void *data = nullptr);
bool gfxWindowUnregisterDropCallback(GfxWindow window);

#endif //! GFX_INCLUDE_GFX_WINDOW_H

//!
//! Implementation details.
//!

#ifdef GFX_IMPLEMENTATION_DEFINE

#pragma once
#include "gfx_window.cpp"

#endif //! GFX_IMPLEMENTATION_DEFINE
