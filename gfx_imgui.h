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
#if !defined(GFX_INCLUDE_GFX_IMGUI_H) && defined(GFX_ENABLE_GUI)
#define GFX_INCLUDE_GFX_IMGUI_H

// Required to support unicode values above 0x10000: https://github.com/gboisse/gfx/pull/71
#define IMGUI_USE_WCHAR32
#define IMGUI_DEFINE_MATH_OPERATORS

#include "gfx.h"
#include "imgui.h"

//!
//! ImGui initialization/termination.
//!

GfxResult gfxImGuiInitialize(GfxContext gfx, char const **font_filenames = nullptr, uint32_t font_count = 0,
    ImFontConfig const *font_configs = nullptr, ImGuiConfigFlags flags = 0);
GfxResult gfxImGuiTerminate();
GfxResult gfxImGuiRender();

bool gfxImGuiIsInitialized();

#endif //! GFX_INCLUDE_GFX_IMGUI_H

//!
//! Implementation details.
//!

#ifdef GFX_IMPLEMENTATION_DEFINE

#pragma once
#include "gfx_imgui.cpp"

#endif //! GFX_IMPLEMENTATION_DEFINE
