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

#include "gfx_window.h"

#    ifdef GFX_ENABLE_GUI
#include "gfx_imgui.h"  // for (optional) ImGui integration
#       ifdef GFX_IMGUI_SOURCE
#include "backends/imgui_impl_win32.h"
#       else
#include "imgui_impl_win32.h"
#       endif
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#   endif

class GfxWindowInternal
{
    GFX_NON_COPYABLE(GfxWindowInternal);

    HWND window_ = {};
    bool is_minimized_ = false;
    bool is_maximized_ = false;
    bool is_close_requested_ = false;
    bool is_key_down_[VK_OEM_CLEAR] = {};
    bool is_previous_key_down_[VK_OEM_CLEAR] = {};
    void (*drop_callback_)(char const *, uint32_t, void *) = nullptr;
    void *callback_data_ = nullptr;
    GfxCreateWindowFlags flags_;
    uint32_t window_width_;
    uint32_t window_height_;
    uint32_t original_window_width_;
    uint32_t original_window_height_;

public:
    GfxWindowInternal(GfxWindow &window) { window.handle = reinterpret_cast<uint64_t>(this); }
    ~GfxWindowInternal() { terminate(); }

    GfxResult initialize(GfxWindow &window, uint32_t window_width, uint32_t window_height, char const *window_title, GfxCreateWindowFlags flags)
    {
        flags_ = flags;
        window_width_ = window_width;
        window_height_ = window_height;
        original_window_width_ = window_width_;
        original_window_height_ = window_height_;

        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

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

        DWORD window_style_ex;
        DWORD const window_style = getWindowStyle(window_style_ex, flags_);

        window_ = CreateWindowEx(window_style_ex,
                                 window_title,
                                 window_title,
                                 window_style,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 0,
                                 0,
                                 nullptr,
                                 nullptr,
                                 GetModuleHandle(nullptr),
                                 nullptr);

        if((flags_ & kGfxCreateWindowFlag_FullscreenWindow) != 0)
        {
            setFullscreen();
        }
        else
        {
            setWindowed(window_style, window_style_ex);
        }

        SetWindowLongPtrA(window_, GWLP_USERDATA, (LONG_PTR)this);

        ShowWindow(window_, (flags_ & kGfxCreateWindowFlag_MaximizeWindow) != 0 ? SW_SHOWMAXIMIZED :
                            (flags_ & kGfxCreateWindowFlag_HideWindow    ) != 0 ? SW_HIDE          :
                                                                                  SW_SHOWDEFAULT);

        window.hwnd = window_;

        return kGfxResult_NoError;
    }

    GfxResult terminate()
    {
        if(window_)
            DestroyWindow(window_);
#   ifdef GFX_ENABLE_GUI
        if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendPlatformUserData != nullptr)
            ImGui_ImplWin32_Shutdown();
#   endif

