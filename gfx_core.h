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

#ifndef GFX_INCLUDE_GFX_CORE_H
#define GFX_INCLUDE_GFX_CORE_H

#include <cstdarg>      // va_list, etc.
#include <cstdint>      // uint32_t, etc.
#include <algorithm>    // std::min(), std::max()

//!
//! Constants.
//!

enum GfxResult
{
    kGfxResult_NoError = 0,
    kGfxResult_InvalidParameter,
    kGfxResult_InvalidOperation,
    kGfxResult_OutOfMemory,
    kGfxResult_InternalError,
    kGfxResult_DeviceError,

    kGfxResult_Count
};

enum GfxConstant
{
    kGfxConstant_BackBufferCount  = 3,
    kGfxConstant_MaxRenderTarget  = 8,
    kGfxConstant_MaxAnisotropy    = 8,
    kGfxConstant_MaxNameLength    = 64,
    kGfxConstant_NumBindlessSlots = 1024
};

//!
//! Public macros.
//!

#define GFX_ASSERT(X)   // skip it

#define GFX_ASSERTMSG(X, ...)

#define GFX_MIN(X, Y)   ((std::min)(X, Y))

#define GFX_MAX(X, Y)   ((std::max)(X, Y))

#define GFX_SNPRINTF(BUFFER, SIZE, FORMAT, ...) \
    _snprintf_s(BUFFER, SIZE, _TRUNCATE, FORMAT, __VA_ARGS__)

#define GFX_NON_COPYABLE(TYPE)  \
    TYPE(TYPE const &) = delete; TYPE &operator =(TYPE const &) = delete

#define GFX_ALIGN(VAL, ALIGN)   \
    (((VAL) + (static_cast<decltype(VAL)>(ALIGN) - 1)) & ~(static_cast<decltype(VAL)>(ALIGN) - 1))

#define GFX_BREAKPOINT          \
    GFX_MULTI_LINE_MACRO_BEGIN  \
        if(IsDebuggerPresent()) \
            DebugBreak();       \
    GFX_MULTI_LINE_MACRO_END

#define GFX_TRY(X)                                                          \
    GFX_MULTI_LINE_MACRO_BEGIN                                              \
        GfxResult const _res = (GfxResult)(X);                              \
        if(_res != kGfxResult_NoError)                                      \
            return GFX_SET_ERROR(_res, "`%s' failed", GFX_STRINGIFY(X));    \
    GFX_MULTI_LINE_MACRO_END

#define GFX_PRINTLN(...)                                    \
    GFX_MULTI_LINE_MACRO_BEGIN                              \
        GFX_PRINTLN_IMPL(__FILE__, __LINE__, __VA_ARGS__);  \
    GFX_MULTI_LINE_MACRO_END

#define GFX_SET_ERROR(RESULT, ...)  \
    GFX_SET_ERROR_IMPL(RESULT, __FILE__, __LINE__, __VA_ARGS__)

#define GFX_PRINT_ERROR(RESULT, ...)                                    \
    GFX_MULTI_LINE_MACRO_BEGIN                                          \
        GFX_PRINT_ERROR_IMPL(RESULT, __FILE__, __LINE__, __VA_ARGS__);  \
    GFX_MULTI_LINE_MACRO_END

//!
//! Debug macros.
//!

#ifdef _DEBUG

#undef GFX_ASSERT
#define GFX_ASSERT(X)                       \
    GFX_MULTI_LINE_MACRO_BEGIN              \
        if(!(X))                            \
        {                                   \
            GFX_BREAKPOINT;                 \
            exit(kGfxResult_InternalError); \
        }                                   \
    GFX_MULTI_LINE_MACRO_END

#undef GFX_ASSERTMSG
#define GFX_ASSERTMSG(X, ...)                                   \
    GFX_MULTI_LINE_MACRO_BEGIN                                  \
        if(!(X))                                                \
        {                                                       \
            GFX_PRINTLN_IMPL(__FILE__, __LINE__, __VA_ARGS__);  \
            GFX_BREAKPOINT;                                     \
        }                                                       \
    GFX_MULTI_LINE_MACRO_END

#endif //! _DEBUG

//!
//! Internal macros.
//!

#define GFX_MULTI_LINE_MACRO_BEGIN                                              \
    __pragma(warning(push))                                                     \
    __pragma(warning(disable:4127)) /* conditional expression is constant */    \
    __pragma(warning(disable:4390)) /* empty controlled statement found   */    \
    do                                                                          \
    {

#define GFX_MULTI_LINE_MACRO_END    \
    }                               \
    while(0)                        \
    __pragma(warning(pop))

#define GFX_STRINGIFY(X)    GFX_STRINGIFY2(X)

#define GFX_STRINGIFY2(X)   #X

#define GFX_INTERNAL_HANDLE(TYPE)       friend class GfxInternal; public: inline TYPE() { memset(this, 0, sizeof(*this)); }         \
                                        inline bool operator ==(TYPE const &other) const { return handle == other.handle; }         \
                                        inline bool operator !=(TYPE const &other) const { return handle != other.handle; }         \
                                        inline uint32_t getIndex() const { return (uint32_t)(handle ? handle & 0xFFFFFFFFull        \
                                                                                                    : 0xFFFFFFFFull); }             \
                                        inline operator uint32_t() const { return getIndex(); }                                     \
                                        inline operator bool() const { return !!handle; }                                           \
                                        private: uint64_t handle

#define GFX_INTERNAL_NAMED_HANDLE(TYPE) friend class GfxInternal; public: inline TYPE() { memset(this, 0, sizeof(*this)); }         \
                                        inline bool operator ==(TYPE const &other) const { return handle == other.handle; }         \
                                        inline bool operator !=(TYPE const &other) const { return handle != other.handle; }         \
                                        inline uint32_t getIndex() const { return (uint32_t)(handle ? handle & 0xFFFFFFFFull        \
                                                                                                    : 0xFFFFFFFFull); }             \
                                        inline operator uint32_t() const { return getIndex(); }                                     \
                                        inline char const *getName() const { return name; }                                         \
                                        inline void setName(char const *n) { uint32_t i = 0;                                        \
                                                                             if(n)                                                  \
                                                                               for(; n[i] && i < kGfxConstant_MaxNameLength; ++i)   \
                                                                                 name[i] = n[i]; name[i] = '\0'; }                  \
                                        inline operator bool() const { return !!handle; }                                           \
                                        private: uint64_t handle; char name[kGfxConstant_MaxNameLength + 1]

char const *gfxResultGetString(GfxResult result);

void GFX_PRINTLN_IMPL(char const *file_name, uint32_t line_number, char const *format, ...);

void GFX_PRINT_ERROR_IMPL(
    GfxResult result, char const *file_name, uint32_t line_number, char const *format, ...);

GfxResult GFX_SET_ERROR_IMPL(
    GfxResult result, char const *file_name, uint32_t line_number, char const *format, ...);

#endif //! GFX_INCLUDE_GFX_CORE_H
