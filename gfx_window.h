/****************************************************************************
MIT License

Copyright (c) 2021 Guillaume Boissé

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
#pragma once

#include "gfx.h"

//!
//! Window management.
//!

class GfxWindow { friend class GfxWindowInternal; uint64_t handle; HWND hwnd; public:
                  inline operator bool() const { return !!handle; }
                  inline HWND getHWND() const { return hwnd; }
                  inline operator HWND() const { return hwnd; } };

GfxWindow gfxCreateWindow(uint32_t window_width, uint32_t window_height, char const *window_title = nullptr);
GfxResult gfxDestroyWindow(GfxWindow window);

GfxResult gfxWindowPumpEvents(GfxWindow window);
bool gfxWindowIsCloseRequested(GfxWindow window);
bool gfxWindowIsMinimized(GfxWindow window);
bool gfxWindowIsMaximized(GfxWindow window);

//!
//! Implementation details.
//!

#ifdef GFX_IMPLEMENTATION_DEFINE

#include "gfx_imgui.h"  // for (optional) ImGui integration
#include "examples/imgui_impl_win32.h"

class GfxWindowInternal
{
    GFX_NON_COPYABLE(GfxWindowInternal);

    HWND window_ = {};
    bool is_minimized_ = false;
    bool is_maximized_ = false;
    bool is_close_requested_ = false;
    bool is_imgui_initialized_ = false;

public:
    GfxWindowInternal(GfxWindow &window) { window.handle = reinterpret_cast<uint64_t>(this); }
    ~GfxWindowInternal() { terminate(); }

    GfxResult initialize(GfxWindow &window, uint32_t window_width, uint32_t window_height, char const *window_title)
    {
        window_title = (!window_title ? "gfx" : window_title);

        WNDCLASSEX
        window_class               = {};
        window_class.cbSize        = sizeof(window_class);
        window_class.style         = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc   = WindowProc;
        window_class.hInstance     = GetModuleHandle(nullptr);
        window_class.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        window_class.lpszClassName = window_title;

        RegisterClassEx(&window_class);

        RECT window_rect = { 0, 0, (LONG)window_width, (LONG)window_height };

        DWORD const window_style = WS_OVERLAPPEDWINDOW;

        AdjustWindowRect(&window_rect, window_style, FALSE);

        window_ = CreateWindowEx(0,
                                 window_title,
                                 window_title,
                                 window_style,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 window_rect.right - window_rect.left,
                                 window_rect.bottom - window_rect.top,
                                 nullptr,
                                 nullptr,
                                 GetModuleHandle(nullptr),
                                 nullptr);

        SetWindowLongPtrA(window_, GWLP_USERDATA, (LONG_PTR)this);

        ShowWindow(window_, SW_SHOWDEFAULT);

        window.hwnd = window_;

        return kGfxResult_NoError;
    }

    GfxResult terminate()
    {
        if(window_)
            DestroyWindow(window_);
        if(is_imgui_initialized_)
            ImGui_ImplWin32_Shutdown();

        return kGfxResult_NoError;
    }

    GfxResult pumpEvents()
    {
        MSG msg = {};
        while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return kGfxResult_NoError;
    }

    inline bool getIsCloseRequested() const
    {
        return is_close_requested_;
    }

    inline bool getIsMinimized() const
    {
        return is_minimized_;
    }

    inline bool getIsMaximized() const
    {
        return is_maximized_;
    }

    static inline GfxWindowInternal *GetGfxWindow(GfxWindow window) { return reinterpret_cast<GfxWindowInternal *>(window.handle); }

private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
    {
        GfxWindowInternal *gfx_window = (GfxWindowInternal *)GetWindowLongPtrA(window, GWLP_USERDATA);
        if(gfx_window != nullptr)
        {
            if(!gfx_window->is_imgui_initialized_ && gfxImGuiIsInitialized())
                gfx_window->is_imgui_initialized_ = ImGui_ImplWin32_Init(gfx_window->window_);
            switch(message)
            {
            case WM_SIZE:
                {
                    gfx_window->is_minimized_ = IsIconic(gfx_window->window_);
                    gfx_window->is_maximized_ = IsZoomed(gfx_window->window_);
                }
                break;
            case WM_DESTROY:
                {
                    gfx_window->is_close_requested_ = true;
                }
                return 0;
            case WM_CHAR:
            case WM_SETCURSOR:
            case WM_DEVICECHANGE:
            case WM_KEYUP: case WM_SYSKEYUP:
            case WM_KEYDOWN: case WM_SYSKEYDOWN:
            case WM_LBUTTONUP: case WM_RBUTTONUP:
            case WM_MBUTTONUP: case WM_XBUTTONUP:
            case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
            case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
                if(gfx_window->is_imgui_initialized_)
                    ImGui_ImplWin32_WndProcHandler(gfx_window->window_, message, w_param, l_param);
                break;
            default:
                break;
            }
        }
        return DefWindowProc(window, message, w_param, l_param);
    }
};

GfxWindow gfxCreateWindow(uint32_t window_width, uint32_t window_height, char const *window_title)
{
    GfxResult result;
    GfxWindow window = {};
    GfxWindowInternal *gfx_window = new GfxWindowInternal(window);
    if(!gfx_window) return window;  // out of memory
    result = gfx_window->initialize(window, window_width, window_height, window_title);
    if(result != kGfxResult_NoError)
    {
        window = {};
        delete gfx_window;
        GFX_PRINT_ERROR(result, "Failed to create new window");
    }
    return window;
}

GfxResult gfxDestroyWindow(GfxWindow window)
{
    delete GfxWindowInternal::GetGfxWindow(window);
    return kGfxResult_NoError;
}

GfxResult gfxWindowPumpEvents(GfxWindow window)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return kGfxResult_InvalidParameter;
    return gfx_window->pumpEvents();
}

bool gfxWindowIsCloseRequested(GfxWindow window)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return false;   // invalid window handle
    return gfx_window->getIsCloseRequested();
}

bool gfxWindowIsMinimized(GfxWindow window)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return false;   // invalid window handle
    return gfx_window->getIsMinimized();
}

bool gfxWindowIsMaximized(GfxWindow window)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return false;   // invalid window handle
    return gfx_window->getIsMaximized();
}

#endif //! GFX_IMPLEMENTATION_DEFINE