        return kGfxResult_NoError;
    }

    GfxResult pumpEvents()
    {
        MSG msg = {};
        memcpy(is_previous_key_down_, is_key_down_, sizeof(is_key_down_));
        while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return kGfxResult_NoError;
    }

    inline bool getIsKeyDown(uint32_t key_code) const
    {
        return (key_code < ARRAYSIZE(is_key_down_) ? is_key_down_[key_code] : false);
    }

    inline bool getIsKeyPressed(uint32_t key_code) const
    {
        return (key_code < ARRAYSIZE(is_key_down_) ? is_key_down_[key_code] && !is_previous_key_down_[key_code] : false);
    }

    inline bool getIsKeyReleased(uint32_t key_code) const
    {
        return (key_code < ARRAYSIZE(is_key_down_) ? !is_key_down_[key_code] && is_previous_key_down_[key_code] : false);
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

    inline void registerDropCallback(void (*callback)(char const *, uint32_t, void *), void *data)
    {
        drop_callback_ = callback;
        callback_data_ = data;
    }

    inline void unregisterDropCallback()
    {
        drop_callback_ = nullptr;
        callback_data_ = nullptr;
    }

    inline float getDPIScale() const
    {
        UINT dpi = GetDpiForWindow(window_);
        return (float)dpi / (float)USER_DEFAULT_SCREEN_DPI;
    }

    inline void setFullscreen(bool creation = true) const
    {
        MONITORINFO mi;
        memset(&mi, 0, sizeof(mi));
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowPos(window_, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_NOZORDER | (creation ? 0 : (SWP_FRAMECHANGED | SWP_SHOWWINDOW)));
    }

    inline void setWindowed(DWORD window_style, DWORD window_style_ex, bool creation = true) const
    {
        RECT window_rect = { 0, 0, (LONG)original_window_width_, (LONG)original_window_height_ };
        UINT dpi = GetDpiForWindow(window_);
        AdjustWindowRectExForDpi(&window_rect, window_style, FALSE, window_style_ex, dpi);
        window_rect.right -= window_rect.left;
        window_rect.bottom -= window_rect.top;
        window_rect.left = 0;
        window_rect.top = 0;
        RECT work_area;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
        LONG work_area_width = work_area.right - work_area.left;
        LONG work_area_height = work_area.bottom - work_area.top;
        if(work_area_width > window_rect.right)
        {
            window_rect.left = (work_area_width - window_rect.right) / 2;
            window_rect.right += window_rect.left;
        }
        else if((flags_ & kGfxCreateWindowFlag_ShrinkToScreen) != 0)
            window_rect.right = work_area_width;
        if(work_area_height > window_rect.bottom)
        {
            window_rect.top = (work_area_height - window_rect.bottom) / 2;
            window_rect.bottom += window_rect.top;
        }
        else if((flags_ & kGfxCreateWindowFlag_ShrinkToScreen) != 0)
            window_rect.bottom = work_area_height;
        SetWindowPos(window_, NULL, window_rect.left, window_rect.top, window_rect.right - window_rect.left,
                     window_rect.bottom - window_rect.top, SWP_NOZORDER | (creation ? SWP_NOACTIVATE : (SWP_FRAMECHANGED | SWP_SHOWWINDOW)));
    }

    inline void toggleFullscreen() const
    {
        DWORD dwStyle = GetWindowLong(window_, GWL_STYLE);
        if((dwStyle & WS_SYSMENU) != 0)
        {
            DWORD       window_style_ex;
            DWORD const window_style = getWindowStyle(window_style_ex, flags_ | kGfxCreateWindowFlag_FullscreenWindow);
            SetWindowLong(window_, GWL_STYLE, window_style);
            SetWindowLong(window_, GWL_EXSTYLE, window_style_ex);
            setFullscreen(false);
        }
        else
        {
            DWORD       window_style_ex;
            DWORD const window_style = getWindowStyle(window_style_ex, flags_ & ~kGfxCreateWindowFlag_FullscreenWindow);
            SetWindowLong(window_, GWL_STYLE, window_style);
            SetWindowLong(window_, GWL_EXSTYLE, window_style_ex);
            setWindowed(window_style, window_style_ex, false);
        }
    }

    static inline GfxWindowInternal *GetGfxWindow(GfxWindow window) { return reinterpret_cast<GfxWindowInternal *>(window.handle); }

private:
    static inline DWORD getWindowStyle(DWORD &window_style_ex, GfxCreateWindowFlags flags)
    {
        DWORD const window_style = ((flags & kGfxCreateWindowFlag_FullscreenWindow) != 0) ? WS_POPUP :
                                       (WS_OVERLAPPEDWINDOW & ~((flags & kGfxCreateWindowFlag_NoResizeWindow) != 0 ?
                                                                    WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX : 0));
        window_style_ex = (((flags & kGfxCreateWindowFlag_FullscreenWindow) != 0) ? 0 : WS_EX_OVERLAPPEDWINDOW) |
                                      ((flags & kGfxCreateWindowFlag_AcceptDrop) != 0 ? WS_EX_ACCEPTFILES : 0);
        return window_style;
    }

    inline void updateKeyBinding(uint32_t message, uint32_t key_code)
    {
        bool is_down;
        switch(message)
        {
        case WM_KEYUP: case WM_SYSKEYUP:
            is_down = false;
            break;
        case WM_KEYDOWN: case WM_SYSKEYDOWN:
            is_down = true;
            break;
        default:
            return;
        }
        is_key_down_[key_code] = is_down;
    }

    inline void resetKeyBindings()
    {
        for(uint32_t i = 0; i < ARRAYSIZE(is_key_down_); ++i)
            is_key_down_[i] = false;
    }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
    {
        GfxWindowInternal *gfx_window = (GfxWindowInternal *)GetWindowLongPtrA(window, GWLP_USERDATA);
        if(gfx_window != nullptr)
        {
#   ifdef GFX_ENABLE_GUI
            if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendPlatformUserData == nullptr && gfxImGuiIsInitialized())
                ImGui_ImplWin32_Init(gfx_window->window_);
#   endif
            SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            switch(message)
            {
            case WM_SIZE:
                {
                    gfx_window->is_minimized_  = IsIconic(gfx_window->window_);
                    gfx_window->is_maximized_  = IsZoomed(gfx_window->window_);
                    gfx_window->window_width_  = LOWORD(l_param);
                    gfx_window->window_height_ = HIWORD(l_param);
                }
                break;
            case WM_DESTROY:
                {
                    gfx_window->is_close_requested_ = true;
                }
                return 0;
            case WM_KILLFOCUS:
                {
                    gfx_window->resetKeyBindings();
                }
                break;
            case WM_DROPFILES:
                {
                    HDROP hdrop = (HDROP)w_param;
                    if(gfx_window->drop_callback_ != nullptr)
                    {
                        char file_name[MAX_PATH];
                        // Get the number of files dropped onto window
                        uint32_t const file_count = DragQueryFileA(hdrop, 0xFFFFFFFFu, file_name, MAX_PATH);
                        for(uint32_t i = 0; i < file_count; i++)
                        {
                            // Get i'th filename
                            DragQueryFileA(hdrop, i, file_name, MAX_PATH);

                            // Pass to callback function
                            (*gfx_window->drop_callback_)(file_name, i, gfx_window->callback_data_);
                        }
                    }
                    DragFinish(hdrop);
                }
                break;
            case WM_GETDPISCALEDSIZE:
                {
                    DWORD const dpi = (WORD)w_param;
                    RECT window_rect = { 0, 0, (LONG)gfx_window->window_width_, (LONG)gfx_window->window_height_ };
                    DWORD const window_style = WS_OVERLAPPEDWINDOW & ~((gfx_window->flags_ & kGfxCreateWindowFlag_NoResizeWindow) != 0 ?
                                                                           WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX : 0);
                    DWORD const window_style_ex = WS_EX_OVERLAPPEDWINDOW & ((gfx_window->flags_ & kGfxCreateWindowFlag_AcceptDrop) != 0 ? WS_EX_ACCEPTFILES : 0);
                    AdjustWindowRectExForDpi(&window_rect, window_style, FALSE, window_style_ex, dpi);
                    SIZE* const out = (SIZE*)l_param;
                    out->cx = window_rect.right - window_rect.left;
                    out->cy = window_rect.bottom - window_rect.top;
                    return TRUE;
                }
#   ifdef GFX_ENABLE_GUI
            case WM_DPICHANGED:
                {
                    UINT  dpi              = HIWORD(w_param);
                    float dpi_scale_factor = (float)dpi / (float)USER_DEFAULT_SCREEN_DPI;
                    gfxImGuiSetDPIScale(dpi_scale_factor);
                }
                break;
#   endif
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
                if(w_param < ARRAYSIZE(is_key_down_))
                    gfx_window->updateKeyBinding(message, (uint32_t)w_param);
#   ifdef GFX_ENABLE_GUI
                if(ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendPlatformUserData != nullptr)
                    ImGui_ImplWin32_WndProcHandler(gfx_window->window_, message, w_param, l_param);
#   endif
                break;
            default:
                break;
            }
        }
        return DefWindowProc(window, message, w_param, l_param);
    }
};

GfxWindow gfxCreateWindow(uint32_t window_width, uint32_t window_height, char const *window_title, GfxCreateWindowFlags flags)
{
    GfxResult result;
    GfxWindow window = {};
    GfxWindowInternal *gfx_window = new GfxWindowInternal(window);
    if(!gfx_window) return window;  // out of memory
    result = gfx_window->initialize(window, window_width, window_height, window_title, flags);
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

bool gfxWindowIsKeyDown(GfxWindow window, uint32_t key_code)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return false;   // invalid window handle
    return gfx_window->getIsKeyDown(key_code);
}

bool gfxWindowIsKeyPressed(GfxWindow window, uint32_t key_code)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return false;   // invalid window handle
    return gfx_window->getIsKeyPressed(key_code);
}

bool gfxWindowIsKeyReleased(GfxWindow window, uint32_t key_code)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return false;   // invalid window handle
    return gfx_window->getIsKeyReleased(key_code);
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

bool gfxWindowRegisterDropCallback(GfxWindow window, void (*callback)(char const *, uint32_t, void *), void *data)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return false; // invalid window handle
    gfx_window->registerDropCallback(callback, data);
    return true;
}

bool gfxWindowUnregisterDropCallback(GfxWindow window)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return false; // invalid window handle
    gfx_window->unregisterDropCallback();
    return true;
}

float gfxWindowGetDPIScale(GfxWindow window)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return 1.0f; // invalid window handle
    return gfx_window->getDPIScale();
}

void gfxWindowToggleFullscreen(GfxWindow window)
{
    GfxWindowInternal *gfx_window = GfxWindowInternal::GetGfxWindow(window);
    if(!gfx_window) return; // invalid window handle
    gfx_window->toggleFullscreen();
}
