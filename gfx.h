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

#include "d3d12.h"
#include "dxgi1_4.h"
#include "gfx_core.h"

//!
//! Context creation/destruction.
//!

class GfxContext { friend class GfxInternal; uint64_t handle; char name[kGfxConstant_MaxNameLength + 1]; uint32_t vendor_id; public:
                   inline bool operator ==(GfxContext const &other) const  { return handle == other.handle; }
                   inline bool operator !=(GfxContext const &other) const  { return handle != other.handle; }
                   inline GfxContext() { memset(this, 0, sizeof(*this)); }
                   inline uint32_t getVendorId() const { return vendor_id; }
                   inline char const *getName() const { return name; }
                   inline operator bool() const { return !!handle; } };

enum GfxCreateContextFlag
{
    kGfxCreateContextFlag_EnableDebugLayer = 1 << 0
};
typedef uint32_t GfxCreateContextFlags;

GfxContext gfxCreateContext(HWND window, GfxCreateContextFlags flags = 0);
GfxResult gfxDestroyContext(GfxContext context);

uint32_t gfxGetBackBufferWidth(GfxContext context);
uint32_t gfxGetBackBufferHeight(GfxContext context);
uint32_t gfxGetBackBufferIndex(GfxContext context);

//!
//! Buffer resources.
//!

enum GfxCpuAccess
{
    kGfxCpuAccess_None = 0, // No CPU access, fastest path for GPU read & write memory.
    kGfxCpuAccess_Read,     // CPU can read but not write, GPU can copy to; ideal for CPU readbacks.
    kGfxCpuAccess_Write,    // CPU can write but not read, GPU can only read (ideally not too often); ideal for constants & uploads.

    kGfxCpuAccess_Count
};

class GfxBuffer { GFX_INTERNAL_NAMED_HANDLE(GfxBuffer); uint64_t size; uint32_t stride; GfxCpuAccess cpu_access; public:
                  inline uint32_t getCount() const { return (uint32_t)(!stride ? 0 : size / stride); }
                  inline GfxCpuAccess getCpuAccess() const { return cpu_access; }
                  inline void setStride(uint32_t stride_) { stride = stride_; }
                  inline uint32_t getStride() const { return stride; }
                  inline uint64_t getSize() const { return size; } };

GfxBuffer gfxCreateBuffer(GfxContext context, uint64_t size, void const *data = nullptr, GfxCpuAccess cpu_access = kGfxCpuAccess_None);
GfxBuffer gfxCreateBufferRange(GfxContext context, GfxBuffer buffer, uint64_t byte_offset, uint64_t size);  // fast path for (sub-)allocating during a frame
GfxResult gfxDestroyBuffer(GfxContext context, GfxBuffer buffer);

void *gfxBufferGetData(GfxContext context, GfxBuffer buffer);

//!
//! Template helpers.
//!

template<typename TYPE>
GfxBuffer gfxCreateBuffer(GfxContext context, uint32_t element_count, void const *data = nullptr, GfxCpuAccess cpu_access = kGfxCpuAccess_None)
{
    GfxBuffer buffer = gfxCreateBuffer(context, element_count * sizeof(TYPE), data, cpu_access);
    if(buffer) buffer.setStride((uint32_t)sizeof(TYPE));
    return buffer;
}

template<typename TYPE>
GfxBuffer gfxCreateBufferRange(GfxContext context, GfxBuffer buffer, uint32_t element_offset, uint32_t element_count)
{
    GfxBuffer buffer_range = gfxCreateBufferRange(context, buffer, element_offset * sizeof(TYPE), element_count * sizeof(TYPE));
    if(buffer_range) buffer_range.setStride((uint32_t)sizeof(TYPE));
    return buffer_range;
}

template<typename TYPE>
TYPE *gfxBufferGetData(GfxContext context, GfxBuffer buffer)
{
    return (TYPE *)gfxBufferGetData(context, buffer);
}

//!
//! Texture resources.
//!

class GfxTexture { GFX_INTERNAL_NAMED_HANDLE(GfxTexture); uint32_t width; uint32_t height; uint32_t depth; DXGI_FORMAT format; uint32_t mip_levels; enum { kType_2D, kType_2DArray, kType_3D, kType_Cube } type; public:
                   inline bool is2DArray() const { return type == kType_2DArray; }
                   inline bool isCube() const { return type == kType_Cube; }
                   inline bool is3D() const { return type == kType_3D; }
                   inline bool is2D() const { return type == kType_2D; }
                   inline uint32_t getWidth() const { return width; }
                   inline uint32_t getHeight() const { return height; }
                   inline uint32_t getDepth() const { return depth; }
                   inline DXGI_FORMAT getFormat() const { return format; }
                   inline uint32_t getMipLevels() const { return mip_levels; } };

GfxTexture gfxCreateTexture2D(GfxContext context, DXGI_FORMAT format);  // creates auto-resize window-sized texture
GfxTexture gfxCreateTexture2D(GfxContext context, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t mip_levels = 1);
GfxTexture gfxCreateTexture2DArray(GfxContext context, uint32_t width, uint32_t height, uint32_t slice_count, DXGI_FORMAT format, uint32_t mip_levels = 1);
GfxTexture gfxCreateTexture3D(GfxContext context, uint32_t width, uint32_t height, uint32_t depth, DXGI_FORMAT format, uint32_t mip_levels = 1);
GfxTexture gfxCreateTextureCube(GfxContext context, uint32_t size, DXGI_FORMAT format, uint32_t mip_levels = 1);
GfxResult gfxDestroyTexture(GfxContext context, GfxTexture texture);

//!
//! Helper functions.
//!

inline uint32_t gfxCalculateMipCount(uint32_t width, uint32_t height = 0, uint32_t depth = 0)
{
    uint32_t mip_count = 0;
    uint32_t mip_size = GFX_MAX(width, GFX_MAX(height, depth));
    while(mip_size >= 1) { mip_size >>= 1; ++mip_count; }
    return mip_count;
}

//!
//! Sampler states.
//!

class GfxSamplerState { GFX_INTERNAL_HANDLE(GfxSamplerState); D3D12_FILTER filter; D3D12_TEXTURE_ADDRESS_MODE address_u; D3D12_TEXTURE_ADDRESS_MODE address_v; D3D12_TEXTURE_ADDRESS_MODE address_w; float mip_lod_bias; uint32_t max_anisotropy; D3D12_COMPARISON_FUNC comparison_func; float min_lod; float max_lod; public:
                        inline D3D12_FILTER getFilter() const { return filter; }
                        inline D3D12_TEXTURE_ADDRESS_MODE getAddressU() const { return address_u; }
                        inline D3D12_TEXTURE_ADDRESS_MODE getAddressV() const { return address_v; }
                        inline D3D12_TEXTURE_ADDRESS_MODE getAddressW() const { return address_w; }
                        inline float getMipLodBias() const { return mip_lod_bias; }
                        inline uint32_t getMaxAnisotropy() const { return max_anisotropy; }
                        inline D3D12_COMPARISON_FUNC getComparisonFunc() const { return comparison_func; }
                        inline float getMinLod() const { return min_lod; }
                        inline float getMaxLod() const { return max_lod; } };

GfxSamplerState gfxCreateSamplerState(GfxContext context, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address_u = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_v = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_w = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               float mip_lod_bias = 0.0f, float min_lod = 0.0f, float max_lod = (float)D3D12_REQ_MIP_LEVELS);
GfxSamplerState gfxCreateSamplerState(GfxContext context, D3D12_FILTER filter, D3D12_COMPARISON_FUNC comparison_func,   // for comparison samplers
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_u = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_v = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_w = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               float mip_lod_bias = 0.0f, float min_lod = 0.0f, float max_lod = (float)D3D12_REQ_MIP_LEVELS);
GfxResult gfxDestroySamplerState(GfxContext context, GfxSamplerState sampler_state);

//!
//! Acceleration structures.
//!

class GfxRaytracingPrimitive;   // forward declaration

class GfxAccelerationStructure { GFX_INTERNAL_NAMED_HANDLE(GfxAccelerationStructure); public: };

GfxAccelerationStructure gfxCreateAccelerationStructure(GfxContext context);
GfxResult gfxDestroyAccelerationStructure(GfxContext context, GfxAccelerationStructure acceleration_structure); // automatically releases all raytracing primitives

GfxResult gfxAccelerationStructureUpdate(GfxContext context, GfxAccelerationStructure acceleration_structure);
uint32_t gfxAccelerationStructureGetRaytracingPrimitiveCount(GfxContext context, GfxAccelerationStructure acceleration_structure);
GfxRaytracingPrimitive const *gfxAccelerationStructureGetRaytracingPrimitives(GfxContext context, GfxAccelerationStructure acceleration_structure);

//!
//! Raytracing primitives.
//!

class GfxRaytracingPrimitive { GFX_INTERNAL_NAMED_HANDLE(GfxRaytracingPrimitive); public: };

GfxRaytracingPrimitive gfxCreateRaytracingPrimitive(GfxContext context, GfxAccelerationStructure acceleration_structure);
GfxResult gfxDestroyRaytracingPrimitive(GfxContext context, GfxRaytracingPrimitive raytracing_primitive);

GfxResult gfxRaytracingPrimitiveBuild(GfxContext context, GfxRaytracingPrimitive raytracing_primitive, GfxBuffer vertex_buffer, uint32_t vertex_stride = 0);
GfxResult gfxRaytracingPrimitiveBuild(GfxContext context, GfxRaytracingPrimitive raytracing_primitive, GfxBuffer index_buffer, GfxBuffer vertex_buffer, uint32_t vertex_stride = 0);
GfxResult gfxRaytracingPrimitiveSetTransform(GfxContext context, GfxRaytracingPrimitive raytracing_primitive, float const *row_major_4x4_transform);
GfxResult gfxRaytracingPrimitiveUpdate(GfxContext context, GfxRaytracingPrimitive raytracing_primitive);

//!
//! Draw state manipulation.
//!

class GfxDrawState { friend class GfxInternal; uint64_t handle; public: GfxDrawState(); GfxDrawState(GfxDrawState const &); GfxDrawState &operator =(GfxDrawState const &); ~GfxDrawState(); };

GfxResult gfxDrawStateSetColorTarget(GfxDrawState draw_state, uint32_t target_index, GfxTexture texture, uint32_t mip_level = 0, uint32_t slice = 0);
GfxResult gfxDrawStateSetDepthStencilTarget(GfxDrawState draw_state, GfxTexture texture, uint32_t mip_level = 0, uint32_t slice = 0);

GfxResult gfxDrawStateSetCullMode(GfxDrawState draw_state, D3D12_CULL_MODE cull_mode);
GfxResult gfxDrawStateSetFillMode(GfxDrawState draw_state, D3D12_FILL_MODE fill_mode);
GfxResult gfxDrawStateSetBlendMode(GfxDrawState draw_state, D3D12_BLEND src_blend, D3D12_BLEND dst_blend, D3D12_BLEND_OP blend_op, D3D12_BLEND src_blend_alpha, D3D12_BLEND dst_blend_alpha, D3D12_BLEND_OP blend_op_alpha);

//!
//! Blend mode helpers.
//!

inline GfxResult gfxDrawStateEnableAlphaBlending(GfxDrawState draw_state)
{
    return gfxDrawStateSetBlendMode(draw_state, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD);
}

//!
//! Program creation/destruction.
//!

class GfxProgram { GFX_INTERNAL_HANDLE(GfxProgram); char name[kGfxConstant_MaxNameLength + 1]; public:
                   inline char const *getName() const { return name; } };

class GfxProgramDesc { public: inline GfxProgramDesc() : cs(nullptr), vs(nullptr), gs(nullptr), ps(nullptr) {}
                       char const *cs;
                       char const *vs;
                       char const *gs;
                       char const *ps; };

GfxProgram gfxCreateProgram(GfxContext context, char const *file_name, char const *file_path = nullptr);
GfxProgram gfxCreateProgram(GfxContext context, GfxProgramDesc program_desc, char const *name = nullptr);
GfxResult gfxDestroyProgram(GfxContext context, GfxProgram program);

GfxResult gfxProgramSetBuffer(GfxContext context, GfxProgram program, char const *parameter_name, GfxBuffer buffer);
GfxResult gfxProgramSetTexture(GfxContext context, GfxProgram program, char const *parameter_name, GfxTexture texture, uint32_t mip_level = 0);
GfxResult gfxProgramSetTextures(GfxContext context, GfxProgram program, char const *parameter_name, GfxTexture const *textures, uint32_t texture_count);
GfxResult gfxProgramSetSamplerState(GfxContext context, GfxProgram program, char const *parameter_name, GfxSamplerState sampler_state);
GfxResult gfxProgramSetAccelerationStructure(GfxContext context, GfxProgram program, char const *parameter_name, GfxAccelerationStructure acceleration_structure);
GfxResult gfxProgramSetConstants(GfxContext context, GfxProgram program, char const *parameter_name, void const *data, uint32_t data_size);

//!
//! Template helpers.
//!

template<typename TYPE>
GfxResult gfxProgramSetParameter(GfxContext context, GfxProgram program, char const *parameter_name, TYPE const &value);

template<> inline GfxResult gfxProgramSetParameter<GfxBuffer>(GfxContext context, GfxProgram program, char const *parameter_name, GfxBuffer const &value) { return gfxProgramSetBuffer(context, program, parameter_name, value); }
template<> inline GfxResult gfxProgramSetParameter<GfxTexture>(GfxContext context, GfxProgram program, char const *parameter_name, GfxTexture const &value) { return gfxProgramSetTexture(context, program, parameter_name, value); }
template<> inline GfxResult gfxProgramSetParameter<GfxSamplerState>(GfxContext context, GfxProgram program, char const *parameter_name, GfxSamplerState const &value) { return gfxProgramSetSamplerState(context, program, parameter_name, value); }
template<> inline GfxResult gfxProgramSetParameter<GfxAccelerationStructure>(GfxContext context, GfxProgram program, char const *parameter_name, GfxAccelerationStructure const &value) { return gfxProgramSetAccelerationStructure(context, program, parameter_name, value); }
template<typename TYPE> inline GfxResult gfxProgramSetParameter(GfxContext context, GfxProgram program, char const *parameter_name, TYPE const &value)
{
    static_assert(!std::is_pointer<TYPE>::value, "Program parameters must be passed by reference, not by pointer");
    return gfxProgramSetConstants(context, program, parameter_name, &value, sizeof(value));
}

//!
//! Kernel compilation.
//!

class GfxKernel { GFX_INTERNAL_HANDLE(GfxKernel); char name[kGfxConstant_MaxNameLength + 1]; enum { kType_Compute, kType_Graphics } type; public:
                  inline bool isGraphics() const { return type == kType_Graphics; }
                  inline bool isCompute() const { return type == kType_Compute; }
                  inline char const *getName() const { return name; } };

GfxKernel gfxCreateComputeKernel(GfxContext context, GfxProgram program, char const *entry_point = nullptr, char const **defines = nullptr, uint32_t define_count = 0);
GfxKernel gfxCreateGraphicsKernel(GfxContext context, GfxProgram program, char const *entry_point = nullptr, char const **defines = nullptr, uint32_t define_count = 0);    // draws to back buffer
GfxKernel gfxCreateGraphicsKernel(GfxContext context, GfxProgram program, GfxDrawState draw_state, char const *entry_point = nullptr, char const **defines = nullptr, uint32_t define_count = 0);
GfxResult gfxDestroyKernel(GfxContext context, GfxKernel kernel);

uint32_t const *gfxKernelGetNumThreads(GfxContext context, GfxKernel kernel);
GfxResult gfxKernelReloadAll(GfxContext context);   // hot-reloads all kernel objects

//!
//! Command encoding.
//!

GfxResult gfxCommandCopyBuffer(GfxContext context, GfxBuffer dst, GfxBuffer src);
GfxResult gfxCommandCopyBuffer(GfxContext context, GfxBuffer dst, uint64_t dst_offset, GfxBuffer src, uint64_t src_offset, uint64_t size);
GfxResult gfxCommandClearBuffer(GfxContext context, GfxBuffer buffer, uint32_t clear_value = 0);

GfxResult gfxCommandClearBackBuffer(GfxContext context);
GfxResult gfxCommandClearTexture(GfxContext context, GfxTexture texture);
GfxResult gfxCommandCopyTexture(GfxContext context, GfxTexture dst, GfxTexture src);
GfxResult gfxCommandClearImage(GfxContext context, GfxTexture texture, uint32_t mip_level = 0, uint32_t slice = 0);

GfxResult gfxCommandCopyBufferToTexture(GfxContext context, GfxTexture dst, GfxBuffer src);
GfxResult gfxCommandGenerateMips(GfxContext context, GfxTexture texture);   // expects mip level 0 to be populated, generates the others

GfxResult gfxCommandBindKernel(GfxContext context, GfxKernel kernel);
GfxResult gfxCommandBindIndexBuffer(GfxContext context, GfxBuffer index_buffer);
GfxResult gfxCommandBindVertexBuffer(GfxContext context, GfxBuffer vertex_buffer);

GfxResult gfxCommandSetViewport(GfxContext context, float x = 0.0f, float y = 0.0f, float width = 0.0f, float height = 0.0f);   // leave at 0 to draw to render target/back buffer size
GfxResult gfxCommandSetScissorRect(GfxContext context, int32_t x = 0, int32_t y = 0, int32_t width = 0, int32_t height = 0);    // leave at 0 to draw to render target/back buffer size

GfxResult gfxCommandDraw(GfxContext context, uint32_t vertex_count, uint32_t instance_count = 1, uint32_t base_vertex = 0, uint32_t base_instance = 0);
GfxResult gfxCommandDrawIndexed(GfxContext context, uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, uint32_t base_vertex = 0, uint32_t base_instance = 0);
GfxResult gfxCommandMultiDrawIndirect(GfxContext context, GfxBuffer args_buffer, uint32_t args_count);          // expects a buffer of D3D12_DRAW_ARGUMENTS elements
GfxResult gfxCommandMultiDrawIndexedIndirect(GfxContext context, GfxBuffer args_buffer, uint32_t args_count);   // expects a buffer of D3D12_DRAW_INDEXED_ARGUMENTS elements
GfxResult gfxCommandDispatch(GfxContext context, uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z);
GfxResult gfxCommandDispatchIndirect(GfxContext context, GfxBuffer args_buffer);    // expects a buffer of D3D12_DISPATCH_ARGUMENTS elements

//!
//! Debug/profile API.
//!

class GfxTimestampQuery { GFX_INTERNAL_HANDLE(GfxTimestampQuery); public: };

GfxTimestampQuery gfxCreateTimestampQuery(GfxContext context);
GfxResult gfxDestroyTimestampQuery(GfxContext context, GfxTimestampQuery timestamp_query);
float gfxTimestampQueryGetDuration(GfxContext context, GfxTimestampQuery timestamp_query);  // in milliseconds
GfxResult gfxCommandBeginTimestampQuery(GfxContext context, GfxTimestampQuery timestamp_query);
GfxResult gfxCommandEndTimestampQuery(GfxContext context, GfxTimestampQuery timestamp_query);

GfxResult gfxCommandBeginEvent(GfxContext context, char const *format, ...);
GfxResult gfxCommandBeginEvent(GfxContext context, uint64_t color, char const *format, ...);
GfxResult gfxCommandEndEvent(GfxContext context);

//!
//! RAII helpers.
//!

class GfxTimedSection { GFX_NON_COPYABLE(GfxTimedSection); GfxContext context; GfxTimestampQuery timestamp_query; char const *section_name; public:
                        inline GfxTimedSection(GfxContext context, GfxTimestampQuery timestamp_query, char const *section_name = nullptr) : context(context), timestamp_query(timestamp_query), section_name(section_name) { gfxCommandBeginTimestampQuery(context, timestamp_query); }
                        inline ~GfxTimedSection() { gfxCommandEndTimestampQuery(context, timestamp_query); if(section_name != nullptr) GFX_PRINTLN("Completed `%s' in %.3fms", section_name, gfxTimestampQueryGetDuration(context, timestamp_query)); } };

class GfxCommandEvent { GFX_NON_COPYABLE(GfxCommandEvent); GfxContext context; public:
                        inline GfxCommandEvent(GfxContext context, char const *format, ...) : context(context) { va_list args; va_start(args, format); char buffer[4096]; vsnprintf(buffer, sizeof(buffer), format, args); va_end(args); gfxCommandBeginEvent(context, buffer); }
                        inline GfxCommandEvent(GfxContext context, uint64_t color, char const *format, ...) : context(context) { va_list args; va_start(args, format); char buffer[4096]; vsnprintf(buffer, sizeof(buffer), format, args); va_end(args); gfxCommandBeginEvent(context, color, buffer); }
                        inline ~GfxCommandEvent() { gfxCommandEndEvent(context); } };

//!
//! Parallel primitives.
//!

enum GfxDataType
{
    kGfxDataType_Int = 0,
    kGfxDataType_Uint,
    kGfxDataType_Float,

    kGfxDataType_Count
};

GfxResult gfxCommandScanMin(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count = nullptr); // dst can be the same the src for all parallel primitives
GfxResult gfxCommandScanMax(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandScanSum(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandReduceMin(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandReduceMax(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandReduceSum(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandSortRadix(GfxContext context, GfxBuffer keys_dst, GfxBuffer keys_src, GfxBuffer const *values_dst = nullptr, GfxBuffer const *values_src = nullptr, GfxBuffer const *count = nullptr);

//!
//! Frame processing.
//!

GfxResult gfxFrame(GfxContext context, bool vsync = true);
GfxResult gfxFinish(GfxContext context);

//!
//! Implementation details.
//!

#ifdef GFX_IMPLEMENTATION_DEFINE

#include <map>              // std::map
#include <deque>            // std::deque
#include "dxcapi.h"         // shader compiler
#include "d3d12shader.h"    // shader reflection

#pragma warning(push)
#pragma warning(disable:4100)   // unreferenced formal parameter
#pragma warning(disable:4127)   // conditional expression is constant
#pragma warning(disable:4189)   // local variable is initialized but not referenced
#pragma warning(disable:4211)   // nonstandard extension used: redefined extern to static
#include "D3D12MemAlloc.cpp"    // D3D12MemoryAllocator
#include "WinPixEventRuntime/pix3.h"
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:4996)   // this function or variable may be unsafe

class GfxInternal
{
    GFX_NON_COPYABLE(GfxInternal);

    HWND window_ = {};
    uint32_t window_width_ = 0;
    uint32_t window_height_ = 0;
    bool raytracing_support_ = false;

    ID3D12Device5 *device_ = nullptr;
    ID3D12CommandQueue *command_queue_ = nullptr;
    ID3D12GraphicsCommandList4 *command_list_ = nullptr;
    ID3D12CommandAllocator *command_allocators_[kGfxConstant_BackBufferCount] = {};

    HANDLE fence_event_ = {};
    uint32_t fence_index_ = 0;
    ID3D12Fence *fences_[kGfxConstant_BackBufferCount] = {};
    uint64_t fence_values_[kGfxConstant_BackBufferCount] = {};

    IDxcUtils *dxc_utils_ = nullptr;
    IDxcCompiler3 *dxc_compiler_ = nullptr;
    IDxcIncludeHandler *dxc_include_handler_ = nullptr;

    IDXGISwapChain3 *swap_chain_ = nullptr;
    D3D12MA::Allocator *mem_allocator_ = nullptr;
    ID3D12CommandSignature *dispatch_signature_ = nullptr;
    ID3D12CommandSignature *multi_draw_signature_ = nullptr;
    ID3D12CommandSignature *multi_draw_indexed_signature_ = nullptr;
    std::vector<D3D12_RESOURCE_BARRIER> resource_barriers_;
    ID3D12Resource *back_buffers_[kGfxConstant_BackBufferCount] = {};
    uint32_t back_buffer_rtvs_[kGfxConstant_BackBufferCount] = {};

    GfxKernel bound_kernel_ = {};
    GfxBuffer draw_id_buffer_ = {};
    uint64_t descriptor_heap_id_ = 0;
    GfxBuffer bound_index_buffer_ = {};
    GfxBuffer bound_vertex_buffer_ = {};
    GfxBuffer texture_upload_buffer_ = {};
    GfxBuffer installed_index_buffer_ = {};
    GfxBuffer installed_vertex_buffer_ = {};
    bool force_install_index_buffer_ = false;
    bool force_install_vertex_buffer_ = false;
    GfxBuffer raytracing_scratch_buffer_ = {};
    std::vector<GfxRaytracingPrimitive> active_raytracing_primitives_;
    GfxBuffer constant_buffer_pool_[kGfxConstant_BackBufferCount] = {};
    uint64_t constant_buffer_pool_cursors_[kGfxConstant_BackBufferCount] = {};

    struct Viewport
    {
        inline Viewport &operator =(D3D12_VIEWPORT const &other)
        {
            x_ = other.TopLeftX;
            y_ = other.TopLeftY;
            width_ = other.Width;
            height_ = other.Height;

            return *this;
        }

        inline bool operator !=(D3D12_VIEWPORT const &other) const
        {
            return (x_ != other.TopLeftX || y_ != other.TopLeftY || width_ != other.Width || height_ != other.Height);
        }

        inline void invalidate()
        {
            width_ = NAN;
            height_ = NAN;
        }

        float x_ = 0.0f;
        float y_ = 0.0f;
        float width_ = 0.0f;
        float height_ = 0.0f;
    };
    Viewport viewport_;
    Viewport bound_viewport_;

    struct ScissorRect
    {
        inline ScissorRect &operator =(D3D12_RECT const &other)
        {
            x_ = (int32_t)other.left;
            y_ = (int32_t)other.top;
            width_ = (int32_t)(other.right - other.left);
            height_ = (int32_t)(other.bottom - other.top);

            return *this;
        }

        inline bool operator !=(D3D12_RECT const &other) const
        {
            return (x_ != other.left || y_ != other.top || x_ + width_ != other.right || y_ + height_ != other.bottom);
        }

        inline void invalidate()
        {
            width_ = -1;
            height_ = -1;
        }

        int32_t x_ = 0;
        int32_t y_ = 0;
        int32_t width_ = 0;
        int32_t height_ = 0;
    };
    ScissorRect scissor_rect_;
    ScissorRect bound_scissor_rect_;

    GfxKernel clear_buffer_kernel_ = {};
    GfxProgram clear_buffer_program_ = {};
    bool issued_clear_buffer_warning_ = false;

    GfxKernel generate_mips_kernel_ = {};
    GfxProgram generate_mips_program_ = {};

    struct ScanKernels
    {
        GfxProgram scan_program_ = {};
        GfxKernel reduce_kernel_ = {};
        GfxKernel scan_add_kernel_ = {};
        GfxKernel scan_kernel_ = {};
        GfxKernel args_kernel_ = {};
    };
    std::map<uint32_t, ScanKernels> scan_kernels_;

    struct String
    {
        char *data_;
        GFX_NON_COPYABLE(String);
        inline String() : data_(nullptr) {}
        inline String(char const *data) : data_(nullptr) { *this = data; }
        inline String(String &&other) : data_(other.data_) { other.data_ = nullptr; }
        inline String &operator =(String &&other) { if(this != &other) { free(data_); data_ = other.data_; other.data_ = nullptr; } return *this; }
        inline String &operator =(char const *data) { free(data_); if(!data) data_ = nullptr; else
                                                                           { data_ = (char *)malloc(strlen(data) + 1); strcpy(data_, data); }
                                                      return *this; }
        inline operator char const *() const { return data_ ? data_ : ""; }
        inline size_t size() const { return data_ ? strlen(data_) : 0; }
        inline char const *c_str() const { return data_ ? data_ : ""; }
        inline operator bool() const { return data_ != nullptr; }
        inline ~String() { free(data_); }
    };

    struct Garbage
    {
        GFX_NON_COPYABLE(Garbage);
        typedef void (*Collector)(Garbage const &garbage);
        inline Garbage() : deletion_counter_(kGfxConstant_BackBufferCount), garbage_collector_(nullptr) { memset(garbage_, 0, sizeof(garbage_)); }
        inline Garbage(Garbage &&other) : deletion_counter_(other.deletion_counter_), garbage_collector_(other.garbage_collector_) { memcpy(garbage_, other.garbage_, sizeof(garbage_)); other.garbage_collector_ = nullptr; }
        inline Garbage &operator =(Garbage &&other) { if(this != &other) { GFX_ASSERT(!garbage_collector_); if(garbage_collector_) garbage_collector_(*this); memcpy(garbage_, other.garbage_, sizeof(garbage_)); deletion_counter_ = other.deletion_counter_; garbage_collector_ = other.garbage_collector_; other.garbage_collector_ = nullptr; } return *this; }
        inline ~Garbage() { GFX_ASSERT(deletion_counter_ == 0xFFFFFFFFu || !garbage_collector_); if(garbage_collector_) garbage_collector_(*this); }

        template<typename TYPE>
        static void ResourceCollector(Garbage const &garbage)
        {
            TYPE *resource = (TYPE *)garbage.garbage_[0];
            GFX_ASSERT(resource != nullptr); if(resource == nullptr) return;
            resource->Release();    // release resource
        }

        static void DescriptorCollector(Garbage const &garbage)
        {
            uint32_t const descriptor_slot = (uint32_t)garbage.garbage_[0];
            GfxFreelist *descriptor_freelist = (GfxFreelist *)garbage.garbage_[1];
            GFX_ASSERT(descriptor_freelist != nullptr); if(descriptor_freelist == nullptr) return;
            descriptor_freelist->free_slot(descriptor_slot);    // release descriptor slot
        }

        uintptr_t garbage_[2];
        uint32_t deletion_counter_;
        Collector garbage_collector_;
    };
    std::deque<Garbage> garbage_collection_;

    struct DescriptorHeap
    {
        inline D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(uint32_t descriptor_slot) const
        {
            GFX_ASSERT(descriptor_slot < (descriptor_heap_ != nullptr ? descriptor_heap_->GetDesc().NumDescriptors : 0));
            D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
            cpu_handle.ptr += descriptor_slot * descriptor_handle_size_;
            return cpu_handle;
        }

        inline D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(uint32_t descriptor_slot) const
        {
            GFX_ASSERT(descriptor_slot < (descriptor_heap_ != nullptr ? descriptor_heap_->GetDesc().NumDescriptors : 0));
            D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = descriptor_heap_->GetGPUDescriptorHandleForHeapStart();
            gpu_handle.ptr += descriptor_slot * descriptor_handle_size_;
            return gpu_handle;
        }

        uint32_t descriptor_handle_size_ = 0;
        ID3D12DescriptorHeap *descriptor_heap_ = nullptr;
    };
    DescriptorHeap descriptors_;
    DescriptorHeap dsv_descriptors_;
    DescriptorHeap rtv_descriptors_;
    DescriptorHeap sampler_descriptors_;
    GfxFreelist freelist_descriptors_;
    GfxFreelist freelist_dsv_descriptors_;
    GfxFreelist freelist_rtv_descriptors_;
    GfxFreelist freelist_sampler_descriptors_;

    struct DrawState
    {
        struct RenderTarget
        {
            GfxTexture texture_ = {};
            uint32_t mip_level = 0;
            uint32_t slice = 0;
        };

        struct Data
        {
            RenderTarget color_targets_[kGfxConstant_MaxRenderTarget] = {};
            RenderTarget depth_stencil_target_ = {};
            struct
            {
                inline operator bool() const
                {
                    return (src_blend_ != 0 && dst_blend_ != 0 && blend_op_ != 0 && src_blend_alpha_ != 0 && dst_blend_alpha_ != 0 && blend_op_alpha_ != 0);
                }

                D3D12_BLEND src_blend_ = {};
                D3D12_BLEND dst_blend_ = {};
                D3D12_BLEND_OP blend_op_ = {};
                D3D12_BLEND src_blend_alpha_ = {};
                D3D12_BLEND dst_blend_alpha_ = {};
                D3D12_BLEND_OP blend_op_alpha_ = {};
            } blend_state_;
            struct
            {
                D3D12_CULL_MODE cull_mode_ = D3D12_CULL_MODE_BACK;
                D3D12_FILL_MODE fill_mode_ = D3D12_FILL_MODE_SOLID;
            } raster_state_;
        };

        DrawState() : reference_count_(0) {}
        DrawState(DrawState &&other) : draw_state_(other.draw_state_), reference_count_(other.reference_count_) { other.reference_count_ = 0; }
        ~DrawState() { GFX_ASSERT(reference_count_ == 0); }

        inline DrawState &operator =(DrawState &&other)
        {
            if(this != &other)
            {
                GFX_ASSERT(reference_count_ == 0);
                draw_state_ = other.draw_state_;
                reference_count_ = other.reference_count_;
                other.reference_count_ = 0;
            }
            return *this;
        }

        Data draw_state_;
        uint32_t reference_count_;
    };
    static GfxArray<DrawState> draw_states_;
    static GfxHandles draw_state_handles_;

    struct Object
    {
        enum Flag
        {
            kFlag_Named = 1 << 0
        };
        uint32_t flags_ = 0;
    };

    struct Buffer : public Object
    {
        void *data_ = nullptr;
        uint64_t data_offset_ = 0;
        ID3D12Resource *resource_ = nullptr;
        uint32_t *reference_count_ = nullptr;
        D3D12MA::Allocation *allocation_ = nullptr;
        D3D12_RESOURCE_STATES *resource_state_ = nullptr;
    };
    GfxArray<Buffer> buffers_;
    GfxHandles buffer_handles_;

    struct Texture : public Object
    {
        enum Flag
        {
            kFlag_AutoResize = 1 << 0
        };

        uint32_t flags_ = 0;
        ID3D12Resource *resource_ = nullptr;
        D3D12MA::Allocation *allocation_ = nullptr;
        std::vector<uint32_t> dsv_descriptor_slots_[D3D12_REQ_MIP_LEVELS];
        std::vector<uint32_t> rtv_descriptor_slots_[D3D12_REQ_MIP_LEVELS];
        D3D12_RESOURCE_STATES resource_state_ = D3D12_RESOURCE_STATE_COMMON;
    };
    GfxArray<Texture> textures_;
    GfxHandles texture_handles_;

    struct SamplerState
    {
        D3D12_SAMPLER_DESC sampler_desc_ = {};
        uint32_t descriptor_slot_ = 0xFFFFFFFFu;
    };
    GfxArray<SamplerState> sampler_states_;
    GfxHandles sampler_state_handles_;

    struct AccelerationStructure
    {
        bool needs_update_ = false;
        bool needs_rebuild_ = false;
        GfxBuffer bvh_buffer_ = {};
        std::vector<GfxRaytracingPrimitive> raytracing_primitives_;
    };
    GfxArray<AccelerationStructure> acceleration_structures_;
    GfxHandles acceleration_structure_handles_;

    struct RaytracingPrimitive
    {
        uint32_t index_ = 0;
        float transform_[16] = {};
        GfxBuffer bvh_buffer_ = {};
        uint32_t index_stride_ = 0;
        GfxBuffer index_buffer_ = {};
        uint32_t vertex_stride_ = 0;
        GfxBuffer vertex_buffer_ = {};
        GfxAccelerationStructure acceleration_structure = {};
    };
    GfxArray<RaytracingPrimitive> raytracing_primitives_;
    GfxHandles raytracing_primitive_handles_;

    struct Program
    {
        struct Parameter
        {
            enum Type
            {
                kType_Buffer = 0,
                kType_Image,
                kType_SamplerState,
                kType_AccelerationStructure,
                kType_Constants,

                kType_Count
            };

            String name_;
            uint32_t id_ = 0;
            Type type_ = kType_Count;
            union
            {
                GfxBuffer buffer_;
                struct { GfxTexture *textures_; uint32_t texture_count; uint32_t mip_level_; } image_;
                GfxSamplerState sampler_state_;
                struct { GfxAccelerationStructure bvh_; GfxBuffer bvh_buffer_; } acceleration_structure_;
                void *constants_;
            }
            data_ = {};
            uint32_t data_size_ = 0;

            void set(GfxBuffer const &buffer)
            {
                if(type_ == kType_Buffer)
                    id_ += (buffer.handle != data_.buffer_.handle || buffer.stride != data_.buffer_.stride);
                else
                {
                    unset();
                    type_ = kType_Buffer;
                    data_size_ = sizeof(buffer);
                }
                data_.buffer_ = buffer;
            }

            void set(GfxTexture const &texture, uint32_t mip_level)
            {
                if(type_ == kType_Image && data_.image_.texture_count == 1)
                    id_ += (texture.handle != data_.image_.textures_->handle || mip_level != data_.image_.mip_level_);
                else
                {
                    unset();
                    type_ = kType_Image;
                    data_size_ = sizeof(texture);
                    data_.image_.texture_count = 1;
                    data_.image_.textures_ = (GfxTexture *)malloc(sizeof(GfxTexture));
                    if(data_.image_.textures_ == nullptr)
                    {
                        GFX_ASSERT(0);  // out of memory
                        unset(); return;
                    }
                }
                *data_.image_.textures_ = texture;
                data_.image_.mip_level_ = mip_level;
            }

            void set(GfxTexture const *textures, uint32_t texture_count)
            {
                GFX_ASSERT(textures != nullptr && texture_count > 1);
                if(type_ == kType_Image && data_.image_.texture_count == texture_count)
                    for(uint32_t i = 0; i < texture_count; ++i) { if(textures[i].handle != data_.image_.textures_[i].handle) { ++id_; break; } }
                else
                {
                    unset();
                    type_ = kType_Image;
                    data_.image_.texture_count = texture_count;
                    data_size_ = texture_count * sizeof(*textures);
                    data_.image_.textures_ = (GfxTexture *)malloc(texture_count * sizeof(GfxTexture));
                    if(data_.image_.textures_ == nullptr)
                    {
                        GFX_ASSERT(0);  // out of memory
                        unset(); return;
                    }
                }
                for(uint32_t i = 0; i < texture_count; ++i) data_.image_.textures_[i] = textures[i];
            }

            void set(GfxSamplerState const &sampler_state)
            {
                if(type_ == kType_SamplerState)
                    id_ += (sampler_state.handle != data_.sampler_state_.handle);
                else
                {
                    unset();
                    type_ = kType_SamplerState;
                    data_size_ = sizeof(sampler_state);
                }
                data_.sampler_state_ = sampler_state;
            }

            void set(GfxAccelerationStructure const &acceleration_structure)
            {
                if(type_ == kType_AccelerationStructure)
                    id_ += (acceleration_structure.handle != data_.acceleration_structure_.bvh_.handle);
                else
                {
                    unset();
                    type_ = kType_AccelerationStructure;
                    data_size_ = sizeof(acceleration_structure);
                }
                data_.acceleration_structure_.bvh_ = acceleration_structure;
            }

            void set(void const *data, uint32_t data_size)
            {
                if(type_ == kType_Constants && data_size_ == data_size)
                    id_ += (memcmp(data, data_.constants_, data_size) != 0);
                else
                {
                    unset();
                    type_ = kType_Constants;
                    data_.constants_ = (data_size > 0 ? malloc(data_size) : nullptr);
                }
                memcpy(data_.constants_, data, data_size);
                data_size_ = data_size;
            }

            void unset()
            {
                switch(type_)
                {
                case kType_Image:
                    free(data_.image_.textures_);
                    break;
                case kType_Constants:
                    free(data_.constants_);
                    break;
                default:
                    break;
                }
                memset(&data_, 0, sizeof(data_));
                type_ = kType_Count;
                data_size_ = 0;
                ++id_;
            }

            char const *getTypeName() const
            {
                switch(type_)
                {
                case kType_Buffer:
                    return "Buffer";
                case kType_Image:
                    return "Texture";
                case kType_SamplerState:
                    return "Sampler state";
                case kType_AccelerationStructure:
                    return "Acceleration structure";
                case kType_Constants:
                    return "Constants";
                default:
                    break;  // undefined type
                }
                return "undefined";
            }
        };

        typedef std::map<uint64_t, Parameter> Parameters;

        Parameter &insertParameter(char const *parameter_name)
        {
            uint64_t const parameter_id = Hash(parameter_name);
            Parameters::iterator const it = parameters_.find(parameter_id);
            GFX_ASSERT(parameter_name != nullptr && *parameter_name != '\0');
            if(it == parameters_.end())
            {
                Parameter &parameter = parameters_[parameter_id];
                parameter.name_ = parameter_name;
                return parameter;
            }
            return (*it).second;
        }

        void eraseParameter(char const *parameter_name)
        {
            uint64_t const parameter_id = Hash(parameter_name);
            Parameters::iterator const it = parameters_.find(parameter_id);
            GFX_ASSERT(parameter_name != nullptr && *parameter_name != '\0');
            if(it != parameters_.end()) { (*it).second.unset(); parameters_.erase(it); }
        }

        String cs_;
        String vs_;
        String gs_;
        String ps_;
        String file_name_;
        String file_path_;
        Parameters parameters_;
    };
    GfxArray<Program> programs_;
    GfxHandles program_handles_;

    struct Kernel
    {
        inline bool isCompute() const { return (num_threads_ != nullptr && *num_threads_ > 0); }

        struct Parameter
        {
            enum Type
            {
                kType_Buffer = 0,
                kType_RWBuffer,

                kType_Texture2D,
                kType_RWTexture2D,

                kType_Texture2DArray,
                kType_RWTexture2DArray,

                kType_Texture3D,
                kType_RWTexture3D,

                kType_TextureCube,

                kType_AccelerationStructure,

                kType_Constants,
                kType_ConstantBuffer,

                kType_Sampler,

                kType_Count
            };

            Type type_ = kType_Count;
            uint32_t id_ = 0xFFFFFFFFu;
            uint64_t parameter_id_ = 0;
            uint32_t descriptor_count_ = 0;
            uint32_t descriptor_slot_ = 0xFFFFFFFFu;
            Program::Parameter const *parameter_ = nullptr;

            struct Variable
            {
                uint32_t id_ = 0;
                uint32_t data_size_ = 0;
                uint32_t data_start_ = 0;
                uint64_t parameter_id_ = 0;
                Program::Parameter const *parameter_ = nullptr;
            };

            Variable *variables_ = nullptr;
            uint32_t variable_count_ = 0;
            uint32_t variable_size_ = 0;
        };

        String entry_point_;
        GfxProgram program_ = {};
        DrawState::Data draw_state_;
        std::vector<String> defines_;
        uint64_t descriptor_heap_id_ = 0;
        uint32_t *num_threads_ = nullptr;
        IDxcBlob *cs_bytecode_ = nullptr;
        IDxcBlob *vs_bytecode_ = nullptr;
        IDxcBlob *gs_bytecode_ = nullptr;
        IDxcBlob *ps_bytecode_ = nullptr;
        ID3D12ShaderReflection *cs_reflection_ = nullptr;
        ID3D12ShaderReflection *vs_reflection_ = nullptr;
        ID3D12ShaderReflection *gs_reflection_ = nullptr;
        ID3D12ShaderReflection *ps_reflection_ = nullptr;
        ID3D12RootSignature *root_signature_ = nullptr;
        ID3D12PipelineState *pipeline_state_ = nullptr;
        Parameter *parameters_ = nullptr;
        uint32_t parameter_count_ = 0;
        uint32_t vertex_stride_ = 0;
    };
    GfxArray<Kernel> kernels_;
    GfxHandles kernel_handles_;

    enum ShaderType
    {
        kShaderType_CS = 0,
        kShaderType_VS,
        kShaderType_GS,
        kShaderType_PS,

        kShaderType_Count
    };
    static char const *shader_extensions_[kShaderType_Count];
    uint32_t dummy_descriptors_[Kernel::Parameter::kType_Count] = {};
    uint32_t dummy_rtv_descriptor_ = 0xFFFFFFFFu;

    struct TimestampQuery
    {
        float duration_ = 0.0f;
        bool was_begun_ = false;
    };
    GfxArray<TimestampQuery> timestamp_queries_;
    GfxHandles timestamp_query_handles_;

    struct TimestampQueryHeap
    {
        GfxBuffer query_buffer_ = {};
        ID3D12QueryHeap *query_heap_ = nullptr;
        std::map<uint64_t, std::pair<uint32_t, GfxTimestampQuery>> timestamp_queries_;
    };
    uint64_t timestamp_query_ticks_per_second_ = 0;
    TimestampQueryHeap timestamp_query_heaps_[kGfxConstant_BackBufferCount];

public:
    GfxInternal(GfxContext &gfx) : buffer_handles_("buffer"), texture_handles_("texture"), sampler_state_handles_("sampler state")
                                 , acceleration_structure_handles_("acceleration structure"), raytracing_primitive_handles_("raytracing primitive")
                                 , program_handles_("program"), kernel_handles_("kernel"), timestamp_query_handles_("timestamp query")
                                 { gfx.handle = reinterpret_cast<uint64_t>(this); }
    ~GfxInternal() { terminate(); }

    GfxResult initialize(HWND window, GfxContext &context, GfxCreateContextFlags flags)
    {
        if(!window)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "An invalid window handle was supplied");
        if((flags & kGfxCreateContextFlag_EnableDebugLayer) != 0)
        {
            ID3D12Debug1 *debug_controller = nullptr;
            if(!SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
                GFX_PRINTLN("Warning: Unable to get D3D12 debug interface, no debugging information will be available");
            else
            {
                debug_controller->EnableDebugLayer();
                //debug_controller->SetEnableGPUBasedValidation(true);
                debug_controller->SetEnableSynchronizedCommandQueueValidation(true);
                debug_controller->Release();
            }
        }
        IDXGIFactory4 *factory = nullptr;
        if(!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create DXGI factory");

        IDXGIAdapter1 *adapters[8] = {};
        uint32_t adapter_scores[ARRAYSIZE(adapters)] = {};
        DXGI_ADAPTER_DESC1 adapter_descs[ARRAYSIZE(adapters)] = {};
        for(uint32_t i = 0; i < ARRAYSIZE(adapters); ++i)
        {
            IDXGIAdapter1 *adapter = nullptr;
            if(!SUCCEEDED(factory->EnumAdapters1(i, &adapter)))
                break;
            uint32_t j, adapter_score;
            DXGI_ADAPTER_DESC1 adapter_desc = {};
            adapter->GetDesc1(&adapter_desc);
            if(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                adapter_score = 0;
            else
                switch(adapter_desc.VendorId)
                {
                case 0x1002u:   // AMD
                    adapter_score = 3;
                    break;
                case 0x10DEu:   // NVIDIA
                    adapter_score = 2;
                    break;
                default:
                    adapter_score = 1;
                    break;
                }
            for(j = 0; j < i; ++j)
                if(adapter_score > adapter_scores[j] ||
                  (adapter_score == adapter_scores[j] && adapter_desc.DedicatedVideoMemory > adapter_descs[j].DedicatedVideoMemory))
                    break;
            for(uint32_t k = i; k > j; --k)
            {
                adapters[k] = adapters[k - 1];
                adapter_descs[k] = adapter_descs[k - 1];
                adapter_scores[k] = adapter_scores[k - 1];
            }
            adapters[j] = adapter;
            adapter_descs[j] = adapter_desc;
            adapter_scores[j] = adapter_score;
        }

        struct DXGIRelease
        {
            GFX_NON_COPYABLE(DXGIRelease);
            IDXGIFactory4 *factory; IDXGIAdapter1 *(&adapters)[ARRAYSIZE(adapters)];
            DXGIRelease(IDXGIFactory4 *factory, IDXGIAdapter1 *(&adapters)[ARRAYSIZE(adapters)]) : factory(factory), adapters(adapters) {}
            ~DXGIRelease() { factory->Release(); for(uint32_t i = 0; i < ARRAYSIZE(adapters); ++i) if(adapters[i]) adapters[i]->Release(); }
        };
        DXGIRelease const scope(factory, adapters);

        uint32_t i = 0;
        ID3D12Device *device = nullptr;
        for(; i < ARRAYSIZE(adapters); ++i)
            if(!adapters[i]) break; else
            if(SUCCEEDED(D3D12CreateDevice(adapters[i], D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device))))
                break;
        if(!device)
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create D3D12 device");
        if(!SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxc_utils_))) ||
           !SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxc_compiler_))) ||
           !SUCCEEDED(dxc_utils_->CreateDefaultIncludeHandler(&dxc_include_handler_)))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create DXC compiler");
        device->QueryInterface(IID_PPV_ARGS(&device_));
        SetDebugName(device_, "gfx_Device");
        device->Release();

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 rt_features = {};
        device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &rt_features, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
        raytracing_support_ = (rt_features.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1 ? true : false);

        D3D12_COMMAND_QUEUE_DESC
        queue_desc      = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        if(!SUCCEEDED(device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_))))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create command queue");
        command_queue_->GetTimestampFrequency(&timestamp_query_ticks_per_second_);
        SetDebugName(command_queue_, "gfx_CommandQueue");

        RECT window_rect = {};
        GetClientRect(window, &window_rect);
        window_width_ = window_rect.right - window_rect.left;
        window_height_ = window_rect.bottom - window_rect.top;

        DXGI_SWAP_CHAIN_DESC1
        swap_chain_desc                  = {};
        swap_chain_desc.Width            = window_width_;
        swap_chain_desc.Height           = window_height_;
        swap_chain_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.BufferCount      = kGfxConstant_BackBufferCount;
        swap_chain_desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swap_chain_desc.SampleDesc.Count = 1;
        IDXGISwapChain1 *swap_chain = nullptr;
        if(!SUCCEEDED(factory->CreateSwapChainForHwnd(command_queue_, window, &swap_chain_desc, nullptr, nullptr, &swap_chain)))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create swap chain");
        swap_chain->QueryInterface(IID_PPV_ARGS(&swap_chain_)); swap_chain->Release();
        if(!swap_chain_ || !SUCCEEDED(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER)))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to initialize swap chain");
        fence_index_ = swap_chain_->GetCurrentBackBufferIndex();

        D3D12MA::ALLOCATOR_DESC
        allocator_desc         = {};
        allocator_desc.Flags   = D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;
        allocator_desc.pDevice = device_;
        if(!SUCCEEDED(D3D12MA::CreateAllocator(&allocator_desc, &mem_allocator_)))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create memory allocator");

        for(uint32_t j = 0; j < ARRAYSIZE(command_allocators_); ++j)
        {
            char buffer[256];
            GFX_SNPRINTF(buffer, sizeof(buffer), "gfx_CommandAllocator%u", j);
            if(!SUCCEEDED(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocators_[j]))))
                return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create command allocator");
            SetDebugName(command_allocators_[j], buffer);
        }
        if(!SUCCEEDED(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, *command_allocators_, nullptr, IID_PPV_ARGS(&command_list_))))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create command list");
        SetDebugName(command_list_, "gfx_CommandList");

        fence_event_ = CreateEvent(nullptr, false, false, nullptr);
        if(!fence_event_)
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create event handle");
        for(uint32_t j = 0; j < ARRAYSIZE(fences_); ++j)
        {
            char buffer[256];
            GFX_SNPRINTF(buffer, sizeof(buffer), "gfx_Fence%u", j);
            if(!SUCCEEDED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fences_[j]))))
                return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create fence object");
            SetDebugName(fences_[j], buffer);
        }
        memset(fence_values_, 0, sizeof(fence_values_));

        for(uint32_t j = 0; j < ARRAYSIZE(dummy_descriptors_); ++j)
        {
            dummy_descriptors_[j] = (j == Kernel::Parameter::kType_Sampler ? allocateSamplerDescriptor() : allocateDescriptor());
            if(dummy_descriptors_[j] == 0xFFFFFFFFu)
                return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to allocate dummy descriptors");
        }
        dummy_rtv_descriptor_ = allocateRTVDescriptor();
        if(dummy_rtv_descriptor_ == 0xFFFFFFFFu)
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to allocate dummy descriptors");
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
            rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            device_->CreateRenderTargetView(nullptr, &rtv_desc, rtv_descriptors_.getCPUHandle(dummy_rtv_descriptor_));
        }
        populateDummyDescriptors();

        GFX_TRY(acquireSwapChainBuffers());
        GFX_TRY(populateDrawIdBuffer(1024));

        D3D12_RESOURCE_BARRIER resource_barrier = {};
        resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Transition.pResource = back_buffers_[fence_index_];
        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        command_list_->ResourceBarrier(1, &resource_barrier);

        D3D12_INDIRECT_ARGUMENT_DESC dispatch_argument_desc = {};
        dispatch_argument_desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
        D3D12_COMMAND_SIGNATURE_DESC dispatch_signature_desc = {};
        dispatch_signature_desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
        dispatch_signature_desc.NumArgumentDescs = 1;
        dispatch_signature_desc.pArgumentDescs = &dispatch_argument_desc;
        if(!SUCCEEDED(device_->CreateCommandSignature(&dispatch_signature_desc, nullptr, IID_PPV_ARGS(&dispatch_signature_))))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create the dispatch command signature");

        D3D12_INDIRECT_ARGUMENT_DESC multi_draw_argument_desc = {};
        multi_draw_argument_desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
        D3D12_COMMAND_SIGNATURE_DESC multi_draw_signature_desc = {};
        multi_draw_signature_desc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
        multi_draw_signature_desc.NumArgumentDescs = 1;
        multi_draw_signature_desc.pArgumentDescs = &multi_draw_argument_desc;
        if(!SUCCEEDED(device_->CreateCommandSignature(&multi_draw_signature_desc, nullptr, IID_PPV_ARGS(&multi_draw_signature_))))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create the multi-draw command signature");

        D3D12_INDIRECT_ARGUMENT_DESC multi_draw_indexed_arguments_desc = {};
        multi_draw_indexed_arguments_desc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
        D3D12_COMMAND_SIGNATURE_DESC multi_draw_indexed_signature_desc = {};
        multi_draw_indexed_signature_desc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
        multi_draw_indexed_signature_desc.NumArgumentDescs = 1;
        multi_draw_indexed_signature_desc.pArgumentDescs = &multi_draw_indexed_arguments_desc;
        if(!SUCCEEDED(device_->CreateCommandSignature(&multi_draw_indexed_signature_desc, nullptr, IID_PPV_ARGS(&multi_draw_indexed_signature_))))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create indexed multi-draw command signature");

        GfxProgramDesc clear_buffer_program_desc = {};
        clear_buffer_program_desc.cs = "RWBuffer<uint> OutputBuffer; uint ClearValue; [numthreads(128, 1, 1)] void main(in uint gidx : SV_DispatchThreadID) { OutputBuffer[gidx] = ClearValue; }";
        clear_buffer_program_ = createProgram(clear_buffer_program_desc, "gfx_ClearBufferProgram");
        clear_buffer_kernel_ = createComputeKernel(clear_buffer_program_, "main", nullptr, 0);
        if(!clear_buffer_kernel_)
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create the compute kernel for clearing buffer objects");

        GfxProgramDesc generate_mips_program_desc = {};
        generate_mips_program_desc.cs =
            "RWTexture2D<float4> InputBuffer;\r\n"
            "RWTexture2D<float4> OutputBuffer;\r\n"
            "\r\n"
            "[numthreads(16, 16, 1)]\r\n"
            "void main(in uint2 gidx : SV_DispatchThreadID)\r\n"
            "{\r\n"
            "    uint2 dims;\r\n"
            "    float w = 0.0f;\r\n"
            "    InputBuffer.GetDimensions(dims.x, dims.y);\r\n"
            "    if(any(gidx >= max(dims >> 1, 1))) return;\r\n"
            "    float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);\r\n"
            "    for(uint y = 0; y < 2; ++y)\r\n"
            "        for(uint x = 0; x < 2; ++x)\r\n"
            "        {\r\n"
            "            const uint2 pix = (gidx << 1) + uint2(x, y);\r\n"
            "            if(any(pix >= dims)) break; // out of bounds\r\n"
            "            result += InputBuffer[pix];\r\n"
            "            ++w;\r\n"
            "        }\r\n"
            "    OutputBuffer[gidx] = result / w;\r\n"
            "}\r\n";
        generate_mips_program_ = createProgram(generate_mips_program_desc, "gfx_GenerateMipsProgram");
        generate_mips_kernel_ = createComputeKernel(generate_mips_program_, "main", nullptr, 0);
        if(!generate_mips_kernel_)
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create the compute kernel for generating the texture mips");

        DXGI_ADAPTER_DESC1 adapter_desc = {};
        adapters[i]->GetDesc1(&adapter_desc);
        context.vendor_id = adapter_desc.VendorId;
        GFX_PRINTLN("Created Direct3D12 device `%ws'", adapter_desc.Description);
        GFX_SNPRINTF(context.name, sizeof(context.name), "%ws", adapter_desc.Description);
        if(!raytracing_support_)
            GFX_PRINTLN("Warning: DXR-1.1 is not supported on the selected device; no raytracing will be available");
        bound_viewport_.invalidate(); bound_scissor_rect_.invalidate();
        window_ = window;   // flag as successfully initialized

        return kGfxResult_NoError;
    }

    GfxResult terminate()
    {
        destroyBuffer(draw_id_buffer_);
        destroyKernel(clear_buffer_kernel_);
        destroyProgram(clear_buffer_program_);
        destroyKernel(generate_mips_kernel_);
        destroyProgram(generate_mips_program_);
        destroyBuffer(texture_upload_buffer_);
        destroyBuffer(raytracing_scratch_buffer_);
        for(uint32_t i = 0; i < ARRAYSIZE(constant_buffer_pool_); ++i)
            destroyBuffer(constant_buffer_pool_[i]);
        for(std::map<uint32_t, ScanKernels>::const_iterator it = scan_kernels_.begin(); it != scan_kernels_.end(); ++it)
        {
            destroyProgram((*it).second.scan_program_);
            destroyKernel((*it).second.reduce_kernel_);
            destroyKernel((*it).second.scan_add_kernel_);
            destroyKernel((*it).second.scan_kernel_);
            destroyKernel((*it).second.args_kernel_);
        }

        collect(descriptors_);
        collect(dsv_descriptors_);
        collect(rtv_descriptors_);
        collect(sampler_descriptors_);

        for(uint32_t i = 0; i < buffers_.size(); ++i)
            collect(buffers_.data()[i]);
        for(uint32_t i = 0; i < textures_.size(); ++i)
            collect(textures_.data()[i]);
        for(uint32_t i = 0; i < sampler_states_.size(); ++i)
            collect(sampler_states_.data()[i]);
        for(uint32_t i = 0; i < acceleration_structures_.size(); ++i)
            collect(acceleration_structures_.data()[i]);
        for(uint32_t i = 0; i < raytracing_primitives_.size(); ++i)
            collect(raytracing_primitives_.data()[i]);
        for(uint32_t i = 0; i < kernels_.size(); ++i)
            collect(kernels_.data()[i]);
        for(uint32_t i = 0; i < programs_.size(); ++i)
            collect(programs_.data()[i]);
        for(uint32_t i = 0; i < ARRAYSIZE(timestamp_query_heaps_); ++i)
        {
            collect(timestamp_query_heaps_[i].query_heap_);
            destroyBuffer(timestamp_query_heaps_[i].query_buffer_);
        }

        forceGarbageCollection();
        freelist_descriptors_.clear();
        freelist_dsv_descriptors_.clear();
        freelist_rtv_descriptors_.clear();
        freelist_sampler_descriptors_.clear();

        if(device_)
            device_->Release();
        if(command_queue_)
            command_queue_->Release();
        if(command_list_)
            command_list_->Release();
        for(uint32_t i = 0; i < ARRAYSIZE(command_allocators_); ++i)
            if(command_allocators_[i])
                command_allocators_[i]->Release();

        if(fence_event_)
            CloseHandle(fence_event_);
        for(uint32_t i = 0; i < ARRAYSIZE(fences_); ++i)
            if(fences_[i])
                fences_[i]->Release();

        if(dxc_utils_)
            dxc_utils_->Release();
        if(dxc_compiler_)
            dxc_compiler_->Release();
        if(dxc_include_handler_)
            dxc_include_handler_->Release();

        if(swap_chain_)
            swap_chain_->Release();
        if(mem_allocator_)
            mem_allocator_->Release();
        if(dispatch_signature_)
            dispatch_signature_->Release();
        if(multi_draw_signature_)
            multi_draw_signature_->Release();
        if(multi_draw_indexed_signature_)
            multi_draw_indexed_signature_->Release();
        for(uint32_t i = 0; i < ARRAYSIZE(back_buffers_); ++i)
            if(back_buffers_[i])
                back_buffers_[i]->Release();

        return kGfxResult_NoError;
    }

    inline uint32_t getBackBufferWidth() const
    {
        return window_width_;
    }

    inline uint32_t getBackBufferHeight() const
    {
        return window_height_;
    }

    inline uint32_t getBackBufferIndex() const
    {
        return fence_index_;
    }

    GfxBuffer createBuffer(uint64_t size, void const *data, GfxCpuAccess cpu_access, D3D12_RESOURCE_STATES resource_state = D3D12_RESOURCE_STATE_COPY_DEST)
    {
        GfxBuffer buffer = {};
        if(cpu_access >= kGfxCpuAccess_Count)
            return buffer;  // invalid parameter
        uint32_t const alignment = (cpu_access == kGfxCpuAccess_Write ? 256 : 4);
        D3D12_RESOURCE_DESC
        resource_desc                  = {};
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Width            = GFX_MAX(GFX_ALIGN(size, alignment), (uint64_t)alignment);
        resource_desc.Height           = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels        = 1;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        buffer.handle = buffer_handles_.allocate_handle();
        D3D12MA::ALLOCATION_DESC allocation_desc = {};
        Buffer &gfx_buffer = buffers_.insert(buffer);
        switch(cpu_access)
        {
        case kGfxCpuAccess_Read:
            resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
            allocation_desc.HeapType = D3D12_HEAP_TYPE_READBACK;
            break;
        case kGfxCpuAccess_Write:
            allocation_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
            resource_state = D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        default:    // kGfxCpuAccess_None
            allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            break;
        }
        if(createResource(allocation_desc, resource_desc, resource_state, &gfx_buffer.allocation_, IID_PPV_ARGS(&gfx_buffer.resource_)) != kGfxResult_NoError)
        {
            GFX_PRINT_ERROR(kGfxResult_OutOfMemory, "Unable to create buffer object of size %u MiB", (uint32_t)((size + 1024 * 1024 - 1) / (1024 * 1024)));
            destroyBuffer(buffer); buffer = {};
            return buffer;
        }
        if(cpu_access != kGfxCpuAccess_None)
            switch(cpu_access)
            {
            case kGfxCpuAccess_Write:
                {
                    D3D12_RANGE read_range = {};
                    gfx_buffer.resource_->Map(0, &read_range, &gfx_buffer.data_);
                    memset(gfx_buffer.data_, 0, (size_t)resource_desc.Width);
                    if(data) memcpy(gfx_buffer.data_, data, (size_t)size);
                }
                break;
            default:
                gfx_buffer.resource_->Map(0, nullptr, &gfx_buffer.data_);
                break;
            }
        buffer.size = size;
        buffer.cpu_access = cpu_access;
        buffer.stride = sizeof(uint32_t);
        gfx_buffer.reference_count_ = (uint32_t *)malloc(sizeof(uint32_t));
        GFX_ASSERT(gfx_buffer.reference_count_ != nullptr);
        *gfx_buffer.reference_count_ = 1;   // retain
        gfx_buffer.resource_state_ = (D3D12_RESOURCE_STATES *)malloc(sizeof(D3D12_RESOURCE_STATES));
        GFX_ASSERT(gfx_buffer.resource_state_ != nullptr);
        *gfx_buffer.resource_state_ = resource_state;
        if(data != nullptr && cpu_access != kGfxCpuAccess_Write)
        {
            GfxBuffer const upload_buffer = createBuffer(size, data, kGfxCpuAccess_Write);
            encodeCopyBuffer(buffer, upload_buffer);
            destroyBuffer(upload_buffer);
        }
        return buffer;
    }

    GfxBuffer createBufferRange(GfxBuffer const &buffer, uint64_t byte_offset, uint64_t size)
    {
        GfxBuffer buffer_range = {};
        if(!buffer_handles_.has_handle(buffer.handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Cannot create a buffer range from an invalid buffer object");
            return buffer_range;
        }
        if(byte_offset + size > buffer.size)
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot create a buffer range that is larger than the original buffer object");
            return buffer_range;
        }
        uint32_t const alignment = (buffer.cpu_access == kGfxCpuAccess_Write ? 256 : 4);
        if(byte_offset != GFX_ALIGN(byte_offset, alignment))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Buffer objects with %s CPU access must be %u-byte aligned; cannot create range starting at offset %llu",
                buffer.cpu_access == kGfxCpuAccess_Read ? "read" : buffer.cpu_access == kGfxCpuAccess_Write ? "write" : "no", alignment, byte_offset);
            return buffer_range;
        }
        Buffer const gfx_buffer = buffers_[buffer];
        buffer_range.handle = buffer_handles_.allocate_handle();
        Buffer &gfx_buffer_range = buffers_.insert(buffer_range, gfx_buffer);
        if(gfx_buffer_range.data_ != nullptr) gfx_buffer_range.data_ = (char *)gfx_buffer_range.data_ + byte_offset;
        GFX_ASSERT(gfx_buffer_range.resource_ != nullptr && gfx_buffer_range.allocation_ != nullptr && gfx_buffer_range.reference_count_ != nullptr);
        gfx_buffer_range.data_offset_ += byte_offset;
        ++*gfx_buffer_range.reference_count_;   // retain
        buffer_range.cpu_access = buffer.cpu_access;
        strcpy(buffer_range.name, buffer.name);
        buffer_range.stride = buffer.stride;
        buffer_range.size = size;
        return buffer_range;
    }

    GfxResult destroyBuffer(GfxBuffer const &buffer)
    {
        if(!buffer)
            return kGfxResult_NoError;
        if(!buffer_handles_.has_handle(buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid buffer object");
        collect(buffers_[buffer]);  // release resources
        buffers_.erase(buffer); // destroy buffer
        buffer_handles_.free_handle(buffer.handle);
        return kGfxResult_NoError;
    }

    void *getBufferData(GfxBuffer const &buffer)
    {
        if(!buffer_handles_.has_handle(buffer.handle))
            return nullptr; // invalid buffer object
        Buffer const &gfx_buffer = buffers_[buffer];
        return gfx_buffer.data_;
    }

    GfxTexture createTexture2D(DXGI_FORMAT format)
    {
        return createTexture2D(window_width_, window_height_, format, 1, Texture::kFlag_AutoResize);
    }

    GfxTexture createTexture2D(uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t mip_levels, uint32_t flags = 0)
    {
        GfxTexture texture = {};
        if(format == DXGI_FORMAT_UNKNOWN || format == DXGI_FORMAT_FORCE_UINT)
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Cannot create texture object using invalid format");
            return texture; // invalid parameter
        }
        width = GFX_MAX(width, 1u);
        height = GFX_MAX(height, 1u);
        mip_levels = GFX_MIN(GFX_MAX(mip_levels, 1u), gfxCalculateMipCount(width, height));
        D3D12_RESOURCE_STATES const resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
        D3D12_RESOURCE_DESC
        resource_desc                  = {};
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Width            = width;
        resource_desc.Height           = height;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels        = (uint16_t)mip_levels;
        resource_desc.Format           = format;
        resource_desc.SampleDesc.Count = 1;
        texture.handle = texture_handles_.allocate_handle();
        Texture &gfx_texture = textures_.insert(texture);
        D3D12MA::ALLOCATION_DESC allocation_desc = {};
        allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        if(createResource(allocation_desc, resource_desc, resource_state, &gfx_texture.allocation_, IID_PPV_ARGS(&gfx_texture.resource_)) != kGfxResult_NoError)
        {
            GFX_PRINT_ERROR(kGfxResult_OutOfMemory, "Unable to create 2D texture object of size %ux%u", width, height);
            gfx_texture.resource_ = nullptr; gfx_texture.allocation_ = nullptr;
            destroyTexture(texture); texture = {};
            return texture;
        }
        texture.format = format;
        texture.mip_levels = mip_levels;
        texture.type = GfxTexture::kType_2D;
        texture.depth = 1;
        if(!((flags & Texture::kFlag_AutoResize) != 0))
        {
            texture.width = width;
            texture.height = height;
        }
        gfx_texture.resource_state_ = resource_state;
        gfx_texture.flags_ = flags;
        return texture;
    }

    GfxTexture createTexture2DArray(uint32_t width, uint32_t height, uint32_t slice_count, DXGI_FORMAT format, uint32_t mip_levels)
    {
        GfxTexture texture = {};
        if(format == DXGI_FORMAT_UNKNOWN || format == DXGI_FORMAT_FORCE_UINT)
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Cannot create texture object using invalid format");
            return texture; // invalid parameter
        }
        if(slice_count > 0xFFFFu)
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot create a 2D texture array with more than %u slices", 0xFFFFu);
            return texture; // invalid operation
        }
        width = GFX_MAX(width, 1u);
        height = GFX_MAX(height, 1u);
        slice_count = GFX_MAX(slice_count, 1u);
        mip_levels = GFX_MIN(GFX_MAX(mip_levels, 1u), gfxCalculateMipCount(width, height));
        D3D12_RESOURCE_STATES const resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
        D3D12_RESOURCE_DESC
        resource_desc                  = {};
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Width            = width;
        resource_desc.Height           = height;
        resource_desc.DepthOrArraySize = (uint16_t)slice_count;
        resource_desc.MipLevels        = (uint16_t)mip_levels;
        resource_desc.Format           = format;
        resource_desc.SampleDesc.Count = 1;
        texture.handle = texture_handles_.allocate_handle();
        Texture &gfx_texture = textures_.insert(texture);
        D3D12MA::ALLOCATION_DESC allocation_desc = {};
        allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        if(createResource(allocation_desc, resource_desc, resource_state, &gfx_texture.allocation_, IID_PPV_ARGS(&gfx_texture.resource_)) != kGfxResult_NoError)
        {
            GFX_PRINT_ERROR(kGfxResult_OutOfMemory, "Unable to create 2D texture array object of size %ux%ux%u", width, height, slice_count);
            gfx_texture.resource_ = nullptr; gfx_texture.allocation_ = nullptr;
            destroyTexture(texture); texture = {};
            return texture;
        }
        texture.format = format;
        texture.mip_levels = mip_levels;
        texture.type = GfxTexture::kType_2DArray;
        texture.width = width;
        texture.height = height;
        texture.depth = slice_count;
        gfx_texture.resource_state_ = resource_state;
        return texture;
    }

    GfxTexture createTexture3D(uint32_t width, uint32_t height, uint32_t depth, DXGI_FORMAT format, uint32_t mip_levels)
    {
        GfxTexture texture = {};
        if(format == DXGI_FORMAT_UNKNOWN || format == DXGI_FORMAT_FORCE_UINT)
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Cannot create texture object using invalid format");
            return texture; // invalid parameter
        }
        if(depth > 0xFFFFu)
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot create a 3D texture with a depth larger than %u", 0xFFFFu);
            return texture; // invalid operation
        }
        width = GFX_MAX(width, 1u);
        height = GFX_MAX(height, 1u);
        depth = GFX_MAX(depth, 1u);
        mip_levels = GFX_MIN(GFX_MAX(mip_levels, 1u), gfxCalculateMipCount(width, height, depth));
        D3D12_RESOURCE_STATES const resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
        D3D12_RESOURCE_DESC
        resource_desc                  = {};
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        resource_desc.Width            = width;
        resource_desc.Height           = height;
        resource_desc.DepthOrArraySize = (uint16_t)depth;
        resource_desc.MipLevels        = (uint16_t)mip_levels;
        resource_desc.Format           = format;
        resource_desc.SampleDesc.Count = 1;
        texture.handle = texture_handles_.allocate_handle();
        Texture &gfx_texture = textures_.insert(texture);
        D3D12MA::ALLOCATION_DESC allocation_desc = {};
        allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        if(createResource(allocation_desc, resource_desc, resource_state, &gfx_texture.allocation_, IID_PPV_ARGS(&gfx_texture.resource_)) != kGfxResult_NoError)
        {
            GFX_PRINT_ERROR(kGfxResult_OutOfMemory, "Unable to create 3D texture object of size %ux%ux%u", width, height, depth);
            gfx_texture.resource_ = nullptr; gfx_texture.allocation_ = nullptr;
            destroyTexture(texture); texture = {};
            return texture;
        }
        texture.format = format;
        texture.mip_levels = mip_levels;
        texture.type = GfxTexture::kType_3D;
        texture.width = width;
        texture.height = height;
        texture.depth = depth;
        gfx_texture.resource_state_ = resource_state;
        return texture;
    }

    GfxTexture createTextureCube(uint32_t size, DXGI_FORMAT format, uint32_t mip_levels)
    {
        GfxTexture texture = {};
        if(format == DXGI_FORMAT_UNKNOWN || format == DXGI_FORMAT_FORCE_UINT)
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Cannot create texture object using invalid format");
            return texture; // invalid parameter
        }
        size = GFX_MAX(size, 1u);
        mip_levels = GFX_MIN(GFX_MAX(mip_levels, 1u), gfxCalculateMipCount(size));
        D3D12_RESOURCE_STATES const resource_state = D3D12_RESOURCE_STATE_COPY_DEST;
        D3D12_RESOURCE_DESC
        resource_desc                  = {};
        resource_desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Width            = size;
        resource_desc.Height           = size;
        resource_desc.DepthOrArraySize = 6;
        resource_desc.MipLevels        = (uint16_t)mip_levels;
        resource_desc.Format           = format;
        resource_desc.SampleDesc.Count = 1;
        texture.handle = texture_handles_.allocate_handle();
        Texture &gfx_texture = textures_.insert(texture);
        D3D12MA::ALLOCATION_DESC allocation_desc = {};
        allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        if(createResource(allocation_desc, resource_desc, resource_state, &gfx_texture.allocation_, IID_PPV_ARGS(&gfx_texture.resource_)) != kGfxResult_NoError)
        {
            GFX_PRINT_ERROR(kGfxResult_OutOfMemory, "Unable to create cubemap texture object of size %u", size);
            gfx_texture.resource_ = nullptr; gfx_texture.allocation_ = nullptr;
            destroyTexture(texture); texture = {};
            return texture;
        }
        texture.format = format;
        texture.mip_levels = mip_levels;
        texture.type = GfxTexture::kType_Cube;
        texture.width = size;
        texture.height = size;
        texture.depth = 6;
        gfx_texture.resource_state_ = resource_state;
        return texture;
    }

    GfxResult destroyTexture(GfxTexture const &texture)
    {
        if(!texture)
            return kGfxResult_NoError;
        if(!texture_handles_.has_handle(texture.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid texture object");
        collect(textures_[texture]);    // release resources
        textures_.erase(texture);   // destroy texture object
        texture_handles_.free_handle(texture.handle);
        return kGfxResult_NoError;
    }

    GfxSamplerState createSamplerState(D3D12_FILTER filter,
                                       D3D12_TEXTURE_ADDRESS_MODE address_u,
                                       D3D12_TEXTURE_ADDRESS_MODE address_v,
                                       D3D12_TEXTURE_ADDRESS_MODE address_w,
                                       float mip_lod_bias, float min_lod, float max_lod)
    {
        GfxSamplerState const sampler_state = {};
        if(D3D12_DECODE_IS_COMPARISON_FILTER(filter))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Comparison samplers require a valid comparison function; cannot create sampler state object");
            return sampler_state;
        }
        return createSamplerState(filter, (D3D12_COMPARISON_FUNC)0, address_u, address_v, address_w, mip_lod_bias, min_lod, max_lod);
    }

    GfxSamplerState createSamplerState(D3D12_FILTER filter,
                                       D3D12_COMPARISON_FUNC comparison_func,
                                       D3D12_TEXTURE_ADDRESS_MODE address_u,
                                       D3D12_TEXTURE_ADDRESS_MODE address_v,
                                       D3D12_TEXTURE_ADDRESS_MODE address_w,
                                       float mip_lod_bias, float min_lod, float max_lod)
    {
        GfxSamplerState sampler_state = {};
        sampler_state.handle = sampler_state_handles_.allocate_handle();
        SamplerState &gfx_sampler_state = sampler_states_.insert(sampler_state);
        gfx_sampler_state.descriptor_slot_ = allocateSamplerDescriptor();
        if(gfx_sampler_state.descriptor_slot_ == 0xFFFFFFFFu)
        {
            GFX_PRINT_ERROR(kGfxResult_OutOfMemory, "Unable to allocate descriptor for sampler state object");
            destroySamplerState(sampler_state); sampler_state = {};
            return sampler_state;
        }
        gfx_sampler_state.sampler_desc_.Filter = filter;
        gfx_sampler_state.sampler_desc_.AddressU = address_u;
        gfx_sampler_state.sampler_desc_.AddressV = address_v;
        gfx_sampler_state.sampler_desc_.AddressW = address_w;
        gfx_sampler_state.sampler_desc_.MipLODBias = mip_lod_bias;
        gfx_sampler_state.sampler_desc_.MaxAnisotropy = (D3D12_DECODE_IS_ANISOTROPIC_FILTER(filter) ? kGfxConstant_MaxAnisotropy : 0);
        gfx_sampler_state.sampler_desc_.ComparisonFunc = comparison_func;
        gfx_sampler_state.sampler_desc_.MinLOD = min_lod;
        gfx_sampler_state.sampler_desc_.MaxLOD = max_lod;
        device_->CreateSampler(&gfx_sampler_state.sampler_desc_, sampler_descriptors_.getCPUHandle(gfx_sampler_state.descriptor_slot_));
        sampler_state.max_anisotropy = gfx_sampler_state.sampler_desc_.MaxAnisotropy;
        sampler_state.comparison_func = comparison_func;
        sampler_state.mip_lod_bias = mip_lod_bias;
        sampler_state.address_u = address_u;
        sampler_state.address_v = address_v;
        sampler_state.address_w = address_w;
        sampler_state.min_lod = min_lod;
        sampler_state.max_lod = max_lod;
        sampler_state.filter = filter;
        return sampler_state;
    }

    GfxResult destroySamplerState(GfxSamplerState const &sampler_state)
    {
        if(!sampler_state)
            return kGfxResult_NoError;
        if(!sampler_state_handles_.has_handle(sampler_state.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid sampler state object");
        collect(sampler_states_[sampler_state]);    // release resources
        sampler_states_.erase(sampler_state);   // destroy sampler state
        sampler_state_handles_.free_handle(sampler_state.handle);
        return kGfxResult_NoError;
    }

    GfxAccelerationStructure createAccelerationStructure()
    {
        GfxAccelerationStructure acceleration_structure = {};
        if(!raytracing_support_)
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Raytracing isn't supported on the selected device; cannot create acceleration structure");
            return acceleration_structure;  // invalid operation
        }
        acceleration_structure.handle = acceleration_structure_handles_.allocate_handle();
        acceleration_structures_.insert(acceleration_structure);
        return acceleration_structure;
    }

    GfxResult destroyAccelerationStructure(GfxAccelerationStructure const &acceleration_structure)
    {
        if(!acceleration_structure)
            return kGfxResult_NoError;
        if(!acceleration_structure_handles_.has_handle(acceleration_structure.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid acceleration structure object");
        collect(acceleration_structures_[acceleration_structure]);  // release resources
        acceleration_structures_.erase(acceleration_structure); // destroy acceleration structure
        acceleration_structure_handles_.free_handle(acceleration_structure.handle);
        return kGfxResult_NoError;
    }

    GfxResult updateAccelerationStructure(GfxAccelerationStructure const &acceleration_structure)
    {
        void *data = nullptr;
        if(!acceleration_structure_handles_.has_handle(acceleration_structure.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot update an invalid acceleration structure object");
        AccelerationStructure &gfx_acceleration_structure = acceleration_structures_[acceleration_structure];
        if(!gfx_acceleration_structure.needs_update_ && !gfx_acceleration_structure.needs_rebuild_)
            return kGfxResult_NoError;  // no outstanding build requests, early out
        D3D12_GPU_VIRTUAL_ADDRESS const gpu_addr = allocateConstantMemory(gfx_acceleration_structure.raytracing_primitives_.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), data);
        D3D12_RAYTRACING_INSTANCE_DESC *instance_descs = (D3D12_RAYTRACING_INSTANCE_DESC *)data;
        uint32_t instance_desc_count = 0;
        if(gpu_addr == 0)
            return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to allocate for updating acceleration structure object with %u raytracing primitives", (uint32_t)gfx_acceleration_structure.raytracing_primitives_.size());
        active_raytracing_primitives_.reserve(GFX_MAX(active_raytracing_primitives_.capacity(), gfx_acceleration_structure.raytracing_primitives_.size()));
        active_raytracing_primitives_.clear();  // compact away discarded raytracing primitives
        for(size_t i = 0; i < gfx_acceleration_structure.raytracing_primitives_.size(); ++i)
        {
            GfxRaytracingPrimitive const &raytracing_primitive = gfx_acceleration_structure.raytracing_primitives_[i];
            if(!raytracing_primitive_handles_.has_handle(raytracing_primitive.handle))
                continue;   // invalid raytracing primitive object
            active_raytracing_primitives_.push_back(raytracing_primitive);
            RaytracingPrimitive const &gfx_raytracing_primitive = raytracing_primitives_[raytracing_primitive];
            if(!buffer_handles_.has_handle(gfx_raytracing_primitive.bvh_buffer_.handle))
                continue;   // no valid BVH memory, probably wasn't built
            Buffer const &gfx_buffer = buffers_[gfx_raytracing_primitive.bvh_buffer_];
            D3D12_RAYTRACING_INSTANCE_DESC instance_desc = {};
            for(uint32_t row = 0; row < 3; ++row)
                for(uint32_t col = 0; col < 3; ++col)
                    instance_desc.Transform[row][col] = gfx_raytracing_primitive.transform_[4 * row + col];
            for(uint32_t j = 0; j < 3; ++j)
                instance_desc.Transform[j][3] = gfx_raytracing_primitive.transform_[4 * j + 3];
            instance_desc.InstanceID = raytracing_primitive;
            instance_desc.InstanceMask = 0xFFu;
            instance_desc.AccelerationStructure = gfx_buffer.resource_->GetGPUVirtualAddress() + gfx_buffer.data_offset_;
            instance_descs[instance_desc_count++] = instance_desc;
        }
        std::swap(active_raytracing_primitives_, gfx_acceleration_structure.raytracing_primitives_);
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlas_inputs = {};
        tlas_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        tlas_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        tlas_inputs.NumDescs = instance_desc_count;
        tlas_inputs.InstanceDescs = gpu_addr;
        if(!gfx_acceleration_structure.needs_rebuild_ && gfx_acceleration_structure.bvh_buffer_)
            tlas_inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        gfx_acceleration_structure.needs_update_ = gfx_acceleration_structure.needs_rebuild_ = false;
        if(instance_desc_count == 0)
        {
            destroyBuffer(gfx_acceleration_structure.bvh_buffer_);
            gfx_acceleration_structure.bvh_buffer_ = {};
            return kGfxResult_NoError;
        }
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlas_info = {};
        device_->GetRaytracingAccelerationStructurePrebuildInfo(&tlas_inputs, &tlas_info);
        uint64_t const scratch_data_size = GFX_MAX(tlas_info.ScratchDataSizeInBytes, tlas_info.UpdateScratchDataSizeInBytes);
        GFX_TRY(allocateRaytracingScratch(scratch_data_size));  // ensure scratch is large enough
        uint64_t const bvh_data_size = GFX_ALIGN(tlas_info.ResultDataMaxSizeInBytes, 65536);
        if(bvh_data_size > gfx_acceleration_structure.bvh_buffer_.size)
        {
            destroyBuffer(gfx_acceleration_structure.bvh_buffer_);
            tlas_inputs.Flags &= ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
            gfx_acceleration_structure.bvh_buffer_ = createBuffer(bvh_data_size, nullptr, kGfxCpuAccess_None, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
            if(!gfx_acceleration_structure.bvh_buffer_)
                return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to create acceleration structure buffer");
        }
        GFX_ASSERT(buffer_handles_.has_handle(gfx_acceleration_structure.bvh_buffer_.handle));
        GFX_ASSERT(buffer_handles_.has_handle(raytracing_scratch_buffer_.handle));
        Buffer &gfx_buffer = buffers_[gfx_acceleration_structure.bvh_buffer_];
        Buffer &gfx_scratch_buffer = buffers_[raytracing_scratch_buffer_];
        SetObjectName(gfx_buffer, acceleration_structure.name);
        transitionResource(gfx_scratch_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        submitPipelineBarriers();   // ensure scratch is not in use
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};
        build_desc.DestAccelerationStructureData = gfx_buffer.resource_->GetGPUVirtualAddress() + gfx_buffer.data_offset_;
        build_desc.Inputs = tlas_inputs;
        if((tlas_inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) != 0)
            build_desc.SourceAccelerationStructureData = gfx_buffer.resource_->GetGPUVirtualAddress() + gfx_buffer.data_offset_;
        build_desc.ScratchAccelerationStructureData = gfx_scratch_buffer.resource_->GetGPUVirtualAddress() + gfx_scratch_buffer.data_offset_;
        command_list_->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);
        return kGfxResult_NoError;
    }

    uint32_t getAccelerationStructureRaytracingPrimitiveCount(GfxAccelerationStructure const &acceleration_structure)
    {
        if(!acceleration_structure_handles_.has_handle(acceleration_structure.handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Cannot get the raytracing primitives of an invalid acceleration structure object");
            return 0;
        }
        AccelerationStructure const &gfx_acceleration_structure = acceleration_structures_[acceleration_structure];
        return (uint32_t)gfx_acceleration_structure.raytracing_primitives_.size();
    }

    GfxRaytracingPrimitive const *getAccelerationStructureRaytracingPrimitives(GfxAccelerationStructure const &acceleration_structure)
    {
        if(!acceleration_structure_handles_.has_handle(acceleration_structure.handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Cannot get the raytracing primitives of an invalid acceleration structure object");
            return nullptr;
        }
        AccelerationStructure const &gfx_acceleration_structure = acceleration_structures_[acceleration_structure];
        return (!gfx_acceleration_structure.raytracing_primitives_.empty() ? gfx_acceleration_structure.raytracing_primitives_.data() : nullptr);
    }

    GfxRaytracingPrimitive createRaytracingPrimitive(GfxAccelerationStructure const &acceleration_structure)
    {
        GfxRaytracingPrimitive raytracing_primitive = {};
        if(!acceleration_structure_handles_.has_handle(acceleration_structure.handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Cannot create a raytracing primitive using an invalid acceleration structure object");
            return raytracing_primitive;
        }
        raytracing_primitive.handle = raytracing_primitive_handles_.allocate_handle();
        RaytracingPrimitive &gfx_raytracing_primitive = raytracing_primitives_.insert(raytracing_primitive);
        AccelerationStructure &gfx_acceleration_structure = acceleration_structures_[acceleration_structure];
        gfx_raytracing_primitive.index_ = (uint32_t)gfx_acceleration_structure.raytracing_primitives_.size();
        for(uint32_t i = 0; i < ARRAYSIZE(gfx_raytracing_primitive.transform_); ++i)
            gfx_raytracing_primitive.transform_[i] = ((i & 3) == (i >> 2) ? 1.0f : 0.0f);
        gfx_acceleration_structure.raytracing_primitives_.push_back(raytracing_primitive);
        gfx_raytracing_primitive.acceleration_structure = acceleration_structure;
        gfx_acceleration_structure.needs_rebuild_ = true;
        return raytracing_primitive;
    }

    GfxResult destroyRaytracingPrimitive(GfxRaytracingPrimitive const &raytracing_primitive)
    {
        if(!raytracing_primitive)
            return kGfxResult_NoError;
        if(!raytracing_primitive_handles_.has_handle(raytracing_primitive.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid raytracing primitive object");
        RaytracingPrimitive const &gfx_raytracing_primitive = raytracing_primitives_[raytracing_primitive];
        if(acceleration_structure_handles_.has_handle(gfx_raytracing_primitive.acceleration_structure.handle))
            acceleration_structures_[gfx_raytracing_primitive.acceleration_structure].needs_rebuild_ = true;
        collect(gfx_raytracing_primitive);  // release resources
        raytracing_primitives_.erase(raytracing_primitive); // destroy raytracing primitive
        raytracing_primitive_handles_.free_handle(raytracing_primitive.handle);
        return kGfxResult_NoError;
    }

    GfxResult buildRaytracingPrimitive(GfxRaytracingPrimitive const &raytracing_primitive, GfxBuffer const &vertex_buffer, uint32_t vertex_stride)
    {
        if(!raytracing_primitive_handles_.has_handle(raytracing_primitive.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set geometry on an invalid raytracing primitive object");
        if(!buffer_handles_.has_handle(vertex_buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot build a raytracing primitive using an invalid vertex buffer object");
        vertex_stride = (vertex_stride != 0 ? vertex_stride : vertex_buffer.stride);
        if(vertex_stride == 0)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot build a raytracing primitive with a vertex buffer object of stride `0'");
        if(vertex_buffer.size / vertex_stride > 0xFFFFFFFFull)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot build a raytracing primitive with a buffer object containing more than 4 billion vertices");
        RaytracingPrimitive &gfx_raytracing_primitive = raytracing_primitives_[raytracing_primitive];
        gfx_raytracing_primitive.vertex_buffer_ = vertex_buffer;
        gfx_raytracing_primitive.vertex_stride_ = vertex_stride;
        return buildRaytracingPrimitive(raytracing_primitive, gfx_raytracing_primitive, false);
    }

    GfxResult buildRaytracingPrimitive(GfxRaytracingPrimitive const &raytracing_primitive, GfxBuffer const &index_buffer, GfxBuffer const &vertex_buffer, uint32_t vertex_stride)
    {
        if(!raytracing_primitive_handles_.has_handle(raytracing_primitive.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set geometry on an invalid raytracing primitive object");
        if(!buffer_handles_.has_handle(index_buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot build a raytracing primitive using an invalid index buffer object");
        if(!buffer_handles_.has_handle(vertex_buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot build a raytracing primitive using an invalid vertex buffer object");
        uint32_t const index_stride = (index_buffer.stride == 2 ? 2 : 4);
        if(index_buffer.size / index_stride > 0xFFFFFFFFull)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot build a raytracing primitive with a buffer object containing more than 4 billion indices");
        vertex_stride = (vertex_stride != 0 ? vertex_stride : vertex_buffer.stride);
        if(vertex_stride == 0)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot build a raytracing primitive with a vertex buffer object of stride `0'");
        if(vertex_buffer.size / vertex_stride > 0xFFFFFFFFull)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot build a raytracing primitive with a buffer object containing more than 4 billion vertices");
        RaytracingPrimitive &gfx_raytracing_primitive = raytracing_primitives_[raytracing_primitive];
        gfx_raytracing_primitive.index_buffer_ = index_buffer;
        gfx_raytracing_primitive.index_stride_ = index_stride;
        gfx_raytracing_primitive.vertex_buffer_ = vertex_buffer;
        gfx_raytracing_primitive.vertex_stride_ = vertex_stride;
        return buildRaytracingPrimitive(raytracing_primitive, gfx_raytracing_primitive, false);
    }

    GfxResult setRaytracingPrimitiveTransform(GfxRaytracingPrimitive const &raytracing_primitive, float const *row_major_4x4_transform)
    {
        if(!raytracing_primitive_handles_.has_handle(raytracing_primitive.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set transform on an invalid raytracing primitive object");
        if(row_major_4x4_transform == nullptr)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot pass `nullptr' as the transform of a raytracing primitive object");
        RaytracingPrimitive &gfx_raytracing_primitive = raytracing_primitives_[raytracing_primitive];
        GFX_TRY(updateRaytracingPrimitive(raytracing_primitive, gfx_raytracing_primitive));
        memcpy(gfx_raytracing_primitive.transform_, row_major_4x4_transform, sizeof(gfx_raytracing_primitive.transform_));
        return kGfxResult_NoError;
    }

    GfxResult updateRaytracingPrimitive(GfxRaytracingPrimitive const &raytracing_primitive)
    {
        if(!raytracing_primitive_handles_.has_handle(raytracing_primitive.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot update an invalid raytracing primitive object");
        RaytracingPrimitive &gfx_raytracing_primitive = raytracing_primitives_[raytracing_primitive];
        return buildRaytracingPrimitive(raytracing_primitive, gfx_raytracing_primitive, true);
    }

    GfxProgram createProgram(char const *file_name, char const *file_path)
    {
        GfxProgram program = {};
        if(!file_name)
            return program; // invalid parameter
        file_path = (file_path != nullptr ? file_path : ".");
        char const last_char = file_path[strlen(file_path) - 1];
        char const *path_separator = (last_char == '/' || last_char == '\\' ? "" : "/");
        GFX_SNPRINTF(program.name, sizeof(program.name), "%s%s%s", file_path, path_separator, file_name);
        program.handle = program_handles_.allocate_handle();
        Program &gfx_program = programs_.insert(program);
        gfx_program.file_name_ = file_name;
        gfx_program.file_path_ = file_path;
        return program;
    }

    GfxProgram createProgram(GfxProgramDesc const &program_desc, char const *name)
    {
        GfxProgram program = {};
        program.handle = program_handles_.allocate_handle();
        if(name)
            GFX_SNPRINTF(program.name, sizeof(program.name), "%s", name);
        else
            GFX_SNPRINTF(program.name, sizeof(program.name), "gfx_Program%llu", program.handle);
        Program &gfx_program = programs_.insert(program);
        gfx_program.file_path_ = program.name;
        gfx_program.cs_ = program_desc.cs;
        gfx_program.vs_ = program_desc.vs;
        gfx_program.gs_ = program_desc.gs;
        gfx_program.ps_ = program_desc.ps;
        return program;
    }

    GfxResult destroyProgram(GfxProgram const &program)
    {
        if(!program.handle)
            return kGfxResult_NoError;
        if(!program_handles_.has_handle(program.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid program object");
        collect(programs_[program]);    // release resources
        programs_.erase(program);   // destroy program
        program_handles_.free_handle(program.handle);
        return kGfxResult_NoError;
    }

    GfxResult setProgramBuffer(GfxProgram const &program, char const *parameter_name, GfxBuffer const &buffer)
    {
        if(!program_handles_.has_handle(program.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot set a parameter onto an invalid program object");
        if(!parameter_name || !*parameter_name)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter with an invalid name");
        if(!buffer_handles_.has_handle(buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter to an invalid buffer object");
        Program &gfx_program = programs_[program];  // get hold of program object
        gfx_program.insertParameter(parameter_name).set(buffer);
        return kGfxResult_NoError;
    }

    GfxResult setProgramTexture(GfxProgram const &program, char const *parameter_name, GfxTexture const &texture, uint32_t mip_level)
    {
        if(!program_handles_.has_handle(program.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot set a parameter onto an invalid program object");
        if(!parameter_name || !*parameter_name)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter with an invalid name");
        if(!texture_handles_.has_handle(texture.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter to an invalid texture object");
        Program &gfx_program = programs_[program];  // get hold of program object
        gfx_program.insertParameter(parameter_name).set(texture, mip_level);
        return kGfxResult_NoError;
    }

    GfxResult setProgramTextures(GfxProgram const &program, char const *parameter_name, GfxTexture const *textures, uint32_t texture_count)
    {
        if(!program_handles_.has_handle(program.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot set a parameter onto an invalid program object");
        if(!parameter_name || !*parameter_name)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter with an invalid name");
        if(textures == nullptr && texture_count > 0)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter to an invalid array of textures");
        Program &gfx_program = programs_[program];  // get hold of program object
        switch(texture_count)
        {
        case 0:
            gfx_program.eraseParameter(parameter_name);
            break;
        case 1:
            gfx_program.insertParameter(parameter_name).set(*textures, 0);
            break;
        default:
            gfx_program.insertParameter(parameter_name).set(textures, texture_count);
            break;
        }
        return kGfxResult_NoError;
    }

    GfxResult setProgramSamplerState(GfxProgram const &program, char const *parameter_name, GfxSamplerState const &sampler_state)
    {
        if(!program_handles_.has_handle(program.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot set a parameter onto an invalid program object");
        if(!parameter_name || !*parameter_name)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter with an invalid name");
        if(!sampler_state_handles_.has_handle(sampler_state.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter to an invalid sampler state object");
        Program &gfx_program = programs_[program];  // get hold of program object
        gfx_program.insertParameter(parameter_name).set(sampler_state);
        return kGfxResult_NoError;
    }

    GfxResult setProgramAccelerationStructure(GfxProgram const &program, char const *parameter_name, GfxAccelerationStructure const &acceleration_structure)
    {
        if(!program_handles_.has_handle(program.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot set a parameter onto an invalid program object");
        if(!parameter_name || !*parameter_name)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter with an invalid name");
        if(!acceleration_structure_handles_.has_handle(acceleration_structure.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter to an invalid acceleration structure object");
        Program &gfx_program = programs_[program];  // get hold of program object
        gfx_program.insertParameter(parameter_name).set(acceleration_structure);
        return kGfxResult_NoError;
    }

    GfxResult setProgramConstants(GfxProgram const &program, char const *parameter_name, void const *data, uint32_t data_size)
    {
        if(!program_handles_.has_handle(program.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot set a parameter onto an invalid program object");
        if(!parameter_name || !*parameter_name)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter with an invalid name");
        if(!data && data_size > 0)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set a program parameter to a null pointer");
        Program &gfx_program = programs_[program];  // get hold of program object
        gfx_program.insertParameter(parameter_name).set(data, data_size);
        return kGfxResult_NoError;
    }

    GfxKernel createComputeKernel(GfxProgram const &program, char const *entry_point, char const **defines, uint32_t define_count)
    {
        GfxKernel compute_kernel = {};
        if(!program_handles_.has_handle(program.handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot create a compute kernel using an invalid program object");
            return compute_kernel;
        }
        compute_kernel.type = GfxKernel::kType_Compute;
        Program const &gfx_program = programs_[program];
        entry_point = (entry_point ? entry_point : "main");
        GFX_SNPRINTF(compute_kernel.name, sizeof(compute_kernel.name), "%s", entry_point);
        compute_kernel.handle = kernel_handles_.allocate_handle();
        Kernel &gfx_kernel = kernels_.insert(compute_kernel);
        gfx_kernel.program_ = program;
        gfx_kernel.entry_point_ = entry_point;
        GFX_ASSERT(define_count == 0 || defines != nullptr);
        for(uint32_t i = 0; i < define_count; ++i) gfx_kernel.defines_.push_back(defines[i]);
        gfx_kernel.num_threads_ = (uint32_t *)malloc(3 * sizeof(uint32_t)); for(uint32_t i = 0; i < 3; ++i) gfx_kernel.num_threads_[i] = 1;
        createKernel(gfx_program, gfx_kernel);  // create compute kernel
        if(!gfx_program.file_name_ && (gfx_kernel.root_signature_ == nullptr || gfx_kernel.pipeline_state_ == nullptr))
        {
            destroyKernel(compute_kernel);
            compute_kernel = {};    // invalidate handle
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Failed to create compute kernel object `%s' using program `%s'", entry_point, gfx_program.file_path_.c_str());
            return compute_kernel;
        }
        return compute_kernel;
    }

    GfxKernel createGraphicsKernel(GfxProgram const &program, GfxDrawState const &draw_state, char const *entry_point, char const **defines, uint32_t define_count)
    {
        GfxKernel graphics_kernel = {};
        if(!program_handles_.has_handle(program.handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot create a graphics kernel using an invalid program object");
            return graphics_kernel;
        }
        GFX_ASSERT(define_count == 0 || defines != nullptr);
        uint32_t const draw_state_index = static_cast<uint32_t>(draw_state.handle & 0xFFFFFFFFull);
        DrawState const *gfx_draw_state = draw_states_.at(draw_state_index);
        if(!gfx_draw_state)
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot create a graphics kernel using an invalid draw state object");
            return graphics_kernel;
        }
        Program const &gfx_program = programs_[program];
        graphics_kernel.type = GfxKernel::kType_Graphics;
        entry_point = (entry_point ? entry_point : "main");
        GFX_SNPRINTF(graphics_kernel.name, sizeof(graphics_kernel.name), "%s", entry_point);
        graphics_kernel.handle = kernel_handles_.allocate_handle();
        Kernel &gfx_kernel = kernels_.insert(graphics_kernel);
        gfx_kernel.program_ = program;
        gfx_kernel.entry_point_ = entry_point;
        gfx_kernel.draw_state_ = gfx_draw_state->draw_state_;
        for(uint32_t i = 0; i < define_count; ++i) gfx_kernel.defines_.push_back(defines[i]);
        gfx_kernel.num_threads_ = (uint32_t *)malloc(3 * sizeof(uint32_t)); for(uint32_t i = 0; i < 3; ++i) gfx_kernel.num_threads_[i] = 0;
        createKernel(gfx_program, gfx_kernel);  // create graphics kernel
        if(!gfx_program.file_name_ && (gfx_kernel.root_signature_ == nullptr || gfx_kernel.pipeline_state_ == nullptr))
        {
            destroyKernel(graphics_kernel);
            graphics_kernel = {};   // invalidate handle
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Failed to create graphics kernel object `%s' using program `%s'", entry_point, gfx_program.file_path_.c_str());
            return graphics_kernel;
        }
        return graphics_kernel;
    }

    GfxResult destroyKernel(GfxKernel const &kernel)
    {
        if(!kernel)
            return kGfxResult_NoError;
        if(!kernel_handles_.has_handle(kernel.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid kernel object");
        collect(kernels_[kernel]);  // release resources
        kernels_.erase(kernel); // destroy kernel
        kernel_handles_.free_handle(kernel.handle);
        return kGfxResult_NoError;
    }

    uint32_t const *getKernelNumThreads(GfxKernel const &kernel)
    {
        if(!kernel_handles_.has_handle(kernel.handle))
            return nullptr; // invalid kernel object
        Kernel const &gfx_kernel = kernels_[kernel];
        GFX_ASSERT(gfx_kernel.num_threads_ != nullptr);
        return gfx_kernel.num_threads_;
    }

    GfxResult reloadAllKernels()
    {
        for(uint32_t i = 0; i < kernels_.size(); ++i)
            reloadKernel(kernels_.data()[i]);
        return kGfxResult_NoError;
    }

    GfxResult encodeCopyBuffer(GfxBuffer const &dst, GfxBuffer const &src)
    {
        if(!buffer_handles_.has_handle(dst.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot copy into an invalid buffer object");
        if(!buffer_handles_.has_handle(src.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot copy from an invalid buffer object");
        if(dst.size != src.size)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy between buffer objects of different size");
        if(dst.cpu_access == kGfxCpuAccess_Write)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy to a buffer object with write CPU access");
        if(src.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy from a buffer object with read CPU access");
        if(dst.size == 0) return kGfxResult_NoError;    // nothing to copy
        Buffer &gfx_dst = buffers_[dst], &gfx_src = buffers_[src];
        SetObjectName(gfx_dst, dst.name); SetObjectName(gfx_src, src.name);
        if(gfx_dst.resource_ == gfx_src.resource_)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy between two buffer objects that are pointing at the same resource; use an intermediate buffer");
        if(dst.cpu_access == kGfxCpuAccess_None) transitionResource(gfx_dst, D3D12_RESOURCE_STATE_COPY_DEST);
        if(src.cpu_access == kGfxCpuAccess_None) transitionResource(gfx_src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        submitPipelineBarriers();   // transition our resources if needed
        command_list_->CopyBufferRegion(gfx_dst.resource_, gfx_dst.data_offset_,
                                        gfx_src.resource_, gfx_src.data_offset_,
                                        dst.size);
        return kGfxResult_NoError;
    }

    GfxResult encodeCopyBuffer(GfxBuffer const &dst, uint64_t dst_offset, GfxBuffer const &src, uint64_t src_offset, uint64_t size)
    {
        if(!buffer_handles_.has_handle(dst.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot copy into an invalid buffer object");
        if(!buffer_handles_.has_handle(src.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot copy from an invalid buffer object");
        if(dst_offset + size > dst.size || src_offset + size > src.size)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy between regions that are not fully contained within their respective buffer objects");
        if(dst.cpu_access == kGfxCpuAccess_Write)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy to a buffer object with write CPU access");
        if(src.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy from a buffer object with read CPU access");
        if(size == 0) return kGfxResult_NoError;    // nothing to copy
        Buffer &gfx_dst = buffers_[dst], &gfx_src = buffers_[src];
        SetObjectName(gfx_dst, dst.name); SetObjectName(gfx_src, src.name);
        if(gfx_dst.resource_ == gfx_src.resource_)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy between two buffer objects that are pointing at the same resource; use an intermediate buffer");
        if(dst.cpu_access == kGfxCpuAccess_None) transitionResource(gfx_dst, D3D12_RESOURCE_STATE_COPY_DEST);
        if(src.cpu_access == kGfxCpuAccess_None) transitionResource(gfx_src, D3D12_RESOURCE_STATE_COPY_SOURCE);
        submitPipelineBarriers();   // transition our resources if needed
        command_list_->CopyBufferRegion(gfx_dst.resource_, gfx_dst.data_offset_ + dst_offset,
                                        gfx_src.resource_, gfx_src.data_offset_ + src_offset,
                                        size);
        return kGfxResult_NoError;
    }

    GfxResult encodeClearBuffer(GfxBuffer const &buffer, uint32_t clear_value)
    {
        GfxResult result = kGfxResult_NoError;
        if(!buffer_handles_.has_handle(buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot clear an invalid buffer object");
        if(buffer.size == 0) return kGfxResult_NoError; // nothing to clear
        uint64_t const data_size = GFX_ALIGN(buffer.size, 4);
        uint64_t const num_uints = data_size / sizeof(uint32_t);
        switch(buffer.cpu_access)
        {
        case kGfxCpuAccess_Write:
            {
                Buffer &gfx_buffer = buffers_[buffer];
                SetObjectName(gfx_buffer, buffer.name);
                uint32_t *data = (uint32_t *)gfx_buffer.data_;
                for(uint64_t i = 0; i < num_uints; ++i) data[i] = clear_value;
            }
            break;
        case kGfxCpuAccess_Read:
            {
                void *data = nullptr;
                D3D12_GPU_VIRTUAL_ADDRESS const gpu_addr = allocateConstantMemory(data_size, data);
                if(data == nullptr)
                    return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to clear buffer object");
                if(!issued_clear_buffer_warning_)
                    GFX_PRINTLN("Warning: It is inefficient to clear a buffer object with read CPU access; prefer a copy instead");
                issued_clear_buffer_warning_ = true;    // we've now warned the user...
                for(uint64_t i = 0; i < num_uints; ++i) ((uint32_t *)data)[i] = clear_value;
                Buffer &dst_buffer = buffers_[buffer], &src_buffer = buffers_[constant_buffer_pool_[fence_index_]];
                SetObjectName(dst_buffer, buffer.name);
                command_list_->CopyBufferRegion(dst_buffer.resource_, dst_buffer.data_offset_,
                    src_buffer.resource_, gpu_addr - src_buffer.resource_->GetGPUVirtualAddress(), data_size);
            }
            break;
        default:    // kGfxCpuAccess_None
            {
                GfxKernel const bound_kernel = bound_kernel_;
                setProgramBuffer(clear_buffer_program_, "OutputBuffer", buffer);
                setProgramConstants(clear_buffer_program_, "ClearValue", &clear_value, sizeof(clear_value));
                uint32_t const group_size = *getKernelNumThreads(clear_buffer_kernel_);
                uint32_t const num_groups = (uint32_t)((num_uints + group_size - 1) / group_size);
                GFX_TRY(encodeBindKernel(clear_buffer_kernel_));
                result = encodeDispatch(num_groups, 1, 1);
                if(kernel_handles_.has_handle(bound_kernel.handle))
                    encodeBindKernel(bound_kernel);
                else
                    bound_kernel_ = bound_kernel;
            }
            break;
        }
        if(result != kGfxResult_NoError)
            return GFX_SET_ERROR(result, "Failed to clear buffer object");
        return kGfxResult_NoError;
    }

    GfxResult encodeClearBackBuffer()
    {
        float const clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        D3D12_RECT
        rect        = {};
        rect.right  = window_width_;
        rect.bottom = window_height_;
        command_list_->ClearRenderTargetView(rtv_descriptors_.getCPUHandle(back_buffer_rtvs_[fence_index_]), clear_color, 1, &rect);
        return kGfxResult_NoError;
    }

    GfxResult encodeClearTexture(GfxTexture const &texture)
    {
        uint32_t depth = texture.depth;
        for(uint32_t i = 0; i < texture.mip_levels; ++i)
        {
            for(uint32_t j = 0; j < depth; ++j)
                GFX_TRY(encodeClearImage(texture, i, j));
            if(texture.is3D())
                depth = GFX_MAX(depth >> 1, 1u);
        }
        return kGfxResult_NoError;
    }

    GfxResult encodeCopyTexture(GfxTexture const &dst, GfxTexture const &src)
    {
        if(!texture_handles_.has_handle(dst.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot copy to an invalid texture object");
        if(!texture_handles_.has_handle(src.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot copy from an invalid texture object");
        Texture &dst_texture = textures_[dst]; SetObjectName(dst_texture, dst.name);
        Texture &src_texture = textures_[src]; SetObjectName(src_texture, src.name);
        transitionResource(dst_texture, D3D12_RESOURCE_STATE_COPY_DEST);
        transitionResource(src_texture, D3D12_RESOURCE_STATE_COPY_SOURCE);
        submitPipelineBarriers();   // transition our resources if needed
        command_list_->CopyResource(dst_texture.resource_, src_texture.resource_);
        return kGfxResult_NoError;
    }

    GfxResult encodeClearImage(GfxTexture const &texture, uint32_t mip_level, uint32_t slice)
    {
        if(!texture_handles_.has_handle(texture.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot clear an invalid texture object");
        if(mip_level >= texture.mip_levels)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot clear non-existing mip level %u", mip_level);
        if(slice >= (texture.is3D() ? GFX_MAX(texture.depth >> mip_level, 1u) : texture.depth))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot clear non-existing slice %u", slice);
        Texture &gfx_texture = textures_[texture];
        SetObjectName(gfx_texture, texture.name);
        if(IsDepthStencilFormat(texture.format))
        {
            D3D12_CLEAR_FLAGS clear_flags = D3D12_CLEAR_FLAG_DEPTH;
            if(HasStencilBits(texture.format)) clear_flags |= D3D12_CLEAR_FLAG_STENCIL;
            GFX_TRY(ensureTextureHasDepthStencilView(texture, gfx_texture, mip_level, slice));
            GFX_ASSERT(gfx_texture.dsv_descriptor_slots_[mip_level][slice] != 0xFFFFFFFFu);
            D3D12_RESOURCE_DESC const resource_desc = gfx_texture.resource_->GetDesc();
            D3D12_RECT
            rect        = {};
            rect.right  = (uint32_t)resource_desc.Width;
            rect.bottom = (uint32_t)resource_desc.Height;
            transitionResource(gfx_texture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            submitPipelineBarriers();   // transition our resources if needed
            command_list_->ClearDepthStencilView(dsv_descriptors_.getCPUHandle(gfx_texture.dsv_descriptor_slots_[mip_level][slice]), clear_flags, 1.0f, 0, 1, &rect);
        }
        else
        {
            float const clear_color[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            GFX_TRY(ensureTextureHasRenderTargetView(texture, gfx_texture, mip_level, slice));
            GFX_ASSERT(gfx_texture.rtv_descriptor_slots_[mip_level][slice] != 0xFFFFFFFFu);
            D3D12_RESOURCE_DESC const resource_desc = gfx_texture.resource_->GetDesc();
            D3D12_RECT
            rect        = {};
            rect.right  = (uint32_t)resource_desc.Width;
            rect.bottom = (uint32_t)resource_desc.Height;
            transitionResource(gfx_texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
            submitPipelineBarriers();   // transition our resources if needed
            command_list_->ClearRenderTargetView(rtv_descriptors_.getCPUHandle(gfx_texture.rtv_descriptor_slots_[mip_level][slice]), clear_color, 1, &rect);
        }
        return kGfxResult_NoError;
    }

    GfxResult encodeCopyBufferToTexture(GfxTexture const &dst, GfxBuffer const &src)
    {
        if(!texture_handles_.has_handle(dst.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot copy to an invalid texture object");
        if(!buffer_handles_.has_handle(src.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot copy from an invalid buffer object");
        if(!dst.is2D()) // TODO: implement for the other texture types (gboisse)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy from buffer to a non-2D texture object");
        if(src.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy from a buffer object with read CPU access");
        if(src.size == 0)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy to texture using an empty buffer object");
        Texture &gfx_texture = textures_[dst]; SetObjectName(gfx_texture, dst.name);
        uint32_t num_rows[D3D12_REQ_MIP_LEVELS] = {};
        uint64_t row_sizes[D3D12_REQ_MIP_LEVELS] = {};
        D3D12_RESOURCE_DESC const resource_desc = gfx_texture.resource_->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresource_footprints[D3D12_REQ_MIP_LEVELS] = {};
        device_->GetCopyableFootprints(&resource_desc, 0, dst.mip_levels, 0, subresource_footprints, num_rows, row_sizes, nullptr);
        uint64_t buffer_offset = 0;
        uint32_t pixels_per_block = 1;
        uint32_t const bytes_per_pixel = GetBytesPerPixel(dst.format);
        if(bytes_per_pixel == 0)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy to texture object of unsupported format");
        if(dst.format >= DXGI_FORMAT_BC1_TYPELESS && dst.format <= DXGI_FORMAT_BC5_SNORM)
        {
            pixels_per_block = 4 * 4;   // BC formats have 4*4 pixels per block
            pixels_per_block /= 4;  // we need to divide by 4 because GetCopyableFootprints introduces a *2 stride divides the rows /4
        }
        for(uint32_t mip_level = 0; mip_level < dst.mip_levels; ++mip_level)
        {
            Buffer *texture_upload_buffer = nullptr;
            if(buffer_offset >= src.size)
                break;  // further mips aren't available
            GFX_ASSERT(num_rows[mip_level] == GFX_MAX(dst.height >> mip_level, 1u));
            uint32_t const buffer_row_pitch = (GFX_MAX(dst.width >> mip_level, 1u) * bytes_per_pixel) / pixels_per_block;
            uint32_t const texture_row_pitch = subresource_footprints[mip_level].Footprint.RowPitch;
            uint64_t const buffer_size = GFX_MAX(dst.height >> mip_level, 1u) * buffer_row_pitch;
            if(buffer_offset + buffer_size > src.size)
                return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot copy to mip level %u from buffer object with insufficient storage", mip_level);
            if(buffer_row_pitch != texture_row_pitch || // we must respect the 256-byte pitch alignment
               GFX_ALIGN(buffer_offset, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) != buffer_offset)
            {
                uint64_t texture_upload_buffer_size = num_rows[mip_level] * static_cast<uint64_t>(subresource_footprints[mip_level].Footprint.RowPitch);
                if(texture_upload_buffer_.size < texture_upload_buffer_size)
                {
                    destroyBuffer(texture_upload_buffer_);
                    texture_upload_buffer_size += (texture_upload_buffer_size + 2) >> 1;
                    texture_upload_buffer_size = GFX_ALIGN(texture_upload_buffer_size, 65536);
                    texture_upload_buffer_ = createBuffer(texture_upload_buffer_size, nullptr, kGfxCpuAccess_None);
                    if(!texture_upload_buffer_)
                        return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to allocate memory to upload texture data");
                    texture_upload_buffer_.setName("gfx_TextureUploadBuffer");
                }
                texture_upload_buffer = &buffers_[texture_upload_buffer_];
                SetObjectName(*texture_upload_buffer, texture_upload_buffer_.name);
                Buffer &gfx_buffer = buffers_[src]; SetObjectName(gfx_buffer, src.name);
                if(src.cpu_access == kGfxCpuAccess_None) transitionResource(gfx_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
                transitionResource(*texture_upload_buffer, D3D12_RESOURCE_STATE_COPY_DEST);
                submitPipelineBarriers();   // transition our resources if needed
                for(uint32_t i = 0; i < num_rows[mip_level]; ++i)
                    command_list_->CopyBufferRegion(texture_upload_buffer->resource_, i * texture_row_pitch, gfx_buffer.resource_, i * buffer_row_pitch, buffer_row_pitch);
            }
            Buffer &gfx_buffer = buffers_[src]; SetObjectName(gfx_buffer, src.name);
            if(texture_upload_buffer != nullptr) transitionResource(*texture_upload_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
            else if(src.cpu_access == kGfxCpuAccess_None) transitionResource(gfx_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
            transitionResource(gfx_texture, D3D12_RESOURCE_STATE_COPY_DEST);
            submitPipelineBarriers();   // transition our resources if needed
            {
                D3D12_TEXTURE_COPY_LOCATION dst_location = {};
                D3D12_TEXTURE_COPY_LOCATION src_location = {};
                dst_location.pResource = gfx_texture.resource_;
                dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dst_location.SubresourceIndex = mip_level;  // copy to mip level
                src_location.pResource = (texture_upload_buffer == nullptr ? gfx_buffer.resource_ : texture_upload_buffer->resource_);
                src_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                src_location.PlacedFootprint.Footprint = subresource_footprints[mip_level].Footprint;
                src_location.PlacedFootprint.Offset = (texture_upload_buffer == nullptr ? buffer_offset : 0);
                command_list_->CopyTextureRegion(&dst_location, 0, 0, 0, &src_location, nullptr);
            }
            buffer_offset += buffer_size;   // advance the buffer offset
        }
        return kGfxResult_NoError;
    }

    GfxResult encodeGenerateMips(GfxTexture const &texture)
    {
        GfxResult result = kGfxResult_NoError;
        if(!texture_handles_.has_handle(texture.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot generate mips of an invalid texture object");
        if(!texture.is2D()) // TODO: implement for the other texture types (gboisse)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot generate mips of a non-2D texture object");
        if(texture.mip_levels <= 1) return kGfxResult_NoError;  // nothing to generate
        Texture &gfx_texture = textures_[texture];
        SetObjectName(gfx_texture, texture.name);
        GfxKernel const bound_kernel = bound_kernel_;
        GFX_TRY(encodeBindKernel(generate_mips_kernel_));
        for(uint32_t mip_level = 1; mip_level < texture.mip_levels; ++mip_level)
        {
            setProgramTexture(generate_mips_program_, "InputBuffer", texture, mip_level - 1);
            setProgramTexture(generate_mips_program_, "OutputBuffer", texture, mip_level);
            uint32_t const *num_threads = getKernelNumThreads(generate_mips_kernel_);
            uint32_t const num_groups_x = (GFX_MAX(texture.width  >> mip_level, 1u) + num_threads[0] - 1) / num_threads[0];
            uint32_t const num_groups_y = (GFX_MAX(texture.height >> mip_level, 1u) + num_threads[1] - 1) / num_threads[1];
            result = encodeDispatch(num_groups_x, num_groups_y, 1);
            if(result != kGfxResult_NoError) break;
        }
        if(kernel_handles_.has_handle(bound_kernel.handle))
            encodeBindKernel(bound_kernel);
        else
            bound_kernel_ = bound_kernel;
        if(result != kGfxResult_NoError)
            return GFX_SET_ERROR(result, "Failed to generate texture mips");
        return kGfxResult_NoError;
    }

    GfxResult encodeBindKernel(GfxKernel const &kernel)
    {
        if(!kernel_handles_.has_handle(kernel.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot bind invalid kernel object");
        if(bound_kernel_.handle == kernel.handle) return kGfxResult_NoError;    // already bound
        Kernel const &gfx_kernel = kernels_[kernel];
        if(gfx_kernel.root_signature_ != nullptr && gfx_kernel.pipeline_state_ != nullptr)
        {
            command_list_->SetPipelineState(gfx_kernel.pipeline_state_);
            if(gfx_kernel.isCompute())
                command_list_->SetComputeRootSignature(gfx_kernel.root_signature_);
            else
                command_list_->SetGraphicsRootSignature(gfx_kernel.root_signature_);
        }
        bound_kernel_ = kernel; // bind kernel
        return kGfxResult_NoError;
    }

    GfxResult encodeBindIndexBuffer(GfxBuffer const &index_buffer)
    {
        if(!buffer_handles_.has_handle(index_buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot bind index data from invalid buffer object");
        if(index_buffer.size > 0xFFFFFFFFull)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot bind an index buffer object that's larger than 4GiB");
        if(index_buffer.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot bind an index buffer object that has read CPU access");
        bound_index_buffer_ = index_buffer;
        return kGfxResult_NoError;
    }

    GfxResult encodeBindVertexBuffer(GfxBuffer const &vertex_buffer)
    {
        if(!buffer_handles_.has_handle(vertex_buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot bind vertex data from invalid buffer object");
        if(vertex_buffer.size > 0xFFFFFFFFull)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot bind a vertex buffer object that's larger than 4GiB");
        if(vertex_buffer.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot bind a vertex buffer object that has read CPU access");
        bound_vertex_buffer_ = vertex_buffer;
        return kGfxResult_NoError;
    }

    GfxResult encodeSetViewport(float x, float y, float width, float height)
    {
        if(isnan(x) || isnan(y) || isnan(width) || isnan(height))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set viewport using invalid floating-point numbers");
        viewport_.x_ = x;
        viewport_.y_ = y;
        viewport_.width_ = width;
        viewport_.height_ = height;
        return kGfxResult_NoError;
    }

    GfxResult encodeSetScissorRect(int32_t x, int32_t y, int32_t width, int32_t height)
    {
        if(width < 0 || height < 0)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set scissor rect using negative width/height values");
        scissor_rect_.x_ = x;
        scissor_rect_.y_ = y;
        scissor_rect_.width_ = width;
        scissor_rect_.height_ = height;
        return kGfxResult_NoError;
    }

    GfxResult encodeDraw(uint32_t vertex_count, uint32_t instance_count, uint32_t base_vertex, uint32_t base_instance)
    {
        if(vertex_count == 0 || instance_count == 0)
            return kGfxResult_NoError;  // nothing to draw
        if(!kernel_handles_.has_handle(bound_kernel_.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw when bound kernel object is invalid");
        Kernel &kernel = kernels_[bound_kernel_];
        if(kernel.isCompute())
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw using a compute kernel object");
        if(kernel.root_signature_ == nullptr || kernel.pipeline_state_ == nullptr)
            return kGfxResult_NoError;  // skip draw call
        GFX_TRY(installShaderState(kernel));
        submitPipelineBarriers();   // transition our resources if needed
        command_list_->DrawInstanced(vertex_count, instance_count, base_vertex, base_instance);
        return kGfxResult_NoError;
    }

    GfxResult encodeDrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t base_vertex, uint32_t base_instance)
    {
        if(index_count == 0 || instance_count == 0)
            return kGfxResult_NoError;  // nothing to draw
        if(!kernel_handles_.has_handle(bound_kernel_.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw when bound kernel object is invalid");
        Kernel &kernel = kernels_[bound_kernel_];
        if(kernel.isCompute())
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw using a compute kernel object");
        if(kernel.root_signature_ == nullptr || kernel.pipeline_state_ == nullptr)
            return kGfxResult_NoError;  // skip draw call
        GFX_TRY(installShaderState(kernel, true));
        submitPipelineBarriers();   // transition our resources if needed
        command_list_->DrawIndexedInstanced(index_count, instance_count, first_index, base_vertex, base_instance);
        return kGfxResult_NoError;
    }

    GfxResult encodeMultiDrawIndirect(GfxBuffer const &args_buffer, uint32_t args_count)
    {
        if(args_count == 0)
            return kGfxResult_NoError;  // nothing to draw
        if(!buffer_handles_.has_handle(args_buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot draw using an invalid arguments buffer object");
        if(args_buffer.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw using an arguments buffer object with read CPU access");
        if(!kernel_handles_.has_handle(bound_kernel_.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw when bound kernel object is invalid");
        Kernel &kernel = kernels_[bound_kernel_];
        if(kernel.isCompute())
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw using a compute kernel object");
        if(kernel.root_signature_ == nullptr || kernel.pipeline_state_ == nullptr)
            return kGfxResult_NoError;  // skip multi-draw call
        GFX_TRY(populateDrawIdBuffer(args_count));
        Buffer &gfx_buffer = buffers_[args_buffer];
        SetObjectName(gfx_buffer, args_buffer.name);
        // TODO: might need multiple resource state flags... (gboisse)
        GFX_TRY(installShaderState(kernel));
        if(args_buffer.cpu_access == kGfxCpuAccess_None)
            transitionResource(gfx_buffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        submitPipelineBarriers();   // transition our resources if needed
        command_list_->ExecuteIndirect(multi_draw_signature_, args_count, gfx_buffer.resource_, gfx_buffer.data_offset_, nullptr, 0);
        return kGfxResult_NoError;
    }

    GfxResult encodeMultiDrawIndexedIndirect(GfxBuffer const &args_buffer, uint32_t args_count)
    {
        if(args_count == 0)
            return kGfxResult_NoError;  // nothing to draw
        if(!buffer_handles_.has_handle(args_buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot draw using an invalid arguments buffer object");
        if(args_buffer.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw using an arguments buffer object with read CPU access");
        if(!kernel_handles_.has_handle(bound_kernel_.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw when bound kernel object is invalid");
        Kernel &kernel = kernels_[bound_kernel_];
        if(kernel.isCompute())
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw using a compute kernel object");
        if(kernel.root_signature_ == nullptr || kernel.pipeline_state_ == nullptr)
            return kGfxResult_NoError;  // skip multi-draw call
        GFX_TRY(populateDrawIdBuffer(args_count));
        Buffer &gfx_buffer = buffers_[args_buffer];
        SetObjectName(gfx_buffer, args_buffer.name);
        // TODO: might need multiple resource state flags... (gboisse)
        GFX_TRY(installShaderState(kernel, true));
        if(args_buffer.cpu_access == kGfxCpuAccess_None)
            transitionResource(gfx_buffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        submitPipelineBarriers();   // transition our resources if needed
        command_list_->ExecuteIndirect(multi_draw_indexed_signature_, args_count, gfx_buffer.resource_, gfx_buffer.data_offset_, nullptr, 0);
        return kGfxResult_NoError;
    }

    GfxResult encodeDispatch(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z)
    {
        if(!num_groups_x || !num_groups_y || !num_groups_z)
            return kGfxResult_NoError;  // nothing to dispatch
        if(!kernel_handles_.has_handle(bound_kernel_.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot dispatch when bound kernel object is invalid");
        Kernel &kernel = kernels_[bound_kernel_];
        if(!kernel.isCompute())
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot dispatch using a graphics kernel object");
        if(kernel.root_signature_ == nullptr || kernel.pipeline_state_ == nullptr)
            return kGfxResult_NoError;  // skip dispatch call
        GFX_TRY(installShaderState(kernel));
        submitPipelineBarriers();   // transition our resources if needed
        command_list_->Dispatch(num_groups_x, num_groups_y, num_groups_z);
        return kGfxResult_NoError;
    }

    GfxResult encodeDispatchIndirect(GfxBuffer args_buffer)
    {
        if(!buffer_handles_.has_handle(args_buffer.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot dispatch using an invalid arguments buffer object");
        if(args_buffer.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot dispatch using an arguments buffer object with read CPU access");
        if(!kernel_handles_.has_handle(bound_kernel_.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot dispatch when bound kernel object is invalid");
        Kernel &kernel = kernels_[bound_kernel_];
        if(!kernel.isCompute())
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot dispatch using a graphics kernel object");
        if(kernel.root_signature_ == nullptr || kernel.pipeline_state_ == nullptr)
            return kGfxResult_NoError;  // skip dispatch call
        Buffer &gfx_buffer = buffers_[args_buffer];
        SetObjectName(gfx_buffer, args_buffer.name);
        // TODO: might need multiple resource state flags... (gboisse)
        GFX_TRY(installShaderState(kernel));
        if(args_buffer.cpu_access == kGfxCpuAccess_None)
            transitionResource(gfx_buffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        submitPipelineBarriers();   // transition our resources if needed
        command_list_->ExecuteIndirect(dispatch_signature_, 1, gfx_buffer.resource_, gfx_buffer.data_offset_, nullptr, 0);
        return kGfxResult_NoError;
    }

    GfxTimestampQuery createTimestampQuery()
    {
        GfxTimestampQuery timestamp_query = {};
        timestamp_query.handle = timestamp_query_handles_.allocate_handle();
        timestamp_queries_.insert(timestamp_query);
        return timestamp_query;
    }

    GfxResult destroyTimestampQuery(GfxTimestampQuery const &timestamp_query)
    {
        if(!timestamp_query)
            return kGfxResult_NoError;
        if(!timestamp_query_handles_.has_handle(timestamp_query.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid timestamp query object");
        timestamp_queries_.erase(timestamp_query);  // destroy timestamp query
        timestamp_query_handles_.free_handle(timestamp_query.handle);
        return kGfxResult_NoError;
    }

    float getTimestampQueryDuration(GfxTimestampQuery const &timestamp_query)
    {
        if(!timestamp_query_handles_.has_handle(timestamp_query.handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Cannot get the duration of an invalid timestamp query object");
            return 0.0f;
        }
        return timestamp_queries_[timestamp_query].duration_;
    }

    GfxResult encodeBeginTimestampQuery(GfxTimestampQuery const &timestamp_query)
    {
        if(!timestamp_query_handles_.has_handle(timestamp_query.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot begin a timed section using an invalid timestamp query object");
        TimestampQuery &gfx_timestamp_query = timestamp_queries_[timestamp_query];
        if(gfx_timestamp_query.was_begun_)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot begin a timed section on a timestamp query object that was already open");
        GFX_ASSERT(fence_index_ < ARRAYSIZE(timestamp_query_heaps_));   // should never happen
        TimestampQueryHeap &timestamp_query_heap = timestamp_query_heaps_[fence_index_];
        if(timestamp_query_heap.timestamp_queries_.find(timestamp_query.handle) != timestamp_query_heap.timestamp_queries_.end())
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot use a timestamp query object more than once per frame");
        uint32_t timestamp_query_index = (uint32_t)timestamp_query_heap.timestamp_queries_.size();
        uint64_t timestamp_query_heap_size = 2 * (timestamp_query_index + 1) * sizeof(uint64_t);
        if(timestamp_query_heap_size > timestamp_query_heap.query_buffer_.size)
        {
            ID3D12QueryHeap *query_heap = nullptr;
            timestamp_query_heap_size += (timestamp_query_heap_size + 2) >> 1;
            timestamp_query_heap_size = GFX_ALIGN(timestamp_query_heap_size, 65536);
            D3D12_QUERY_HEAP_DESC
            query_heap_desc       = {};
            query_heap_desc.Type  = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
            query_heap_desc.Count = (uint32_t)(timestamp_query_heap_size >> 3);
            device_->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(&query_heap));
            if(query_heap == nullptr)
                return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to create query heap for timestamp query object");
            GfxBuffer query_buffer = createBuffer(timestamp_query_heap_size, nullptr, kGfxCpuAccess_Read);
            if(!query_buffer)
            {
                query_heap->Release();  // release resource
                return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to allocate memory for timestamp query heap buffer");
            }
            query_buffer.setName("gfx_TimestampQueryHeapBuffer");
            collect(timestamp_query_heap.query_heap_);
            destroyBuffer(timestamp_query_heap.query_buffer_);
            timestamp_query_heap.timestamp_queries_.clear();
            timestamp_query_heap.query_buffer_ = query_buffer;
            timestamp_query_heap.query_heap_ = query_heap;
        }
        GFX_ASSERT(timestamp_query_heap.query_heap_ != nullptr);
        timestamp_query_index = (uint32_t)timestamp_query_heap.timestamp_queries_.size();
        command_list_->EndQuery(timestamp_query_heap.query_heap_, D3D12_QUERY_TYPE_TIMESTAMP, 2 * timestamp_query_index + 0);
        timestamp_query_heap.timestamp_queries_[timestamp_query.handle] = std::pair<uint32_t, GfxTimestampQuery>(timestamp_query_index, timestamp_query);
        gfx_timestamp_query.was_begun_ = true;
        return kGfxResult_NoError;
    }

    GfxResult encodeEndTimestampQuery(GfxTimestampQuery const &timestamp_query)
    {
        if(!timestamp_query_handles_.has_handle(timestamp_query.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot end a timed section using an invalid timestamp query object");
        TimestampQuery &gfx_timestamp_query = timestamp_queries_[timestamp_query];
        if(!gfx_timestamp_query.was_begun_)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot end a timed section using a timestamp query object that was already closed");
        GFX_ASSERT(fence_index_ < ARRAYSIZE(timestamp_query_heaps_));   // should never happen
        TimestampQueryHeap &timestamp_query_heap = timestamp_query_heaps_[fence_index_];
        gfx_timestamp_query.was_begun_ = false; // timestamp query is now closed
        std::map<uint64_t, std::pair<uint32_t, GfxTimestampQuery>>::const_iterator const it = timestamp_query_heap.timestamp_queries_.find(timestamp_query.handle);
        GFX_ASSERT(it != timestamp_query_heap.timestamp_queries_.end());
        if(it == timestamp_query_heap.timestamp_queries_.end())
            return kGfxResult_InternalError;    // should never happen
        uint32_t const timestamp_query_index = (*it).second.first;
        command_list_->EndQuery(timestamp_query_heap.query_heap_, D3D12_QUERY_TYPE_TIMESTAMP, 2 * timestamp_query_index + 1);
        return kGfxResult_NoError;
    }

    GfxResult encodeBeginEvent(uint64_t color, char const *format, va_list args)
    {
        char buffer[4096];
        vsnprintf(buffer, sizeof(buffer), format, args);
        PIXBeginEvent(command_list_, color, buffer);
        return kGfxResult_NoError;
    }

    GfxResult encodeEndEvent()
    {
        PIXEndEvent(command_list_);
        return kGfxResult_NoError;
    }

    enum OpType
    {
        kOpType_Min = 0,
        kOpType_Max,
        kOpType_Sum,

        kOpType_Count
    };

    GfxResult encodeScan(OpType op_type, GfxDataType data_type, GfxBuffer const &dst, GfxBuffer const &src, GfxBuffer const *count)
    {
        ScanKernels *scan_kernels = nullptr;
        GFX_ASSERT(op_type < kOpType_Count);    // should never happen
        if(data_type < 0 || data_type >= kGfxDataType_Count)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot scan buffer object of unsupported data type `%u'", data_type);
        if(dst.size != src.size)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot scan if source and destination buffer objects aren't of the same size");
        if((dst.size >> 2) > 0xFFFFFFFFull)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot scan buffer object of more than 4 billion keys");
        if(dst.cpu_access == kGfxCpuAccess_Read || src.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot scan buffer object with read CPU access");
        if(!buffer_handles_.has_handle(dst.handle) || !buffer_handles_.has_handle(src.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot scan when supplied buffer object is invalid");
        if(count != nullptr && !buffer_handles_.has_handle(count->handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot scan when supplied count buffer object is invalid");
        uint32_t const key = (data_type << 3) | (op_type << 1) | (count != nullptr ? 1 : 0);
        std::map<uint32_t, ScanKernels>::iterator const it = scan_kernels_.find(key);
        if(it != scan_kernels_.end())
            scan_kernels = &(*it).second;
        else
        {
            char const *data_type_str = nullptr, *identity_str = nullptr;
            switch(data_type)
            {
            case kGfxDataType_Int:
                data_type_str = "int";
                identity_str = (op_type == kOpType_Min ? "2147483647" : op_type == kOpType_Max ? "-2147483648" : "0");
                break;
            case kGfxDataType_Uint:
                data_type_str = "uint";
                identity_str = (op_type == kOpType_Min ? "4294967295u" : "0");
                break;
            default:
                data_type_str = "float";
                identity_str = (op_type == kOpType_Min ? "3.402823466e+38f" : op_type == kOpType_Max ? "1.175494351e-38f" : "0.0f");
                break;
            }
            std::string scan_program_source;
            scan_program_source +=
                "#define GROUP_SIZE      256\r\n"
                "#define KEYS_PER_THREAD 4\r\n"
                "#define KEYS_PER_GROUP  (GROUP_SIZE * KEYS_PER_THREAD)\r\n"
                "\r\n";
            if(count != nullptr)
                scan_program_source += "RWStructuredBuffer<uint> g_CountBuffer;\r\n";
            else
                scan_program_source += "uint g_Count;\r\n\r\n";
            scan_program_source +=
                "RWStructuredBuffer<";
            scan_program_source += data_type_str;
            scan_program_source += "> g_InputKeys;\r\n";
            scan_program_source +=
                "RWStructuredBuffer<";
            scan_program_source += data_type_str;
            scan_program_source += "> g_OutputKeys;\r\n";
            scan_program_source +=
                "RWStructuredBuffer<";
            scan_program_source += data_type_str;
            scan_program_source += "> g_PartialResults;\r\n";
            if(count != nullptr)
                scan_program_source +=
                    "\r\n"
                    "RWStructuredBuffer<uint4> g_Args1Buffer;\r\n"
                    "RWStructuredBuffer<uint4> g_Args2Buffer;\r\n"
                    "RWStructuredBuffer<uint>  g_Count1Buffer;\r\n"
                    "RWStructuredBuffer<uint>  g_Count2Buffer;\r\n";
            scan_program_source +=
                "\r\n"
                "groupshared ";
            scan_program_source += data_type_str;
            scan_program_source += " lds_keys[GROUP_SIZE];\r\n";
            scan_program_source +=
                "groupshared ";
            scan_program_source += data_type_str;
            scan_program_source += " lds_loads[KEYS_PER_THREAD][GROUP_SIZE];\r\n";
            scan_program_source +=
                "\r\n"
                "uint GetCount()\r\n"
                "{\r\n";
            if(count != nullptr)
                scan_program_source += "    return g_CountBuffer[0];\r\n";
            else
                scan_program_source += "    return g_Count;\r\n";
            scan_program_source +=
                "}\r\n"
                "\r\n";
            scan_program_source += data_type_str;
            scan_program_source += " Op(in ";
            scan_program_source += data_type_str;
            scan_program_source += " lhs, in ";
            scan_program_source += data_type_str;
            scan_program_source += " rhs)\r\n";
            scan_program_source +=
                "{\r\n";
            switch(op_type)
            {
            case kOpType_Min:
                scan_program_source += "    return min(lhs, rhs);\r\n";
                break;
            case kOpType_Max:
                scan_program_source += "    return max(lhs, rhs);\r\n";
                break;
            default:
                scan_program_source += "    return lhs + rhs;\r\n";
                break;
            }
            scan_program_source +=
                "}\r\n"
                "\r\n";
            scan_program_source += data_type_str;
            scan_program_source += " GroupScan(in ";
            scan_program_source += data_type_str;
            scan_program_source += " key, in uint lidx)\r\n";
            scan_program_source +=
                "{\r\n"
                "    lds_keys[lidx] = key;\r\n"
                "    GroupMemoryBarrierWithGroupSync();\r\n"
                "\r\n"
                "    uint stride;\r\n"
                "    for(stride = 1; stride < GROUP_SIZE; stride <<= 1)\r\n"
                "    {\r\n"
                "        if(lidx < GROUP_SIZE / (2 * stride))\r\n"
                "        {\r\n"
                "            lds_keys[2 * (lidx + 1) * stride - 1] = Op(lds_keys[2 * (lidx + 1) * stride - 1],\r\n"
                "                                                       lds_keys[(2 * lidx + 1) * stride - 1]);\r\n"
                "        }\r\n"
                "        GroupMemoryBarrierWithGroupSync();\r\n"
                "    }\r\n"
                "\r\n"
                "    if(lidx == 0)\r\n"
                "        lds_keys[GROUP_SIZE - 1] = ";
            scan_program_source += identity_str;
            scan_program_source += ";\r\n";
            scan_program_source +=
                "    GroupMemoryBarrierWithGroupSync();\r\n"
                "\r\n"
                "    for(stride = (GROUP_SIZE >> 1); stride > 0; stride >>= 1)\r\n"
                "    {\r\n"
                "        if(lidx < GROUP_SIZE / (2 * stride))\r\n"
                "        {\r\n            ";
            scan_program_source += data_type_str;
            scan_program_source += " tmp = lds_keys[(2 * lidx + 1) * stride - 1];\r\n";
            scan_program_source +=
                "            lds_keys[(2 * lidx + 1) * stride - 1] = lds_keys[2 * (lidx + 1) * stride - 1];\r\n"
                "            lds_keys[2 * (lidx + 1) * stride - 1] = Op(lds_keys[2 * (lidx + 1) * stride - 1], tmp);\r\n"
                "        }\r\n"
                "        GroupMemoryBarrierWithGroupSync();\r\n"
                "    }\r\n"
                "\r\n"
                "    return lds_keys[lidx];\r\n"
                "}\r\n"
                "\r\n";
            scan_program_source += data_type_str;
            scan_program_source += " GroupReduce(in ";
            scan_program_source += data_type_str;
            scan_program_source += " key, in uint lidx)\r\n";
            scan_program_source +=
                "{\r\n"
                "    lds_keys[lidx] = key;\r\n"
                "    GroupMemoryBarrierWithGroupSync();\r\n"
                "\r\n"
                "    for(uint stride = (GROUP_SIZE >> 1); stride > 0; stride >>= 1)\r\n"
                "    {\r\n"
                "        if(lidx < stride)\r\n"
                "        {\r\n"
                "            lds_keys[lidx] = Op(lds_keys[lidx], lds_keys[lidx + stride]);\r\n"
                "        }\r\n"
                "        GroupMemoryBarrierWithGroupSync();\r\n"
                "    }\r\n"
                "\r\n"
                "    return lds_keys[0];\r\n"
                "}\r\n"
                "\r\n"
                "[numthreads(GROUP_SIZE, 1, 1)]\r\n"
                "void BlockScan(in uint gidx : SV_DispatchThreadID, in uint lidx : SV_GroupThreadID, in uint bidx : SV_GroupID)\r\n"
                "{\r\n"
                "    uint i, count = GetCount();\r\n"
                "\r\n"
                "    uint range_begin = bidx * GROUP_SIZE * KEYS_PER_THREAD;\r\n"
                "    for(i = 0; i < KEYS_PER_THREAD; ++i)\r\n"
                "    {\r\n"
                "        uint load_index = range_begin + i * GROUP_SIZE + lidx;\r\n"
                "\r\n"
                "        uint col = (i * GROUP_SIZE + lidx) / KEYS_PER_THREAD;\r\n"
                "        uint row = (i * GROUP_SIZE + lidx) % KEYS_PER_THREAD;\r\n"
                "\r\n"
                "        lds_loads[row][col] = (load_index < count ? g_InputKeys[load_index] : ";
            scan_program_source += identity_str;
            scan_program_source += ");\r\n";
            scan_program_source +=
                "    }\r\n"
                "    GroupMemoryBarrierWithGroupSync();\r\n"
                "\r\n    ";
            scan_program_source += data_type_str;
            scan_program_source += " thread_sum = ";
            scan_program_source += identity_str;
            scan_program_source += ";\r\n";
            scan_program_source +=
                "    for(i = 0; i < KEYS_PER_THREAD; ++i)\r\n"
                "    {\r\n        ";
            scan_program_source += data_type_str;
            scan_program_source += " tmp = lds_loads[i][lidx];\r\n";
            scan_program_source +=
                "        lds_loads[i][lidx] = thread_sum;\r\n"
                "        thread_sum = Op(thread_sum, tmp);\r\n"
                "    }\r\n"
                "    thread_sum = GroupScan(thread_sum, lidx);\r\n"
                "\r\n    ";
            scan_program_source += data_type_str;
            scan_program_source += " partial_result = ";
            scan_program_source += identity_str;
            scan_program_source += ";\r\n";
            scan_program_source +=
                "#ifdef PARTIAL_RESULT\r\n"
                "    partial_result = g_PartialResults[bidx];\r\n"
                "#endif\r\n"
                "\r\n"
                "    for(i = 0; i < KEYS_PER_THREAD; ++i)\r\n"
                "    {\r\n"
                "        lds_loads[i][lidx] = Op(lds_loads[i][lidx], thread_sum);\r\n"
                "    }\r\n"
                "    GroupMemoryBarrierWithGroupSync();\r\n"
                "\r\n"
                "    for(i = 0; i < KEYS_PER_THREAD; ++i)\r\n"
                "    {\r\n"
                "        uint store_index = range_begin + i * GROUP_SIZE + lidx;\r\n"
                "\r\n"
                "        uint col = (i * GROUP_SIZE + lidx) / KEYS_PER_THREAD;\r\n"
                "        uint row = (i * GROUP_SIZE + lidx) % KEYS_PER_THREAD;\r\n"
                "\r\n"
                "        if(store_index < count)\r\n"
                "        {\r\n"
                "            g_OutputKeys[store_index] = Op(lds_loads[row][col], partial_result);\r\n"
                "        }\r\n"
                "    }\r\n"
                "}\r\n"
                "\r\n"
                "[numthreads(GROUP_SIZE, 1, 1)]\r\n"
                "void BlockReduce(in uint gidx : SV_DispatchThreadID, in uint lidx : SV_GroupThreadID, in uint bidx : SV_GroupID)\r\n"
                "{\r\n    ";
            scan_program_source += data_type_str;
            scan_program_source += " thread_sum = ";
            scan_program_source += identity_str;
            scan_program_source += ";\r\n";
            scan_program_source +=
                "\r\n"
                "    uint range_begin = bidx * GROUP_SIZE * KEYS_PER_THREAD, count = GetCount();\r\n"
                "    for(uint i = 0; i < KEYS_PER_THREAD; ++i)\r\n"
                "    {\r\n"
                "        uint load_index = range_begin + i * GROUP_SIZE + lidx;\r\n"
                "        thread_sum = Op(load_index < count ? g_InputKeys[load_index] : ";
            scan_program_source += identity_str;
            scan_program_source += ", thread_sum);\r\n";
            scan_program_source +=
                "    }\r\n"
                "    thread_sum = GroupReduce(thread_sum, lidx);\r\n"
                "\r\n"
                "    if(lidx == 0)\r\n"
                "    {\r\n"
                "        g_PartialResults[bidx] = thread_sum;\r\n"
                "    }\r\n"
                "}\r\n";
            if(count != nullptr)
            {
                scan_program_source +=
                    "\r\n"
                    "[numthreads(1, 1, 1)]\r\n"
                    "void GenerateDispatches()\r\n"
                    "{\r\n"
                    "    uint num_keys = g_CountBuffer[0];\r\n"
                    "    uint num_groups_level_1 = (num_keys + KEYS_PER_GROUP - 1) / KEYS_PER_GROUP;\r\n"
                    "    uint num_groups_level_2 = (num_groups_level_1 + KEYS_PER_GROUP - 1) / KEYS_PER_GROUP;\r\n"
                    "    g_Args1Buffer[0] = uint4((num_keys + GROUP_SIZE - 1) / GROUP_SIZE, 1, 1, 0);\r\n"
                    "    g_Args2Buffer[0] = uint4((num_groups_level_1 + GROUP_SIZE - 1) / GROUP_SIZE, 1, 1, 0);\r\n"
                    "    g_Count1Buffer[0] = num_groups_level_1;\r\n"
                    "    g_Count2Buffer[0] = num_groups_level_2;\r\n"
                    "}\r\n";
            }
            scan_kernels = &scan_kernels_[key];
            GfxProgramDesc scan_program_desc = {};
            scan_program_desc.cs = scan_program_source.c_str();
            char const *scan_add_defines[] = { "PARTIAL_RESULT" };
            scan_kernels->scan_program_ = createProgram(scan_program_desc, "gfx_ScanProgram");
            scan_kernels->reduce_kernel_ = createComputeKernel(scan_kernels->scan_program_, "BlockReduce", nullptr, 0);
            scan_kernels->scan_add_kernel_ = createComputeKernel(scan_kernels->scan_program_, "BlockScan", scan_add_defines, ARRAYSIZE(scan_add_defines));
            scan_kernels->scan_kernel_ = createComputeKernel(scan_kernels->scan_program_, "BlockScan", nullptr, 0);
            if(count != nullptr)
                scan_kernels->args_kernel_ = createComputeKernel(scan_kernels->scan_program_, "GenerateDispatches", nullptr, 0);
        }
        GFX_ASSERT(scan_kernels != nullptr);
        uint32_t const group_size = 256;
        uint32_t const keys_per_thread = 4;
        uint32_t const keys_per_group = group_size * keys_per_thread;
        uint32_t const num_keys = (uint32_t)(dst.size >> 2);
        uint32_t const num_groups_level_1 = (num_keys + keys_per_group - 1) / keys_per_group;
        uint32_t const num_groups_level_2 = (num_groups_level_1 + keys_per_group - 1) / keys_per_group;
        if(num_groups_level_2 > keys_per_group)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot scan buffer object of more than 1 billion keys"); // TODO: implement 3-level scan? (gboisse)
        uint64_t scratch_buffer_size = (num_groups_level_1 + num_groups_level_2 + 10) << 2;
        if(texture_upload_buffer_.size < scratch_buffer_size)
        {
            destroyBuffer(texture_upload_buffer_);
            scratch_buffer_size += (scratch_buffer_size + 2) >> 1;
            scratch_buffer_size = GFX_ALIGN(scratch_buffer_size, 65536);
            texture_upload_buffer_ = createBuffer(scratch_buffer_size, nullptr, kGfxCpuAccess_None);
            if(!texture_upload_buffer_)
                return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to allocate scratch memory for scan");
            texture_upload_buffer_.setName("gfx_TextureUploadBuffer");
        }
        GfxBuffer const scan_level_1_buffer = createBufferRange(texture_upload_buffer_, 0, num_groups_level_1 << 2);
        GfxBuffer const scan_level_2_buffer = createBufferRange(texture_upload_buffer_, num_groups_level_1 << 2, num_groups_level_2 << 2);
        GfxBuffer const num_groups_level_1_buffer = createBufferRange(texture_upload_buffer_, (num_groups_level_1 + num_groups_level_2) << 2, 4);
        GfxBuffer const num_groups_level_2_buffer = createBufferRange(texture_upload_buffer_, (num_groups_level_1 + num_groups_level_2 + 1) << 2, 4);
        GfxBuffer const scan_level_1_args_buffer = createBufferRange(texture_upload_buffer_, (num_groups_level_1 + num_groups_level_2 + 2) << 2, 4 << 2);
        GfxBuffer const scan_level_2_args_buffer = createBufferRange(texture_upload_buffer_, (num_groups_level_1 + num_groups_level_2 + 6) << 2, 4 << 2);
        if(!scan_level_1_buffer || !scan_level_2_buffer || !num_groups_level_1_buffer || !num_groups_level_2_buffer || !scan_level_1_args_buffer || !scan_level_2_args_buffer)
        {
            destroyBuffer(scan_level_1_buffer); destroyBuffer(scan_level_2_buffer);
            destroyBuffer(scan_level_1_args_buffer); destroyBuffer(scan_level_2_args_buffer);
            destroyBuffer(num_groups_level_1_buffer); destroyBuffer(num_groups_level_2_buffer);
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to create scratch memory for scan");
        }
        GfxKernel const bound_kernel = bound_kernel_;
        if(count != nullptr)
        {
            setProgramBuffer(scan_kernels->scan_program_, "g_CountBuffer", *count);
            setProgramBuffer(scan_kernels->scan_program_, "g_Args1Buffer", scan_level_1_args_buffer);
            setProgramBuffer(scan_kernels->scan_program_, "g_Args2Buffer", scan_level_2_args_buffer);
            setProgramBuffer(scan_kernels->scan_program_, "g_Count1Buffer", num_groups_level_1_buffer);
            setProgramBuffer(scan_kernels->scan_program_, "g_Count2Buffer", num_groups_level_2_buffer);
            encodeBindKernel(scan_kernels->args_kernel_);
            encodeDispatch(1, 1, 1);
        }
        if(num_keys > keys_per_group)
        {
            if(count != nullptr)
                setProgramBuffer(scan_kernels->scan_program_, "g_CountBuffer", *count);
            else
                setProgramConstants(scan_kernels->scan_program_, "g_Count", &num_keys, sizeof(num_keys));
            setProgramBuffer(scan_kernels->scan_program_, "g_PartialResults", scan_level_1_buffer);
            setProgramBuffer(scan_kernels->scan_program_, "g_InputKeys", src);
            encodeBindKernel(scan_kernels->reduce_kernel_);
            if(count != nullptr)
                encodeDispatchIndirect(scan_level_1_args_buffer);
            else
                encodeDispatch(num_groups_level_1, 1, 1);
            if(num_groups_level_1 > keys_per_group)
            {
                if(count != nullptr)
                    setProgramBuffer(scan_kernels->scan_program_, "g_CountBuffer", num_groups_level_1_buffer);
                else
                    setProgramConstants(scan_kernels->scan_program_, "g_Count", &num_groups_level_1, sizeof(num_groups_level_1));
                setProgramBuffer(scan_kernels->scan_program_, "g_PartialResults", scan_level_2_buffer);
                setProgramBuffer(scan_kernels->scan_program_, "g_InputKeys", scan_level_1_buffer);
                if(count != nullptr)
                    encodeDispatchIndirect(scan_level_2_args_buffer);
                else
                    encodeDispatch(num_groups_level_2, 1, 1);
                {
                    if(count != nullptr)
                        setProgramBuffer(scan_kernels->scan_program_, "g_CountBuffer", num_groups_level_2_buffer);
                    else
                        setProgramConstants(scan_kernels->scan_program_, "g_Count", &num_groups_level_2, sizeof(num_groups_level_2));
                    setProgramBuffer(scan_kernels->scan_program_, "g_InputKeys", scan_level_2_buffer);
                    setProgramBuffer(scan_kernels->scan_program_, "g_OutputKeys", scan_level_2_buffer);
                    encodeBindKernel(scan_kernels->scan_kernel_);
                    encodeDispatch(1, 1, 1);
                }
            }
            if(count != nullptr)
                setProgramBuffer(scan_kernels->scan_program_, "g_CountBuffer", num_groups_level_1_buffer);
            else
                setProgramConstants(scan_kernels->scan_program_, "g_Count", &num_groups_level_1, sizeof(num_groups_level_1));
            setProgramBuffer(scan_kernels->scan_program_, "g_InputKeys", scan_level_1_buffer);
            setProgramBuffer(scan_kernels->scan_program_, "g_OutputKeys", scan_level_1_buffer);
            setProgramBuffer(scan_kernels->scan_program_, "g_PartialResults", scan_level_2_buffer);
            encodeBindKernel(num_groups_level_1 > keys_per_group ? scan_kernels->scan_add_kernel_ : scan_kernels->scan_kernel_);
            if(count != nullptr)
                encodeDispatchIndirect(scan_level_2_args_buffer);
            else
                encodeDispatch(num_groups_level_2, 1, 1);
        }
        if(count != nullptr)
            setProgramBuffer(scan_kernels->scan_program_, "g_CountBuffer", *count);
        else
            setProgramConstants(scan_kernels->scan_program_, "g_Count", &num_keys, sizeof(num_keys));
        setProgramBuffer(scan_kernels->scan_program_, "g_InputKeys", src);
        setProgramBuffer(scan_kernels->scan_program_, "g_OutputKeys", dst);
        setProgramBuffer(scan_kernels->scan_program_, "g_PartialResults", scan_level_1_buffer);
        encodeBindKernel(num_keys > keys_per_group ? scan_kernels->scan_add_kernel_ : scan_kernels->scan_kernel_);
        if(count != nullptr)
            encodeDispatchIndirect(scan_level_1_args_buffer);
        else
            encodeDispatch(num_groups_level_1, 1, 1);
        destroyBuffer(scan_level_1_buffer); destroyBuffer(scan_level_2_buffer);
        destroyBuffer(scan_level_1_args_buffer); destroyBuffer(scan_level_2_args_buffer);
        destroyBuffer(num_groups_level_1_buffer); destroyBuffer(num_groups_level_2_buffer);
        if(kernel_handles_.has_handle(bound_kernel.handle))
            encodeBindKernel(bound_kernel);
        else
            bound_kernel_ = bound_kernel;
        return kGfxResult_NoError;
    }

    GfxResult encodeReduce(OpType op_type, GfxDataType data_type, GfxBuffer const &dst, GfxBuffer const &src, GfxBuffer const *count)
    {
        GFX_ASSERT(op_type < kOpType_Count);    // should never happen
        if(data_type < 0 || data_type >= kGfxDataType_Count)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot reduce buffer object of unsupported data type `%u'", data_type);
        if(dst.size < 4)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot reduce if destination buffer object isn't at least 4 bytes large");
        if((src.size >> 2) > 0xFFFFFFFFull)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot reduce buffer object of more than 4 billion keys");
        if(dst.cpu_access == kGfxCpuAccess_Read || src.cpu_access == kGfxCpuAccess_Read)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot reduce buffer object with read CPU access");
        if(!buffer_handles_.has_handle(dst.handle) || !buffer_handles_.has_handle(src.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot reduce when supplied buffer object is invalid");
        if(count != nullptr && !buffer_handles_.has_handle(count->handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot reduce when supplied count buffer object is invalid");
        // TODO: ... (gboisse)
        (void)op_type;
        return kGfxResult_NoError;
    }

    GfxResult encodeRadixSort(GfxBuffer const &keys_dst, GfxBuffer const &keys_src, GfxBuffer const *values_dst, GfxBuffer const *values_src, GfxBuffer const *count)
    {
        if(keys_dst.size != keys_src.size)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot sort if source and destination buffer objects aren't of the same size");
        if(values_dst != nullptr && keys_dst.size != values_dst->size)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot sort if keys and values buffer objects aren't of the same size");
        if(values_dst != nullptr && values_src != nullptr && values_dst->size != values_src->size)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot sort if source and destination buffer objects aren't of the same size");
        if((keys_dst.size >> 2) > 0xFFFFFFFFull)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot sort buffer object of more than 4 billion keys");
        if(!buffer_handles_.has_handle(keys_dst.handle) || !buffer_handles_.has_handle(keys_src.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot sort keys when supplied buffer object is invalid");
        if((values_dst != nullptr && !buffer_handles_.has_handle(values_dst->handle)) || (values_src != nullptr && !buffer_handles_.has_handle(values_src->handle)))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot sort values when supplied buffer object is invalid");
        if(count != nullptr && !buffer_handles_.has_handle(count->handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot sort when supplied count buffer object is invalid");
        // TODO: ... (gboisse)
        return kGfxResult_NoError;
    }

    GfxResult frame(bool vsync)
    {
        RECT window_rect = {};
        GetClientRect(window_, &window_rect);
        D3D12_RESOURCE_BARRIER resource_barrier = {};
        resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Transition.pResource = back_buffers_[fence_index_];
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        if(!timestamp_query_heaps_[fence_index_].timestamp_queries_.empty())
        {
            for(std::map<uint64_t, std::pair<uint32_t, GfxTimestampQuery>>::const_iterator it = timestamp_query_heaps_[fence_index_].timestamp_queries_.begin(); it != timestamp_query_heaps_[fence_index_].timestamp_queries_.end(); ++it)
            {
                if(!timestamp_query_handles_.has_handle((*it).second.second.handle))
                    continue;   // timestamp query object was destroyed
                TimestampQuery const &timestamp_query = timestamp_queries_[(*it).second.second];
                if(timestamp_query.was_begun_)  // was the query not closed properly?
                    encodeEndTimestampQuery((*it).second.second);
                GFX_ASSERT(!timestamp_query.was_begun_);
            }
            uint32_t const timestamp_query_count = (uint32_t)timestamp_query_heaps_[fence_index_].timestamp_queries_.size();
            GFX_ASSERT(buffer_handles_.has_handle(timestamp_query_heaps_[fence_index_].query_buffer_.handle));
            Buffer const &query_buffer = buffers_[timestamp_query_heaps_[fence_index_].query_buffer_];
            command_list_->ResolveQueryData(timestamp_query_heaps_[fence_index_].query_heap_,
                D3D12_QUERY_TYPE_TIMESTAMP, 0, 2 * timestamp_query_count, query_buffer.resource_, query_buffer.data_offset_);
        }
        command_list_->ResourceBarrier(1, &resource_barrier);
        command_list_->Close(); // close command list for submit
        ID3D12CommandList *const command_lists[] = { command_list_ };
        command_queue_->ExecuteCommandLists(ARRAYSIZE(command_lists), command_lists);
        command_queue_->Signal(fences_[fence_index_], ++fence_values_[fence_index_]);
        swap_chain_->Present(vsync ? 1 : 0, 0); // toggle vsync
        uint32_t const window_width  = GFX_MAX(window_rect.right,  (LONG)8);
        uint32_t const window_height = GFX_MAX(window_rect.bottom, (LONG)8);
        fence_index_ = swap_chain_->GetCurrentBackBufferIndex();
        if(window_width != window_width_ || window_height != window_height_)
            resizeCallback(window_width, window_height);    // reset fence index
        if(fences_[fence_index_]->GetCompletedValue() < fence_values_[fence_index_])
        {
            fences_[fence_index_]->SetEventOnCompletion(fence_values_[fence_index_], fence_event_);
            WaitForSingleObject(fence_event_, INFINITE);    // wait for GPU to complete
        }
        bound_kernel_ = {};
        command_allocators_[fence_index_]->Reset();
        command_list_->Reset(command_allocators_[fence_index_], nullptr);
        constant_buffer_pool_cursors_[fence_index_] = 0;    // reset pool
        resource_barrier.Transition.pResource = back_buffers_[fence_index_];
        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        command_list_->ResourceBarrier(1, &resource_barrier);
        if(!timestamp_query_heaps_[fence_index_].timestamp_queries_.empty())
        {
            double const ticks_per_milliseconds = timestamp_query_ticks_per_second_ / 1000.0;
            for(std::map<uint64_t, std::pair<uint32_t, GfxTimestampQuery>>::const_iterator it = timestamp_query_heaps_[fence_index_].timestamp_queries_.begin(); it != timestamp_query_heaps_[fence_index_].timestamp_queries_.end(); ++it)
            {
                if(!timestamp_query_handles_.has_handle((*it).second.second.handle))
                    continue;   // timestamp query object was destroyed
                uint32_t const timestamp_query_index = (*it).second.first;
                TimestampQuery &timestamp_query = timestamp_queries_[(*it).second.second];
                GFX_ASSERT(buffer_handles_.has_handle(timestamp_query_heaps_[fence_index_].query_buffer_.handle));
                GFX_ASSERT(timestamp_query_index < timestamp_query_heaps_[fence_index_].timestamp_queries_.size());
                Buffer const &query_buffer = buffers_[timestamp_query_heaps_[fence_index_].query_buffer_];
                uint64_t const *timestamp_query_data = (uint64_t const *)((char const *)query_buffer.data_ + query_buffer.data_offset_);
                double const begin = timestamp_query_data[2 * timestamp_query_index + 0] / ticks_per_milliseconds;
                double const end   = timestamp_query_data[2 * timestamp_query_index + 1] / ticks_per_milliseconds;
                float const duration    = (float)(GFX_MAX(begin, end) - begin); // elapsed time in milliseconds
                float const lerp_amount = (fabsf(duration - timestamp_query.duration_) / GFX_MAX(duration, 1e-3f) > 1.0f ? 0.0f : 0.95f);
                timestamp_query.duration_ = duration * (1.0f - lerp_amount) + timestamp_query.duration_ * lerp_amount;
            }
            timestamp_query_heaps_[fence_index_].timestamp_queries_.clear();
        }
        resetState();   // re-install state
        return runGarbageCollection();
    }

    GfxResult finish()
    {
        command_list_->Close(); // close command list for submit
        ID3D12CommandList *const command_lists[] = { command_list_ };
        command_queue_->ExecuteCommandLists(ARRAYSIZE(command_lists), command_lists);
        GFX_TRY(sync());    // make sure GPU has gone through all pending work
        command_allocators_[fence_index_]->Reset();
        command_list_->Reset(command_allocators_[fence_index_], nullptr);
        resetState();   // re-install state
        return kGfxResult_NoError;
    }

    void resetState()
    {
        descriptor_heap_id_ = 0;
        bound_viewport_.invalidate();
        bound_scissor_rect_.invalidate();
        force_install_index_buffer_ = true;
        force_install_vertex_buffer_ = true;
        bindDrawIdBuffer(); // bind drawID buffer
    }

    static inline GfxInternal *GetGfx(GfxContext &context) { return reinterpret_cast<GfxInternal *>(context.handle); }

    static void DispenseDrawState(GfxDrawState &draw_state)
    {
        draw_state.handle = draw_state_handles_.allocate_handle();
        uint32_t const draw_state_index = static_cast<uint32_t>(draw_state.handle & 0xFFFFFFFFull);
        GFX_ASSERT(!draw_states_.has(draw_state_index));    // should never happen
        draw_states_.insert(draw_state_index).reference_count_ = 1;
    }

    static void RetainDrawState(GfxDrawState const &draw_state)
    {
        uint32_t const draw_state_index = static_cast<uint32_t>(draw_state.handle & 0xFFFFFFFFull);
        DrawState *gfx_draw_state = draw_states_.at(draw_state_index);  // look up draw state
        GFX_ASSERT(gfx_draw_state != nullptr); if(!gfx_draw_state) return;
        GFX_ASSERT(gfx_draw_state->reference_count_ > 0);
        ++gfx_draw_state->reference_count_;
    }

    static void ReleaseDrawState(GfxDrawState const &draw_state)
    {
        uint32_t const draw_state_index = static_cast<uint32_t>(draw_state.handle & 0xFFFFFFFFull);
        DrawState *gfx_draw_state = draw_states_.at(draw_state_index);  // look up draw state
        GFX_ASSERT(gfx_draw_state != nullptr); if(!gfx_draw_state) return;
        GFX_ASSERT(gfx_draw_state->reference_count_ > 0);
        if(--gfx_draw_state->reference_count_ == 0)
        {
            draw_states_.erase(draw_state_index);
            draw_state_handles_.free_handle(draw_state.handle);
        }
    }

    static GfxResult SetDrawStateColorTarget(GfxDrawState const &draw_state, uint32_t target_index, GfxTexture const &texture, uint32_t mip_level, uint32_t slice)
    {
        uint32_t const draw_state_index = static_cast<uint32_t>(draw_state.handle & 0xFFFFFFFFull);
        DrawState *gfx_draw_state = draw_states_.at(draw_state_index);
        if(!gfx_draw_state)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set color target on an invalid draw state object");
        if(target_index >= kGfxConstant_MaxRenderTarget)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot have more than %u render targets in draw state object", (uint32_t)kGfxConstant_MaxRenderTarget);
        if(mip_level >= texture.mip_levels)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw to mip level that does not exist in texture object");
        if(slice >= (texture.is3D() ? GFX_MAX(texture.depth >> mip_level, 1u) : texture.depth))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw to slice that does not exist in texture object");
        gfx_draw_state->draw_state_.color_targets_[target_index].texture_ = texture;
        gfx_draw_state->draw_state_.color_targets_[target_index].mip_level = mip_level;
        gfx_draw_state->draw_state_.color_targets_[target_index].slice = slice;
        return kGfxResult_NoError;
    }

    static GfxResult SetDrawStateDepthStencilTarget(GfxDrawState const &draw_state, GfxTexture const &texture, uint32_t mip_level, uint32_t slice)
    {
        uint32_t const draw_state_index = static_cast<uint32_t>(draw_state.handle & 0xFFFFFFFFull);
        DrawState *gfx_draw_state = draw_states_.at(draw_state_index);
        if(!gfx_draw_state)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set depth/stencil target on an invalid draw state object");
        if(mip_level >= texture.mip_levels)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw to mip level that does not exist in texture object");
        if(slice >= (texture.is3D() ? GFX_MAX(texture.depth >> mip_level, 1u) : texture.depth))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw to slice that does not exist in texture object");
        gfx_draw_state->draw_state_.depth_stencil_target_.texture_ = texture;
        gfx_draw_state->draw_state_.depth_stencil_target_.mip_level = mip_level;
        gfx_draw_state->draw_state_.depth_stencil_target_.slice = slice;
        return kGfxResult_NoError;
    }

    static GfxResult SetDrawStateCullMode(GfxDrawState const &draw_state, D3D12_CULL_MODE cull_mode)
    {
        uint32_t const draw_state_index = static_cast<uint32_t>(draw_state.handle & 0xFFFFFFFFull);
        DrawState *gfx_draw_state = draw_states_.at(draw_state_index);
        if(!gfx_draw_state)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set cull mode on an invalid draw state object");
        gfx_draw_state->draw_state_.raster_state_.cull_mode_ = cull_mode;
        return kGfxResult_NoError;
    }

    static GfxResult SetDrawStateFillMode(GfxDrawState const &draw_state, D3D12_FILL_MODE fill_mode)
    {
        uint32_t const draw_state_index = static_cast<uint32_t>(draw_state.handle & 0xFFFFFFFFull);
        DrawState *gfx_draw_state = draw_states_.at(draw_state_index);
        if(!gfx_draw_state)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set fill mode on an invalid draw state object");
        gfx_draw_state->draw_state_.raster_state_.fill_mode_ = fill_mode;
        return kGfxResult_NoError;
    }

    static GfxResult SetDrawStateBlendMode(GfxDrawState const &draw_state, D3D12_BLEND src_blend, D3D12_BLEND dst_blend, D3D12_BLEND_OP blend_op, D3D12_BLEND src_blend_alpha, D3D12_BLEND dst_blend_alpha, D3D12_BLEND_OP blend_op_alpha)
    {
        uint32_t const draw_state_index = static_cast<uint32_t>(draw_state.handle & 0xFFFFFFFFull);
        DrawState *gfx_draw_state = draw_states_.at(draw_state_index);
        if(!gfx_draw_state)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Cannot set blend mode on an invalid draw state object");
        gfx_draw_state->draw_state_.blend_state_.src_blend_ = src_blend;
        gfx_draw_state->draw_state_.blend_state_.dst_blend_ = dst_blend;
        gfx_draw_state->draw_state_.blend_state_.blend_op_ = blend_op;
        gfx_draw_state->draw_state_.blend_state_.src_blend_alpha_ = src_blend_alpha;
        gfx_draw_state->draw_state_.blend_state_.dst_blend_alpha_ = dst_blend_alpha;
        gfx_draw_state->draw_state_.blend_state_.blend_op_alpha_ = blend_op_alpha;
        return kGfxResult_NoError;
    }

private:
    // http://www.cse.yorku.ca/~oz/hash.html
    static uint64_t Hash(char const *value)
    {
        uint64_t hash = 0;
        GFX_ASSERT(value != nullptr);
        if(value == nullptr) return hash;
        while(*value)
        {
            hash = static_cast<uint64_t>(*value) + (hash << 6) + (hash << 16) - hash;
            ++value;
        }
        return hash;
    }

    template<typename TYPE>
    static inline void SetObjectName(TYPE &object, char const *name)
    {
        WCHAR buffer[1024];
        GFX_ASSERT(name != nullptr);
        if(!*name || (object.Object::flags_ & Object::kFlag_Named) != 0)
            return; // no name or already named
        mbstowcs(buffer, name, ARRAYSIZE(buffer));
        object.Object::flags_ |= Object::kFlag_Named;
        GFX_ASSERT(object.resource_ != nullptr);
        object.resource_->SetName(buffer);
    }

    static inline void SetDebugName(ID3D12Object *object, char const *debug_name)
    {
        WCHAR buffer[1024];
        GFX_ASSERT(object != nullptr);
        debug_name = (debug_name ? debug_name : "");
        mbstowcs(buffer, debug_name, ARRAYSIZE(buffer));
        object->SetName(buffer);
    }

    static inline D3D12_SHADER_BYTECODE GetShaderBytecode(IDxcBlob *shader_bytecode)
    {
        D3D12_SHADER_BYTECODE result = {};
        if(shader_bytecode)
        {
            result.pShaderBytecode = shader_bytecode->GetBufferPointer();
            result.BytecodeLength = shader_bytecode->GetBufferSize();
        }
        return result;
    }

    static inline D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDefaultPSODesc()
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
        for(uint32_t i = 0; i < ARRAYSIZE(pso_desc.BlendState.RenderTarget); ++i)
        {
            pso_desc.BlendState.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
            pso_desc.BlendState.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
            pso_desc.BlendState.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
            pso_desc.BlendState.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
            pso_desc.BlendState.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
            pso_desc.BlendState.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            pso_desc.BlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
            pso_desc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }
        pso_desc.SampleMask = UINT_MAX;
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        pso_desc.RasterizerState.FrontCounterClockwise = TRUE;
        pso_desc.RasterizerState.DepthClipEnable = TRUE;
        pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso_desc.SampleDesc.Count = 1;
        return pso_desc;
    }

    static inline uint32_t GetBytesPerPixel(DXGI_FORMAT format)
    {
        switch(format)
        {
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
            return 1;
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
            return 2;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
            return 4;
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return 8;
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return 16;
        default:
            break;  // unsupported texture format
        }
        return 0;
    }

    static inline bool IsDepthStencilFormat(DXGI_FORMAT format)
    {
        switch(format)
        {
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return true;
        default:
            break;
        }
        return false;
    }

    static inline bool HasStencilBits(DXGI_FORMAT format)
    {
        switch(format)
        {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return true;
        default:
            break;
        }
        return false;
    }

    static inline DXGI_FORMAT GetCBVSRVUAVFormat(DXGI_FORMAT format)
    {
        switch(format)
        {
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        default:
            break;
        }
        return format;
    }

    uint64_t getDescriptorHeapId() const
    {
        return (static_cast<uint64_t>(descriptors_.descriptor_heap_ != nullptr ? descriptors_.descriptor_heap_->GetDesc().NumDescriptors : 0) << 32);
    }

    template<typename TYPE>
    void collect(TYPE *resource)
    {
        Garbage garbage;
        if(resource == nullptr) return;
        garbage.garbage_[0] = (uintptr_t)resource;
        garbage.garbage_collector_ = Garbage::ResourceCollector<TYPE>;
        garbage_collection_.push_back(std::move(garbage));
    }

    void collect(Buffer const &buffer)
    {
        if(buffer.reference_count_ != nullptr)
        {
            GFX_ASSERT(*buffer.reference_count_ > 0);
            if(--*buffer.reference_count_ > 0)
                return; // still in use
        }
        collect(buffer.resource_);
        collect(buffer.allocation_);
        free(buffer.resource_state_);
        free(buffer.reference_count_);
    }

    void collect(Texture const &texture)
    {
        collect(texture.resource_);
        collect(texture.allocation_);
        for(uint32_t i = 0; i < ARRAYSIZE(texture.dsv_descriptor_slots_); ++i)
            for(size_t j = 0; j < texture.dsv_descriptor_slots_[i].size(); ++j)
                freeDSVDescriptor(texture.dsv_descriptor_slots_[i][j]);
        for(uint32_t i = 0; i < ARRAYSIZE(texture.rtv_descriptor_slots_); ++i)
            for(size_t j = 0; j < texture.rtv_descriptor_slots_[i].size(); ++j)
                freeRTVDescriptor(texture.rtv_descriptor_slots_[i][j]);
    }

    void collect(SamplerState const &sampler_state)
    {
        freeSamplerDescriptor(sampler_state.descriptor_slot_);
    }

    void collect(AccelerationStructure const &acceleration_structure)
    {
        destroyBuffer(acceleration_structure.bvh_buffer_);
        for(size_t i = 0; i < acceleration_structure.raytracing_primitives_.size(); ++i)
            if(raytracing_primitive_handles_.has_handle(acceleration_structure.raytracing_primitives_[i].handle))
                destroyRaytracingPrimitive(acceleration_structure.raytracing_primitives_[i]);
    }

    void collect(RaytracingPrimitive const &raytracing_primitive)
    {
        destroyBuffer(raytracing_primitive.bvh_buffer_);
    }

    void collect(Program const &program)
    {
        for(Program::Parameters::const_iterator it = program.parameters_.begin(); it != program.parameters_.end(); ++it)
            const_cast<Program::Parameter &>((*it).second).unset(); // release parameter resources
    }

    void collect(Kernel const &kernel)
    {
        free(kernel.num_threads_);
        collect(kernel.cs_bytecode_);
        collect(kernel.vs_bytecode_);
        collect(kernel.gs_bytecode_);
        collect(kernel.ps_bytecode_);
        collect(kernel.cs_reflection_);
        collect(kernel.vs_reflection_);
        collect(kernel.gs_reflection_);
        collect(kernel.ps_reflection_);
        collect(kernel.root_signature_);
        collect(kernel.pipeline_state_);
        for(uint32_t i = 0; i < kernel.parameter_count_; ++i)
        {
            freeDescriptor(kernel.parameters_[i].descriptor_slot_);
            for(uint32_t j = 0; j < kernel.parameters_[i].variable_count_; ++j)
                kernel.parameters_[i].variables_[j].~Variable();
            free(kernel.parameters_[i].variables_);
            kernel.parameters_[i].~Parameter();
        }
        free(kernel.parameters_);
    }

    void collect(DescriptorHeap const &descriptor_heap)
    {
        collect(descriptor_heap.descriptor_heap_);
    }

    void collect(uint32_t descriptor_slot, GfxFreelist &descriptor_freelist)
    {
        Garbage garbage;
        if(descriptor_slot == 0xFFFFFFFFu) return;
        garbage.garbage_[0] = (uintptr_t)descriptor_slot;
        garbage.garbage_[1] = (uintptr_t)&descriptor_freelist;
        GFX_ASSERT(descriptor_slot < descriptor_freelist.size());
        garbage.garbage_collector_ = Garbage::DescriptorCollector;
        garbage_collection_.push_back(std::move(garbage));
    }

    GfxResult runGarbageCollection()
    {
        for(uint32_t i = 0; i < garbage_collection_.size();)
            if(!garbage_collection_[i].deletion_counter_--)
            {
                if(!i)
                    garbage_collection_.pop_front();
                else
                    garbage_collection_[i++].deletion_counter_ = 0;
            }
            else
                ++i;
        return kGfxResult_NoError;
    }

    GfxResult forceGarbageCollection()
    {
        while(!garbage_collection_.empty())
        {
            while(garbage_collection_.front().deletion_counter_--) {}
            garbage_collection_.pop_front();    // release
        }
        return kGfxResult_NoError;
    }

    uint32_t allocateDescriptor(uint32_t descriptor_count = 1)
    {
        uint32_t const descriptor_slot = freelist_descriptors_.allocate_slots(descriptor_count);
        uint32_t const size = (descriptors_.descriptor_heap_ != nullptr ? descriptors_.descriptor_heap_->GetDesc().NumDescriptors : 0);
        if(freelist_descriptors_.size() > size)
        {
            ID3D12DescriptorHeap *descriptor_heap = nullptr;
            D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
            descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            descriptor_heap_desc.NumDescriptors = freelist_descriptors_.size();
            descriptor_heap_desc.NumDescriptors += ((descriptor_heap_desc.NumDescriptors + 2) >> 1);
            descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            device_->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap));
            if(descriptor_heap == nullptr) return 0xFFFFFFFFu;  // allocation failed
            descriptors_.descriptor_handle_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            SetDebugName(descriptor_heap, "gfx_CBVSRVUAVDescriptorHeap");
            collect(descriptors_);  // free previous descriptor heap
            descriptors_.descriptor_heap_ = descriptor_heap;
        }
        return descriptor_slot;
    }

    void freeDescriptor(uint32_t descriptor_slot)
    {
        collect(descriptor_slot, freelist_descriptors_);
    }

    uint32_t allocateDSVDescriptor()
    {
        uint32_t const dsv_slot = freelist_dsv_descriptors_.allocate_slot();
        uint32_t const size = (dsv_descriptors_.descriptor_heap_ != nullptr ? dsv_descriptors_.descriptor_heap_->GetDesc().NumDescriptors : 0);
        if(freelist_dsv_descriptors_.size() > size)
        {
            ID3D12DescriptorHeap *descriptor_heap = nullptr;
            D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
            descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            descriptor_heap_desc.NumDescriptors = freelist_dsv_descriptors_.size();
            descriptor_heap_desc.NumDescriptors += ((descriptor_heap_desc.NumDescriptors + 2) >> 1);
            device_->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap));
            if(descriptor_heap == nullptr) return 0xFFFFFFFFu;  // allocation failed
            if(size > 0) device_->CopyDescriptorsSimple(size, descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
                dsv_descriptors_.descriptor_heap_->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            dsv_descriptors_.descriptor_handle_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            collect(dsv_descriptors_);  // free previous descriptor heap
            SetDebugName(descriptor_heap, "gfx_DSVDescriptorHeap");
            dsv_descriptors_.descriptor_heap_ = descriptor_heap;
        }
        return dsv_slot;
    }

    void freeDSVDescriptor(uint32_t dsv_slot)
    {
        collect(dsv_slot, freelist_dsv_descriptors_);
    }

    uint32_t allocateRTVDescriptor()
    {
        uint32_t const rtv_slot = freelist_rtv_descriptors_.allocate_slot();
        uint32_t const size = (rtv_descriptors_.descriptor_heap_ != nullptr ? rtv_descriptors_.descriptor_heap_->GetDesc().NumDescriptors : 0);
        if(freelist_rtv_descriptors_.size() > size)
        {
            ID3D12DescriptorHeap *descriptor_heap = nullptr;
            D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
            descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            descriptor_heap_desc.NumDescriptors = freelist_rtv_descriptors_.size();
            descriptor_heap_desc.NumDescriptors += ((descriptor_heap_desc.NumDescriptors + 2) >> 1);
            device_->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap));
            if(descriptor_heap == nullptr) return 0xFFFFFFFFu;  // allocation failed
            if(size > 0) device_->CopyDescriptorsSimple(size, descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
                rtv_descriptors_.descriptor_heap_->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            rtv_descriptors_.descriptor_handle_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            collect(rtv_descriptors_);  // free previous descriptor heap
            SetDebugName(descriptor_heap, "gfx_RTVDescriptorHeap");
            rtv_descriptors_.descriptor_heap_ = descriptor_heap;
        }
        return rtv_slot;
    }

    void freeRTVDescriptor(uint32_t rtv_slot)
    {
        collect(rtv_slot, freelist_rtv_descriptors_);
    }

    uint32_t allocateSamplerDescriptor()
    {
        uint32_t const sampler_slot = freelist_sampler_descriptors_.allocate_slot();
        uint32_t const size = (sampler_descriptors_.descriptor_heap_ != nullptr ? sampler_descriptors_.descriptor_heap_->GetDesc().NumDescriptors : 0);
        if(freelist_sampler_descriptors_.size() > size)
        {
            ID3D12DescriptorHeap *descriptor_heap = nullptr;
            D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
            descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            descriptor_heap_desc.NumDescriptors = freelist_sampler_descriptors_.size();
            descriptor_heap_desc.NumDescriptors += ((descriptor_heap_desc.NumDescriptors + 2) >> 1);
            descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            device_->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap));
            if(descriptor_heap == nullptr) return 0xFFFFFFFFu;  // allocation failed
            sampler_descriptors_.descriptor_handle_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            uint32_t const dummy_descriptor = (sampler_descriptors_.descriptor_heap_ == nullptr ? sampler_slot : dummy_descriptors_[Kernel::Parameter::kType_Sampler]);
            collect(sampler_descriptors_);  // free previous descriptor heap
            SetDebugName(descriptor_heap, "gfx_SamplerDescriptorHeap");
            sampler_descriptors_.descriptor_heap_ = descriptor_heap;
            GFX_ASSERT(dummy_descriptor != 0xFFFFFFFFu);
            if(dummy_descriptor != 0xFFFFFFFFu)
            {
                D3D12_SAMPLER_DESC sampler_desc = {};
                sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                sampler_desc.MaxLOD = static_cast<float>(D3D12_REQ_MIP_LEVELS);
                device_->CreateSampler(&sampler_desc, sampler_descriptors_.getCPUHandle(dummy_descriptor));
            }
            for(uint32_t i = 0; i < sampler_states_.size(); ++i)
            {
                SamplerState const &sampler_state = sampler_states_.data()[i];
                if(sampler_state.descriptor_slot_ == 0xFFFFFFFFu) continue; // invalid descriptor slot
                device_->CreateSampler(&sampler_state.sampler_desc_, sampler_descriptors_.getCPUHandle(sampler_state.descriptor_slot_));
            }
        }
        return sampler_slot;
    }

    void freeSamplerDescriptor(uint32_t sampler_slot)
    {
        collect(sampler_slot, freelist_sampler_descriptors_);
    }

    GfxResult createRootSignature(Kernel &kernel)
    {
        ID3DBlob *result = nullptr, *error = nullptr;
        std::vector<Kernel::Parameter> kernel_parameters;
        std::vector<D3D12_ROOT_PARAMETER> root_parameters;
        std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_ranges;
        GFX_ASSERT(kernel.root_signature_ == nullptr && kernel.parameters_ == nullptr);

        std::vector<void *> allocated_memory;
        struct MemoryReleaser
        {
            GFX_NON_COPYABLE(MemoryReleaser);
            inline MemoryReleaser(std::vector<void *> &allocated_memory) : allocated_memory_(allocated_memory) {}
            inline ~MemoryReleaser() { for(size_t i = 0; i < allocated_memory_.size(); ++i) free(allocated_memory_[i]); }
            std::vector<void *> allocated_memory_;
        };
        MemoryReleaser const memory_releaser(allocated_memory);

        for(uint32_t i = 0; i < kShaderType_Count; ++i)
        {
            D3D12_ROOT_PARAMETER root_parameter = {};
            ID3D12ShaderReflection *shader = nullptr;
            switch(i)
            {
            case kShaderType_CS:
                shader = kernel.cs_reflection_;
                break;
            case kShaderType_VS:
                shader = kernel.vs_reflection_;
                root_parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
                break;
            case kShaderType_GS:
                shader = kernel.gs_reflection_;
                root_parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
                break;
            case kShaderType_PS:
                shader = kernel.ps_reflection_;
                root_parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                break;
            default:
                GFX_ASSERTMSG(0, "Unsupported shader type `%u' was encountered", i);
                break;
            }

            if(!shader) continue;
            D3D12_SHADER_DESC shader_desc = {};
            shader->GetDesc(&shader_desc);

            for(uint32_t j = 0; j < shader_desc.BoundResources; ++j)
            {
                Kernel::Parameter kernel_parameter = {};
                D3D12_DESCRIPTOR_RANGE descriptor_range = {};
                D3D12_SHADER_INPUT_BIND_DESC resource_desc = {};
                shader->GetResourceBindingDesc(j, &resource_desc);
                kernel_parameter.parameter_id_ = Hash(resource_desc.Name);

                switch(resource_desc.Type)
                {
                case D3D_SIT_CBUFFER:
                    {
                        descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                        bool is_root_constant = (strcmp(resource_desc.Name, "$Globals") == 0);
                        if(is_root_constant)
                        {
                            ID3D12ShaderReflectionConstantBuffer *constant_buffer = shader->GetConstantBufferByName(resource_desc.Name);
                            GFX_ASSERT(constant_buffer != nullptr);
                            D3D12_SHADER_BUFFER_DESC buffer_desc = {};
                            constant_buffer->GetDesc(&buffer_desc);
                            if(buffer_desc.Size > 256)  // https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#root-argument-limits
                                is_root_constant = false;   // if exceeding the root parameters size limit, go through the constant buffer pool
                            kernel_parameter.variables_ = (Kernel::Parameter::Variable *)malloc(buffer_desc.Variables * sizeof(Kernel::Parameter::Variable));
                            allocated_memory.push_back(kernel_parameter.variables_);
                            GFX_ASSERT(kernel_parameter.variables_ != nullptr);
                            kernel_parameter.variable_count_ = buffer_desc.Variables;
                            kernel_parameter.variable_size_ = buffer_desc.Size;
                            for(uint32_t k = 0; k < buffer_desc.Variables; ++k)
                            {
                                ID3D12ShaderReflectionVariable *root_variable = constant_buffer->GetVariableByIndex(k);
                                GFX_ASSERT(root_variable != nullptr);
                                D3D12_SHADER_VARIABLE_DESC variable_desc = {};
                                root_variable->GetDesc(&variable_desc);
                                new(&kernel_parameter.variables_[k]) Kernel::Parameter::Variable();
                                kernel_parameter.variables_[k].data_start_ = variable_desc.StartOffset;
                                kernel_parameter.variables_[k].parameter_id_ = Hash(variable_desc.Name);
                                kernel_parameter.variables_[k].data_size_ = variable_desc.Size;
                            }
                        }
                        kernel_parameter.type_ = (is_root_constant ? Kernel::Parameter::kType_Constants : Kernel::Parameter::kType_ConstantBuffer);
                    }
                    break;
                case D3D_SIT_TBUFFER:
                case D3D_SIT_TEXTURE:
                case D3D_SIT_STRUCTURED:
                case D3D_SIT_BYTEADDRESS:
                    descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    switch(resource_desc.Dimension)
                    {
                    case D3D_SRV_DIMENSION_BUFFER:
                        kernel_parameter.type_ = Kernel::Parameter::kType_Buffer;
                        break;
                    case D3D_SRV_DIMENSION_TEXTURE2D:
                        kernel_parameter.type_ = Kernel::Parameter::kType_Texture2D;
                        break;
                    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                        kernel_parameter.type_ = Kernel::Parameter::kType_Texture2DArray;
                        break;
                    case D3D_SRV_DIMENSION_TEXTURE3D:
                        kernel_parameter.type_ = Kernel::Parameter::kType_Texture3D;
                        break;
                    case D3D_SRV_DIMENSION_TEXTURECUBE:
                        kernel_parameter.type_ = Kernel::Parameter::kType_TextureCube;
                        break;
                    default:
                        GFX_ASSERT(0);  // unimplemented
                        break;
                    }
                    break;
                case D3D_SIT_SAMPLER:
                    descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                    kernel_parameter.type_ = Kernel::Parameter::kType_Sampler;
                    break;
                case D3D_SIT_UAV_FEEDBACKTEXTURE:
                    descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    GFX_ASSERT(0);  // unimplemented
                    break;
                case D3D_SIT_UAV_RWTYPED:
                case D3D_SIT_UAV_RWSTRUCTURED:
                case D3D_SIT_UAV_RWBYTEADDRESS:
                case D3D_SIT_UAV_APPEND_STRUCTURED:
                case D3D_SIT_UAV_CONSUME_STRUCTURED:
                case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                    descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    switch(resource_desc.Dimension)
                    {
                    case D3D_SRV_DIMENSION_BUFFER:
                        kernel_parameter.type_ = Kernel::Parameter::kType_RWBuffer;
                        break;
                    case D3D_SRV_DIMENSION_TEXTURE2D:
                        kernel_parameter.type_ = Kernel::Parameter::kType_RWTexture2D;
                        break;
                    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                    case D3D_SRV_DIMENSION_TEXTURECUBE: // note: there is no RWTextureCube<> type, RWTexture2DArray<> must be used instead...
                        kernel_parameter.type_ = Kernel::Parameter::kType_RWTexture2DArray;
                        break;
                    case D3D_SRV_DIMENSION_TEXTURE3D:
                        kernel_parameter.type_ = Kernel::Parameter::kType_RWTexture3D;
                        break;
                    default:
                        GFX_ASSERT(0);  // unimplemented
                        break;
                    }
                    break;
                case D3D_SIT_RTACCELERATIONSTRUCTURE:
                    descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    kernel_parameter.type_ = Kernel::Parameter::kType_AccelerationStructure;
                    break;
                default:
                    GFX_ASSERTMSG(0, "Encountered unsupported shader resource type `%u'", (uint32_t)resource_desc.Type);
                    continue;
                }

                descriptor_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                descriptor_range.BaseShaderRegister = resource_desc.BindPoint;
                descriptor_range.NumDescriptors = resource_desc.BindCount;
                descriptor_range.RegisterSpace = resource_desc.Space;

                if(resource_desc.BindCount == 0)
                    switch(descriptor_range.RangeType)
                    {
                    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                        descriptor_range.NumDescriptors = kGfxConstant_NumBindlessSlots;
                        break;
                    default:
                        break;
                    }
                kernel_parameter.descriptor_count_ = descriptor_range.NumDescriptors;

                root_parameters.push_back(root_parameter);
                descriptor_ranges.push_back(descriptor_range);
                kernel_parameters.push_back(kernel_parameter);
            }
            GFX_ASSERT(root_parameters.size() == kernel_parameters.size());
            GFX_ASSERT(kernel_parameters.size() == descriptor_ranges.size());
        }

        for(size_t i = 0; i < root_parameters.size(); ++i)
        {
            Kernel::Parameter const &kernel_parameter = kernel_parameters[i];
            switch(kernel_parameter.type_)
            {
            case Kernel::Parameter::kType_Constants:
                root_parameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                root_parameters[i].Constants.Num32BitValues = kernel_parameter.variable_size_ / sizeof(uint32_t);
                root_parameters[i].Constants.ShaderRegister = descriptor_ranges[i].BaseShaderRegister;
                root_parameters[i].Constants.RegisterSpace = descriptor_ranges[i].RegisterSpace;
                break;
            default:
                root_parameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                root_parameters[i].DescriptorTable.pDescriptorRanges = &descriptor_ranges[i];
                root_parameters[i].DescriptorTable.NumDescriptorRanges = 1;
                break;
            }
        }

        D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
        root_signature_desc.pParameters = root_parameters.data();
        root_signature_desc.NumParameters = (uint32_t) root_parameters.size();
        root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS   |
                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS   |
                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                    D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
        root_signature_desc.Flags |= (kernel.vs_bytecode_ != nullptr ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                                                                     : D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS);
        root_signature_desc.Flags |= (kernel.gs_bytecode_ != nullptr ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT
                                                                     : D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);
        if(kernel.ps_bytecode_ == nullptr) root_signature_desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &result, &error);
        if(!result)
        {
            GFX_PRINTLN("Error: Failed to serialize root signature%s%s", error ? ":\r\n" : "", error ? (char const *)error->GetBufferPointer() : "");
            if(error) error->Release();
            return GFX_SET_ERROR(kGfxResult_InternalError, "Failed to serizalize root signature");
        }

        device_->CreateRootSignature(0, result->GetBufferPointer(), result->GetBufferSize(), IID_PPV_ARGS(&kernel.root_signature_));
        if(kernel.root_signature_)
        {
            allocated_memory.clear();
            kernel.parameter_count_ = (uint32_t)root_parameters.size();
            kernel.parameters_ = (!root_parameters.empty() ? (Kernel::Parameter *)malloc(kernel.parameter_count_ * sizeof(Kernel::Parameter)) : nullptr);
            GFX_ASSERT(kernel.parameters_ != nullptr || kernel.parameter_count_ == 0);
            GFX_ASSERT(root_parameters.size() == kernel_parameters.size());
            for(uint32_t i = 0; i < kernel.parameter_count_; ++i)
                new(&kernel.parameters_[i]) Kernel::Parameter(kernel_parameters[i]);
        }
        if(error) error->Release();
        result->Release();

        return kGfxResult_NoError;
    }

    GfxResult createComputePipelineState(Kernel &kernel)
    {
        GFX_ASSERT(kernel.pipeline_state_ == nullptr);
        if(kernel.root_signature_ == nullptr) return kGfxResult_NoError;
        D3D12_COMPUTE_PIPELINE_STATE_DESC
        pso_desc                = {};
        pso_desc.pRootSignature = kernel.root_signature_;
        pso_desc.CS             = GetShaderBytecode(kernel.cs_bytecode_);
        device_->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&kernel.pipeline_state_));
        return kGfxResult_NoError;
    }

    GfxResult createGraphicsPipelineState(Kernel &kernel, DrawState::Data const &draw_state)
    {
        GFX_ASSERT(kernel.pipeline_state_ == nullptr);
        std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout;
        if(kernel.root_signature_ == nullptr) return kGfxResult_NoError;
        if(kernel.vs_reflection_)
        {
            D3D12_SHADER_DESC shader_desc = {};
            GFX_ASSERT(kernel.vertex_stride_ == 0);
            kernel.vs_reflection_->GetDesc(&shader_desc);
            for(uint32_t i = 0; i < shader_desc.InputParameters; ++i)
            {
                uint32_t component_count = 0;
                DXGI_FORMAT component_format = DXGI_FORMAT_UNKNOWN;
                D3D12_SIGNATURE_PARAMETER_DESC parameter_desc = {};
                kernel.vs_reflection_->GetInputParameterDesc(i, &parameter_desc);
                if(parameter_desc.SystemValueType != D3D_NAME_UNDEFINED)
                    continue;   // ignore system value types
                while(parameter_desc.Mask)
                {
                    ++component_count;
                    parameter_desc.Mask >>= 1;
                }
                switch(parameter_desc.ComponentType)
                {
                case D3D_REGISTER_COMPONENT_UINT32:
                    component_format = (component_count == 1 ? DXGI_FORMAT_R32_UINT       :
                                        component_count == 2 ? DXGI_FORMAT_R32G32_UINT    :
                                        component_count == 3 ? DXGI_FORMAT_R32G32B32_UINT :
                                                               DXGI_FORMAT_R32G32B32A32_UINT);
                    break;
                case D3D_REGISTER_COMPONENT_SINT32:
                    component_format = (component_count == 1 ? DXGI_FORMAT_R32_SINT       :
                                        component_count == 2 ? DXGI_FORMAT_R32G32_SINT    :
                                        component_count == 3 ? DXGI_FORMAT_R32G32B32_SINT :
                                                               DXGI_FORMAT_R32G32B32A32_SINT);
                    break;
                case D3D_REGISTER_COMPONENT_FLOAT32:
                    component_format = (component_count == 1 ? DXGI_FORMAT_R32_FLOAT       :
                                        component_count == 2 ? DXGI_FORMAT_R32G32_FLOAT    :
                                        component_count == 3 ? DXGI_FORMAT_R32G32B32_FLOAT :
                                                               DXGI_FORMAT_R32G32B32A32_FLOAT);
                    break;
                default:
                    break;  // D3D_REGISTER_COMPONENT_UNKNOWN
                }
                D3D12_INPUT_ELEMENT_DESC
                input_desc                   = {};
                input_desc.SemanticName      = parameter_desc.SemanticName;
                input_desc.SemanticIndex     = parameter_desc.SemanticIndex;
                input_desc.Format            = component_format;
                input_desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
                if(!!strcmp(parameter_desc.SemanticName, "gfx_DrawID"))
                    kernel.vertex_stride_ += component_count * sizeof(uint32_t);
                else
                {
                    input_desc.InputSlot            = 1;
                    input_desc.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                    input_desc.InstanceDataStepRate = 1;
                }
                input_layout.push_back(input_desc);
            }
        }
        D3D12_GRAPHICS_PIPELINE_STATE_DESC
        pso_desc                                = GetDefaultPSODesc();
        pso_desc.pRootSignature                 = kernel.root_signature_;
        pso_desc.VS                             = GetShaderBytecode(kernel.vs_bytecode_);
        pso_desc.PS                             = GetShaderBytecode(kernel.ps_bytecode_);
        pso_desc.GS                             = GetShaderBytecode(kernel.gs_bytecode_);
        pso_desc.RasterizerState.CullMode       = draw_state.raster_state_.cull_mode_;
        pso_desc.RasterizerState.FillMode       = draw_state.raster_state_.fill_mode_;
        pso_desc.InputLayout.pInputElementDescs = input_layout.data();
        pso_desc.InputLayout.NumElements        = (uint32_t)input_layout.size();
        {
            for(uint32_t i = 0; i < ARRAYSIZE(draw_state.color_targets_); ++i)
                if(!draw_state.color_targets_[i].texture_)
                    continue;   // no valid color target at index
                else
                {
                    GfxTexture const &texture = draw_state.color_targets_[i].texture_;
                    pso_desc.RTVFormats[i] = texture.format;
                    pso_desc.NumRenderTargets = i + 1;
                }
            if(draw_state.depth_stencil_target_.texture_)
            {
                pso_desc.DepthStencilState.DepthEnable = TRUE;
                pso_desc.DSVFormat = draw_state.depth_stencil_target_.texture_.format;
            }
            else if(pso_desc.NumRenderTargets == 0)  // special case - if no color target is supplied, draw to back buffer
            {
                pso_desc.NumRenderTargets = 1;
                pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            }
        }
        if(draw_state.blend_state_)
        {
            pso_desc.BlendState.RenderTarget->BlendEnable    = TRUE;
            pso_desc.BlendState.RenderTarget->SrcBlend       = draw_state.blend_state_.src_blend_;
            pso_desc.BlendState.RenderTarget->DestBlend      = draw_state.blend_state_.dst_blend_;
            pso_desc.BlendState.RenderTarget->BlendOp        = draw_state.blend_state_.blend_op_;
            pso_desc.BlendState.RenderTarget->SrcBlendAlpha  = draw_state.blend_state_.src_blend_alpha_;
            pso_desc.BlendState.RenderTarget->DestBlendAlpha = draw_state.blend_state_.dst_blend_alpha_;
            pso_desc.BlendState.RenderTarget->BlendOpAlpha   = draw_state.blend_state_.blend_op_alpha_;
        }
        device_->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&kernel.pipeline_state_));
        return kGfxResult_NoError;
    }

    GfxResult installShaderState(Kernel &kernel, bool indexed = false)
    {
        uint32_t root_constants[64];
        bool const is_compute = kernel.isCompute();
        if(!program_handles_.has_handle(kernel.program_.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot %s with a %s kernel pointing to an invalid program object",
                is_compute ? "dispatch" : "draw", is_compute ? "compute" : "graphics");
        if(!is_compute)
        {
            uint32_t color_target_count = 0;
            D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_target = {};
            uint32_t render_width = 0xFFFFFFFFu, render_height = 0xFFFFFFFFu;
            D3D12_CPU_DESCRIPTOR_HANDLE color_targets[kGfxConstant_MaxRenderTarget] = {};
            for(uint32_t i = 0; i < ARRAYSIZE(kernel.draw_state_.color_targets_); ++i)
                if(!kernel.draw_state_.color_targets_[i].texture_)
                    continue;   // no valid color target at index
                else
                {
                    GfxTexture const &texture = kernel.draw_state_.color_targets_[i].texture_;
                    if(!texture_handles_.has_handle(texture.handle))
                        return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw to an invalid texture object; found at color target %u", i);
                    Texture &gfx_texture = textures_[texture]; SetObjectName(gfx_texture, texture.name);
                    GFX_TRY(ensureTextureHasRenderTargetView(texture, gfx_texture, kernel.draw_state_.color_targets_[i].mip_level, kernel.draw_state_.color_targets_[i].slice));
                    GFX_ASSERT(gfx_texture.rtv_descriptor_slots_[kernel.draw_state_.color_targets_[i].mip_level][kernel.draw_state_.color_targets_[i].slice] != 0xFFFFFFFFu);
                    color_targets[i] = rtv_descriptors_.getCPUHandle(gfx_texture.rtv_descriptor_slots_[kernel.draw_state_.color_targets_[i].mip_level][kernel.draw_state_.color_targets_[i].slice]);
                    for(uint32_t j = color_target_count; j < i; ++j) color_targets[j] = rtv_descriptors_.getCPUHandle(dummy_rtv_descriptor_);
                    uint32_t const texture_width  = ((gfx_texture.flags_ & Texture::kFlag_AutoResize) != 0 ? window_width_  : texture.width);
                    uint32_t const texture_height = ((gfx_texture.flags_ & Texture::kFlag_AutoResize) != 0 ? window_height_ : texture.height);
                    render_width = GFX_MIN(render_width, texture_width); render_height = GFX_MIN(render_height, texture_height);
                    transitionResource(gfx_texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
                    color_target_count = i + 1;
                }
            if(kernel.draw_state_.depth_stencil_target_.texture_)
            {
                GfxTexture const &texture = kernel.draw_state_.depth_stencil_target_.texture_;
                if(!texture_handles_.has_handle(texture.handle))
                    return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot draw to an invalid texture object; found at depth/stencil target");
                Texture &gfx_texture = textures_[texture]; SetObjectName(gfx_texture, texture.name);
                GFX_TRY(ensureTextureHasDepthStencilView(texture, gfx_texture, kernel.draw_state_.depth_stencil_target_.mip_level, kernel.draw_state_.depth_stencil_target_.slice));
                GFX_ASSERT(gfx_texture.dsv_descriptor_slots_[kernel.draw_state_.depth_stencil_target_.mip_level][kernel.draw_state_.depth_stencil_target_.slice] != 0xFFFFFFFFu);
                depth_stencil_target = dsv_descriptors_.getCPUHandle(gfx_texture.dsv_descriptor_slots_[kernel.draw_state_.depth_stencil_target_.mip_level][kernel.draw_state_.depth_stencil_target_.slice]);
                uint32_t const texture_width  = ((gfx_texture.flags_ & Texture::kFlag_AutoResize) != 0 ? window_width_  : texture.width);
                uint32_t const texture_height = ((gfx_texture.flags_ & Texture::kFlag_AutoResize) != 0 ? window_height_ : texture.height);
                render_width = GFX_MIN(render_width, texture_width); render_height = GFX_MIN(render_height, texture_height);
                transitionResource(gfx_texture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            }
            if(color_target_count == 0 && depth_stencil_target.ptr == 0)    // special case - if no color target is supplied, draw to back buffer
            {
                render_width = window_width_; render_height = window_height_;
                color_targets[color_target_count++] = rtv_descriptors_.getCPUHandle(back_buffer_rtvs_[fence_index_]);
            }
            D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)render_width, (float)render_height, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
            if(viewport_.x_ != 0.0f || viewport_.y_ != 0.0f || viewport_.width_ != 0.0f || viewport_.height_ != 0.0f)
            {
                viewport.TopLeftX = (float)viewport_.x_;
                viewport.TopLeftY = (float)viewport_.y_;
                viewport.Width    = (float)viewport_.width_;
                viewport.Height   = (float)viewport_.height_;
            }
            if(bound_viewport_ != viewport)
            {
                bound_viewport_ = viewport;
                command_list_->RSSetViewports(1, &viewport);
            }
            D3D12_RECT scissor_rect = { 0, 0, (LONG)render_width, (LONG)render_height };
            if(scissor_rect_.x_ != 0 || scissor_rect_.y_ != 0 || scissor_rect_.width_ != 0 || scissor_rect_.height_ != 0)
            {
                scissor_rect.left   = (LONG)scissor_rect_.x_;
                scissor_rect.top    = (LONG)scissor_rect_.y_;
                scissor_rect.right  = scissor_rect.left + (LONG)scissor_rect_.width_;
                scissor_rect.bottom = scissor_rect.top  + (LONG)scissor_rect_.height_;
            }
            if(bound_scissor_rect_ != scissor_rect)
            {
                bound_scissor_rect_ = scissor_rect;
                command_list_->RSSetScissorRects(1, &scissor_rect);
            }
            command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            command_list_->OMSetRenderTargets(color_target_count, color_targets, false, depth_stencil_target.ptr != 0 ? &depth_stencil_target : nullptr);
        }
        uint64_t const previous_descriptor_heap_id = getDescriptorHeapId();
        Program const &program = programs_[kernel.program_];
        for(uint32_t i = 0; i < kernel.parameter_count_; ++i)
        {
            Kernel::Parameter &parameter = kernel.parameters_[i];
            if(parameter.type_ >= Kernel::Parameter::kType_Count) continue;
            switch(parameter.type_)
            {
            case Kernel::Parameter::kType_Constants:
                populateRootConstants(program, parameter, root_constants);
                if(is_compute)
                    command_list_->SetComputeRoot32BitConstants(i, parameter.variable_size_ / sizeof(uint32_t), root_constants, 0);
                else
                    command_list_->SetGraphicsRoot32BitConstants(i, parameter.variable_size_ / sizeof(uint32_t), root_constants, 0);
                break;
            case Kernel::Parameter::kType_ConstantBuffer:
                if(parameter.parameter_ == nullptr && parameter.variable_size_ == 0)
                {
                    Program::Parameters::const_iterator const it = program.parameters_.find(parameter.parameter_id_);
                    if(it != program.parameters_.end()) parameter.parameter_ = &(*it).second;
                }
                if(parameter.parameter_ != nullptr || parameter.variable_size_ > 0)
                {
                    freeDescriptor(parameter.descriptor_slot_);
                    parameter.descriptor_slot_ = allocateDescriptor();
                }
                break;
            case Kernel::Parameter::kType_Sampler:
                if(parameter.parameter_ == nullptr)
                {
                    Program::Parameters::const_iterator const it = program.parameters_.find(parameter.parameter_id_);
                    if(it != program.parameters_.end()) parameter.parameter_ = &(*it).second;
                }
                break;
            default:
                if(parameter.parameter_ == nullptr)
                {
                    Program::Parameters::const_iterator const it = program.parameters_.find(parameter.parameter_id_);
                    if(it != program.parameters_.end()) parameter.parameter_ = &(*it).second;
                }
                switch(parameter.type_)
                {
                case Kernel::Parameter::kType_RWTexture2D:
                case Kernel::Parameter::kType_RWTexture3D:
                case Kernel::Parameter::kType_RWTexture2DArray:
                    if(parameter.parameter_ != nullptr && parameter.id_ != parameter.parameter_->id_ &&
                       parameter.parameter_->type_ == Program::Parameter::kType_Image &&
                       texture_handles_.has_handle(parameter.parameter_->data_.image_.textures_->handle))
                        ensureTextureHasUsageFlag(textures_[*parameter.parameter_->data_.image_.textures_], D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
                    break;
                case Kernel::Parameter::kType_AccelerationStructure:
                    if(parameter.parameter_ != nullptr && parameter.id_ == parameter.parameter_->id_)
                    {
                        if(acceleration_structure_handles_.has_handle(parameter.parameter_->data_.acceleration_structure_.bvh_.handle))
                        {
                            AccelerationStructure const &acceleration_structure = acceleration_structures_[parameter.parameter_->data_.acceleration_structure_.bvh_];
                            if(acceleration_structure.bvh_buffer_.handle != parameter.parameter_->data_.acceleration_structure_.bvh_buffer_.handle)
                                ++const_cast<Program::Parameter *>(parameter.parameter_)->id_;
                        }
                    }
                    break;
                default:
                    break;
                }
                if(parameter.parameter_ != nullptr && parameter.id_ != parameter.parameter_->id_)
                {
                    freeDescriptor(parameter.descriptor_slot_);
                    GFX_ASSERT(parameter.descriptor_count_ > 0);
                    parameter.descriptor_slot_ = allocateDescriptor(parameter.descriptor_count_);
                }
                break;
            }
        }
        uint64_t const descriptor_heap_id = getDescriptorHeapId();
        if(descriptor_heap_id_ != descriptor_heap_id)
        {
            uint32_t descriptor_heap_count = 0;
            ID3D12DescriptorHeap *const descriptor_heaps[] =
            {
                descriptors_.descriptor_heap_,
                sampler_descriptors_.descriptor_heap_
            };
            for(uint32_t i = 0; i < ARRAYSIZE(descriptor_heaps); ++i)
                descriptor_heap_count += (descriptor_heaps[descriptor_heap_count] != nullptr ? 1 : 0);
            command_list_->SetDescriptorHeaps(descriptor_heap_count, descriptor_heaps);
            descriptor_heap_id_ = descriptor_heap_id;
        }
        if(descriptor_heap_id != previous_descriptor_heap_id) populateDummyDescriptors();
        bool const invalidate_descriptors = (kernel.descriptor_heap_id_ != descriptor_heap_id);
        kernel.descriptor_heap_id_ = descriptor_heap_id;    // update descriptor heap id
        for(uint32_t i = 0; i < kernel.parameter_count_; ++i)
        {
            Kernel::Parameter &parameter = kernel.parameters_[i];
            if(parameter.type_ >= Kernel::Parameter::kType_Count) continue;
            if(parameter.type_ == Kernel::Parameter::kType_Constants) continue;
            if(parameter.type_ == Kernel::Parameter::kType_Sampler)
            {
                uint32_t descriptor_slot = dummy_descriptors_[parameter.type_];
                if(parameter.parameter_ != nullptr)
                {
                    if(parameter.parameter_->type_ != Program::Parameter::kType_SamplerState)
                    {
                        if(parameter.id_ != parameter.parameter_->id_)
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a sampler state object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                    }
                    else if(!sampler_state_handles_.has_handle(parameter.parameter_->data_.sampler_state_.handle))
                        GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid sampler state object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                    else
                    {
                        SamplerState const &sampler_state = sampler_states_[parameter.parameter_->data_.sampler_state_];
                        GFX_ASSERT(sampler_state.descriptor_slot_ != 0xFFFFFFFFu);
                        if(sampler_state.descriptor_slot_ != 0xFFFFFFFFu)
                            descriptor_slot = sampler_state.descriptor_slot_;
                    }
                    parameter.id_ = parameter.parameter_->id_;
                }
                GFX_ASSERT(descriptor_slot < (sampler_descriptors_.descriptor_heap_ != nullptr ? sampler_descriptors_.descriptor_heap_->GetDesc().NumDescriptors : 0));
                if(is_compute)
                    command_list_->SetComputeRootDescriptorTable(i, sampler_descriptors_.getGPUHandle(descriptor_slot));
                else
                    command_list_->SetGraphicsRootDescriptorTable(i, sampler_descriptors_.getGPUHandle(descriptor_slot));
                continue;   // done
            }
            uint32_t descriptor_slot = (parameter.descriptor_slot_ != 0xFFFFFFFFu ? parameter.descriptor_slot_ :
                                                                                    dummy_descriptors_[parameter.type_]);
            if(parameter.descriptor_slot_ != 0xFFFFFFFFu)
            {
                bool const invalidate_descriptor = parameter.parameter_ != nullptr && (invalidate_descriptors || parameter.id_ != parameter.parameter_->id_);
                if(parameter.parameter_ != nullptr) parameter.id_ = parameter.parameter_->id_;
                GFX_ASSERT(parameter.parameter_ != nullptr || parameter.variable_size_ > 0);
                switch(parameter.type_)
                {
                case Kernel::Parameter::kType_Buffer:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_Buffer)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a buffer object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        GfxBuffer const &buffer = parameter.parameter_->data_.buffer_;
                        if(!buffer_handles_.has_handle(buffer.handle))
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid buffer object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an invalid buffer object
                        }
                        if(buffer.cpu_access == kGfxCpuAccess_Read)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind buffer object with read CPU access as a shader resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // invalid buffer object for shader SRV
                        }
                        if(buffer.stride == 0)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind buffer object with a stride of 0 as a shader resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // invalid buffer stride
                        }
                        if(buffer.size < buffer.stride)
                        {
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // invalid buffer size
                        }
                        Buffer &gfx_buffer = buffers_[buffer];
                        SetObjectName(gfx_buffer, buffer.name);
                        if(buffer.cpu_access == kGfxCpuAccess_None)
                            transitionResource(gfx_buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                        if(!invalidate_descriptor) break;   // already up to date
                        if(buffer.stride != GFX_ALIGN(buffer.stride, 4))
                            GFX_PRINTLN("Warning: Encountered a buffer stride of %u that isn't 4-byte aligned for parameter `%s' of program `%s/%s'; is this intentional?", buffer.stride, parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srv_desc.Buffer.FirstElement = gfx_buffer.data_offset_ / buffer.stride;
                        srv_desc.Buffer.NumElements = (uint32_t)(buffer.size / buffer.stride);
                        srv_desc.Buffer.StructureByteStride = buffer.stride;
                        device_->CreateShaderResourceView(gfx_buffer.resource_, &srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_));
                    }
                    break;
                case Kernel::Parameter::kType_RWBuffer:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_Buffer)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a buffer object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        GfxBuffer const &buffer = parameter.parameter_->data_.buffer_;
                        if(!buffer_handles_.has_handle(buffer.handle))
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid buffer object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an invalid buffer object
                        }
                        if(buffer.cpu_access != kGfxCpuAccess_None)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind buffer object with read/write CPU access as a RW shader resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // invalid buffer object for shader UAV
                        }
                        if(buffer.stride == 0)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind buffer object with a stride of 0 as a RW shader resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // invalid buffer stride
                        }
                        if(buffer.size < buffer.stride)
                        {
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // invalid buffer size
                        }
                        Buffer &gfx_buffer = buffers_[buffer];
                        SetObjectName(gfx_buffer, buffer.name);
                        transitionResource(gfx_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                        if(!invalidate_descriptor) break;   // already up to date
                        if(buffer.stride != GFX_ALIGN(buffer.stride, 4))
                            GFX_PRINTLN("Warning: Encountered a buffer stride of %u that isn't 4-byte aligned for parameter `%s' of program `%s/%s'; is this intentional?", buffer.stride, parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                        uav_desc.Buffer.FirstElement = gfx_buffer.data_offset_ / buffer.stride;
                        uav_desc.Buffer.NumElements = (uint32_t)(buffer.size / buffer.stride);
                        uav_desc.Buffer.StructureByteStride = buffer.stride;
                        device_->CreateUnorderedAccessView(gfx_buffer.resource_, nullptr, &uav_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_));
                    }
                    break;
                case Kernel::Parameter::kType_Texture2D:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_Image)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a texture object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        D3D12_SHADER_RESOURCE_VIEW_DESC dummy_srv_desc = {};
                        dummy_srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        dummy_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        dummy_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        for(uint32_t j = 0; j < parameter.descriptor_count_; ++j)
                            if(j >= parameter.parameter_->data_.image_.texture_count)
                            {
                                if(!invalidate_descriptor) continue;    // already up to date
                                device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                            }
                            else
                            {
                                GfxTexture const &texture = parameter.parameter_->data_.image_.textures_[j];
                                if(!texture_handles_.has_handle(texture.handle))
                                {
                                    if(texture.handle != 0)
                                        GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid texture object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                    device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                                    continue;   // user set an invalid texture object
                                }
                                if(!texture.is2D())
                                {
                                    GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind non-2D texture object as a 2D sampler resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                    if(!invalidate_descriptor) continue;    // already up to date
                                    device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                                    continue;   // invalid texture object for shader SRV
                                }
                                Texture &gfx_texture = textures_[texture];
                                SetObjectName(gfx_texture, texture.name);
                                transitionResource(gfx_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                                if(!invalidate_descriptor) continue;    // already up to date
                                D3D12_RESOURCE_DESC const &resource_desc = gfx_texture.resource_->GetDesc();
                                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                                srv_desc.Format = GetCBVSRVUAVFormat(resource_desc.Format);
                                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                                srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                                srv_desc.Texture2D.MostDetailedMip = GFX_MIN(parameter.parameter_->data_.image_.mip_level_, GFX_MAX((uint32_t)resource_desc.MipLevels, 1u) - 1);
                                srv_desc.Texture2D.MipLevels = 0xFFFFFFFFu; // select all mipmaps from MostDetailedMip on down to least detailed
                                device_->CreateShaderResourceView(gfx_texture.resource_, &srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                            }
                    }
                    break;
                case Kernel::Parameter::kType_RWTexture2D:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_Image)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a texture object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        GfxTexture const &texture = *parameter.parameter_->data_.image_.textures_;
                        if(!texture_handles_.has_handle(texture.handle))
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid texture object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an invalid texture object
                        }
                        if(!texture.is2D())
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind non-2D texture object as a 2D image resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // invalid texture object for shader UAV
                        }
                        Texture &gfx_texture = textures_[texture];
                        SetObjectName(gfx_texture, texture.name);
                        transitionResource(gfx_texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                        if(!invalidate_descriptor) break;   // already up to date
                        D3D12_RESOURCE_DESC const &resource_desc = gfx_texture.resource_->GetDesc();
                        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                        uav_desc.Format = GetCBVSRVUAVFormat(resource_desc.Format);
                        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                        uav_desc.Texture2D.MipSlice = parameter.parameter_->data_.image_.mip_level_;
                        device_->CreateUnorderedAccessView(gfx_texture.resource_, nullptr, &uav_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_));
                    }
                    break;
                case Kernel::Parameter::kType_Texture2DArray:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_Image)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a texture object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        D3D12_SHADER_RESOURCE_VIEW_DESC dummy_srv_desc = {};
                        dummy_srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        dummy_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        dummy_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        for(uint32_t j = 0; j < parameter.descriptor_count_; ++j)
                            if(j >= parameter.parameter_->data_.image_.texture_count)
                            {
                                if(!invalidate_descriptor) continue;    // already up to date
                                device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                            }
                            else
                            {
                                GfxTexture const &texture = parameter.parameter_->data_.image_.textures_[j];
                                if(!texture_handles_.has_handle(texture.handle))
                                {
                                    if(texture.handle != 0)
                                        GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid texture object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                    device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                                    continue;   // user set an invalid texture object
                                }
                                if(!texture.is2DArray())
                                {
                                    GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind non-2D array texture object as a 2D sampler array resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                    if(!invalidate_descriptor) continue;    // already up to date
                                    device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                                    continue;   // invalid texture object for shader SRV
                                }
                                Texture &gfx_texture = textures_[texture];
                                SetObjectName(gfx_texture, texture.name);
                                transitionResource(gfx_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                                if(!invalidate_descriptor) continue;    // already up to date
                                D3D12_RESOURCE_DESC const &resource_desc = gfx_texture.resource_->GetDesc();
                                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                                srv_desc.Format = GetCBVSRVUAVFormat(resource_desc.Format);
                                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                                srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                                srv_desc.Texture2DArray.MostDetailedMip = GFX_MIN(parameter.parameter_->data_.image_.mip_level_, GFX_MAX((uint32_t)resource_desc.MipLevels, 1u) - 1);
                                srv_desc.Texture2DArray.MipLevels = 0xFFFFFFFFu;    // select all mipmaps from MostDetailedMip on down to least detailed
                                srv_desc.Texture2DArray.ArraySize = resource_desc.DepthOrArraySize;
                                device_->CreateShaderResourceView(gfx_texture.resource_, &srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                            }
                    }
                    break;
                case Kernel::Parameter::kType_RWTexture2DArray:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_Image)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a texture object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        GfxTexture const &texture = *parameter.parameter_->data_.image_.textures_;
                        if(!texture_handles_.has_handle(texture.handle))
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid texture object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an invalid texture object
                        }
                        if(!texture.is2DArray() && !texture.isCube())
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind non-2D array texture object as a 2D image array resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // invalid texture object for shader UAV
                        }
                        Texture &gfx_texture = textures_[texture];
                        SetObjectName(gfx_texture, texture.name);
                        transitionResource(gfx_texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                        if(!invalidate_descriptor) break;   // already up to date
                        D3D12_RESOURCE_DESC const &resource_desc = gfx_texture.resource_->GetDesc();
                        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                        uav_desc.Format = GetCBVSRVUAVFormat(resource_desc.Format);
                        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                        uav_desc.Texture2DArray.MipSlice = parameter.parameter_->data_.image_.mip_level_;
                        uav_desc.Texture2DArray.ArraySize = resource_desc.DepthOrArraySize;
                        device_->CreateUnorderedAccessView(gfx_texture.resource_, nullptr, &uav_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_));
                    }
                    break;
                case Kernel::Parameter::kType_Texture3D:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_Image)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a texture object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        D3D12_SHADER_RESOURCE_VIEW_DESC dummy_srv_desc = {};
                        dummy_srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        dummy_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                        dummy_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        for(uint32_t j = 0; j < parameter.descriptor_count_; ++j)
                            if(j >= parameter.parameter_->data_.image_.texture_count)
                            {
                                if(!invalidate_descriptor) continue;    // already up to date
                                device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                            }
                            else
                            {
                                GfxTexture const &texture = parameter.parameter_->data_.image_.textures_[j];
                                if(!texture_handles_.has_handle(texture.handle))
                                {
                                    if(texture.handle != 0)
                                        GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid texture object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                    device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                                    continue;   // user set an invalid texture object
                                }
                                if(!texture.is3D())
                                {
                                    GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind non-3D texture object as a 3D sampler resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                    if(!invalidate_descriptor) continue;    // already up to date
                                    device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                                    continue;   // invalid texture object for shader SRV
                                }
                                Texture &gfx_texture = textures_[texture];
                                SetObjectName(gfx_texture, texture.name);
                                transitionResource(gfx_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                                if(!invalidate_descriptor) continue;    // already up to date
                                D3D12_RESOURCE_DESC const &resource_desc = gfx_texture.resource_->GetDesc();
                                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                                srv_desc.Format = GetCBVSRVUAVFormat(resource_desc.Format);
                                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                                srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                                srv_desc.Texture3D.MostDetailedMip = GFX_MIN(parameter.parameter_->data_.image_.mip_level_, GFX_MAX((uint32_t)resource_desc.MipLevels, 1u) - 1);
                                srv_desc.Texture3D.MipLevels = 0xFFFFFFFFu; // select all mipmaps from MostDetailedMip on down to least detailed
                                device_->CreateShaderResourceView(gfx_texture.resource_, &srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                            }
                    }
                    break;
                case Kernel::Parameter::kType_RWTexture3D:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_Image)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a texture object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        GfxTexture const &texture = *parameter.parameter_->data_.image_.textures_;
                        if(!texture_handles_.has_handle(texture.handle))
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid texture object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an invalid texture object
                        }
                        if(!texture.is3D())
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind non-3D texture object as a 3D image resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // invalid texture object for shader UAV
                        }
                        Texture &gfx_texture = textures_[texture];
                        SetObjectName(gfx_texture, texture.name);
                        transitionResource(gfx_texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                        if(!invalidate_descriptor) break;   // already up to date
                        D3D12_RESOURCE_DESC const &resource_desc = gfx_texture.resource_->GetDesc();
                        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                        uav_desc.Format = GetCBVSRVUAVFormat(resource_desc.Format);
                        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                        uav_desc.Texture3D.MipSlice = parameter.parameter_->data_.image_.mip_level_;
                        uav_desc.Texture3D.WSize = resource_desc.DepthOrArraySize;
                        device_->CreateUnorderedAccessView(gfx_texture.resource_, nullptr, &uav_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_));
                    }
                    break;
                case Kernel::Parameter::kType_TextureCube:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_Image)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected a texture object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        D3D12_SHADER_RESOURCE_VIEW_DESC dummy_srv_desc = {};
                        dummy_srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        dummy_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                        dummy_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        for(uint32_t j = 0; j < parameter.descriptor_count_; ++j)
                            if(j >= parameter.parameter_->data_.image_.texture_count)
                            {
                                if(!invalidate_descriptor) continue;    // already up to date
                                device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                            }
                            else
                            {
                                GfxTexture const &texture = parameter.parameter_->data_.image_.textures_[j];
                                if(!texture_handles_.has_handle(texture.handle))
                                {
                                    if(texture.handle != 0)
                                        GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid texture object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                    device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                                    continue;   // user set an invalid texture object
                                }
                                if(!texture.isCube())
                                {
                                    GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind non-cube texture object as a cubemap sampler resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                    if(!invalidate_descriptor) continue;    // already up to date
                                    device_->CreateShaderResourceView(nullptr, &dummy_srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                                    continue;   // invalid texture object for shader SRV
                                }
                                Texture &gfx_texture = textures_[texture];
                                SetObjectName(gfx_texture, texture.name);
                                transitionResource(gfx_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                                if(!invalidate_descriptor) continue;    // already up to date
                                D3D12_RESOURCE_DESC const &resource_desc = gfx_texture.resource_->GetDesc();
                                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                                srv_desc.Format = GetCBVSRVUAVFormat(resource_desc.Format);
                                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                                srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                                srv_desc.TextureCube.MostDetailedMip = GFX_MIN(parameter.parameter_->data_.image_.mip_level_, GFX_MAX((uint32_t)resource_desc.MipLevels, 1u) - 1);
                                srv_desc.TextureCube.MipLevels = 0xFFFFFFFFu;   // select all mipmaps from MostDetailedMip on down to least detailed
                                device_->CreateShaderResourceView(gfx_texture.resource_, &srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_ + j));
                            }
                    }
                    break;
                case Kernel::Parameter::kType_AccelerationStructure:
                    {
                        if(parameter.parameter_->type_ != Program::Parameter::kType_AccelerationStructure)
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected an acceleration structure object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        GfxAccelerationStructure const &acceleration_structure = parameter.parameter_->data_.acceleration_structure_.bvh_;
                        if(!acceleration_structure_handles_.has_handle(acceleration_structure.handle))
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid acceleration structure object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an invalid buffer object
                        }
                        AccelerationStructure const &gfx_acceleration_structure = acceleration_structures_[acceleration_structure];
                        const_cast<Program::Parameter *>(parameter.parameter_)->data_.acceleration_structure_.bvh_buffer_ = gfx_acceleration_structure.bvh_buffer_;
                        if(!gfx_acceleration_structure.bvh_buffer_)
                        {
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // acceleration structure hasn't been built yet
                        }
                        GFX_ASSERT(buffer_handles_.has_handle(gfx_acceleration_structure.bvh_buffer_.handle));
                        Buffer &buffer = buffers_[gfx_acceleration_structure.bvh_buffer_];
                        SetObjectName(buffer, acceleration_structure.name);
                        if(buffer_handles_.has_handle(raytracing_scratch_buffer_.handle))
                            transitionResource(buffers_[raytracing_scratch_buffer_], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                        GFX_ASSERT(*buffer.resource_state_ == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
                        if(!invalidate_descriptor) break;   // already up to date
                        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
                        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srv_desc.RaytracingAccelerationStructure.Location = buffer.resource_->GetGPUVirtualAddress() + buffer.data_offset_;
                        device_->CreateShaderResourceView(nullptr, &srv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_));
                    }
                    break;
                case Kernel::Parameter::kType_ConstantBuffer:
                    {
                        void *data = nullptr;
                        D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
                        if(parameter.variable_size_ > 0)
                        {
                            cbv_desc.BufferLocation = allocateConstantMemory(parameter.variable_size_, data);
                            if(data != nullptr) populateRootConstants(program, parameter, (uint32_t *)data);
                            cbv_desc.SizeInBytes = GFX_ALIGN(parameter.variable_size_, 256);
                        }
                        else if(parameter.parameter_->type_ == Program::Parameter::kType_Constants)
                        {
                            cbv_desc.BufferLocation = allocateConstantMemory(parameter.parameter_->data_size_, data);
                            if(data != nullptr) memcpy(data, parameter.parameter_->data_.constants_, parameter.parameter_->data_size_);
                            cbv_desc.SizeInBytes = GFX_ALIGN(parameter.parameter_->data_size_, 256);
                        }
                        else if(parameter.parameter_->type_ == Program::Parameter::kType_Buffer)
                        {
                            GfxBuffer const &buffer = parameter.parameter_->data_.buffer_;
                            if(!buffer_handles_.has_handle(buffer.handle))
                            {
                                GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Found invalid buffer object for parameter `%s' of program `%s/%s'; cannot bind to pipeline", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                descriptor_slot = dummy_descriptors_[parameter.type_];
                                freeDescriptor(parameter.descriptor_slot_);
                                parameter.descriptor_slot_ = 0xFFFFFFFFu;
                                break;  // user set an invalid buffer object
                            }
                            if(buffer.cpu_access != kGfxCpuAccess_Write)
                            {
                                GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind buffer object that does not have write CPU access as a constant shader resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                descriptor_slot = dummy_descriptors_[parameter.type_];
                                freeDescriptor(parameter.descriptor_slot_);
                                parameter.descriptor_slot_ = 0xFFFFFFFFu;
                                break;  // user set an invalid buffer object
                            }
                            if(buffer.size > 0xFFFFFFFFull)
                            {
                                GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot bind buffer object that's larger than 4GiB as a constant shader resource for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                                descriptor_slot = dummy_descriptors_[parameter.type_];
                                freeDescriptor(parameter.descriptor_slot_);
                                parameter.descriptor_slot_ = 0xFFFFFFFFu;
                                break;  // constant buffer is too large
                            }
                            Buffer &gfx_buffer = buffers_[buffer];
                            SetObjectName(gfx_buffer, buffer.name);
                            GFX_ASSERT(*gfx_buffer.resource_state_ == D3D12_RESOURCE_STATE_GENERIC_READ);
                            GFX_ASSERT(gfx_buffer.data_offset_ == GFX_ALIGN(gfx_buffer.data_offset_, 256));
                            cbv_desc.BufferLocation = gfx_buffer.resource_->GetGPUVirtualAddress() + gfx_buffer.data_offset_;
                            cbv_desc.SizeInBytes = GFX_ALIGN((uint32_t)buffer.size, 256);
                        }
                        else
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected constant or buffer object", parameter.parameter_->getTypeName(), parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // user set an unrelated parameter type
                        }
                        if(cbv_desc.BufferLocation == 0)
                        {
                            GFX_PRINT_ERROR(kGfxResult_OutOfMemory, "Unable to allocate constant memory for parameter `%s' of program `%s/%s'", parameter.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                            descriptor_slot = dummy_descriptors_[parameter.type_];
                            freeDescriptor(parameter.descriptor_slot_);
                            parameter.descriptor_slot_ = 0xFFFFFFFFu;
                            break;  // failed to allocate constant memory
                        }
                        device_->CreateConstantBufferView(&cbv_desc, descriptors_.getCPUHandle(parameter.descriptor_slot_));
                    }
                    break;
                default:
                    GFX_ASSERT(0);  // missing implementation
                    break;
                }
            }
            GFX_ASSERT(descriptor_slot < (descriptors_.descriptor_heap_ != nullptr ? descriptors_.descriptor_heap_->GetDesc().NumDescriptors : 0));
            if(is_compute)
                command_list_->SetComputeRootDescriptorTable(i, descriptors_.getGPUHandle(descriptor_slot));
            else
                command_list_->SetGraphicsRootDescriptorTable(i, descriptors_.getGPUHandle(descriptor_slot));
        }
        if(!is_compute && kernel.vertex_stride_ > 0)
        {
            if(indexed && (force_install_index_buffer_ || bound_index_buffer_.handle != installed_index_buffer_.handle))
            {
                D3D12_INDEX_BUFFER_VIEW ibv_desc = {};
                if(!buffer_handles_.has_handle(bound_index_buffer_.handle))
                    bound_index_buffer_ = {};
                else
                {
                    Buffer &gfx_buffer = buffers_[bound_index_buffer_];
                    SetObjectName(gfx_buffer, bound_index_buffer_.name);
                    ibv_desc.BufferLocation = gfx_buffer.resource_->GetGPUVirtualAddress() + gfx_buffer.data_offset_;
                    ibv_desc.SizeInBytes = (uint32_t)bound_index_buffer_.size;
                    ibv_desc.Format = (bound_index_buffer_.stride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
                    // TODO: might need multiple resource state flags... (gboisse)
                    if(bound_index_buffer_.cpu_access == kGfxCpuAccess_None)
                        transitionResource(gfx_buffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
                }
                command_list_->IASetIndexBuffer(&ibv_desc);
                installed_index_buffer_ = bound_index_buffer_;
                force_install_index_buffer_ = false;
            }
            if(force_install_vertex_buffer_ || bound_vertex_buffer_.handle != installed_vertex_buffer_.handle)
            {
                D3D12_VERTEX_BUFFER_VIEW vbv_desc = {};
                if(!buffer_handles_.has_handle(bound_vertex_buffer_.handle))
                    bound_vertex_buffer_ = {};
                else
                {
                    Buffer &gfx_buffer = buffers_[bound_vertex_buffer_];
                    SetObjectName(gfx_buffer, bound_vertex_buffer_.name);
                    vbv_desc.BufferLocation = gfx_buffer.resource_->GetGPUVirtualAddress() + gfx_buffer.data_offset_;
                    vbv_desc.SizeInBytes = (uint32_t)bound_vertex_buffer_.size;
                    vbv_desc.StrideInBytes = GFX_MAX(bound_vertex_buffer_.stride, kernel.vertex_stride_);
                    // TODO: might need multiple resource state flags... (gboisse)
                    if(bound_vertex_buffer_.cpu_access == kGfxCpuAccess_None)
                        transitionResource(gfx_buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                }
                command_list_->IASetVertexBuffers(0, 1, &vbv_desc);
                installed_vertex_buffer_ = bound_vertex_buffer_;
                force_install_vertex_buffer_ = false;
            }
        }
        return kGfxResult_NoError;
    }

    GfxResult ensureTextureHasUsageFlag(Texture &texture, D3D12_RESOURCE_FLAGS usage_flag)
    {
        D3D12_RESOURCE_DESC resource_desc = texture.resource_->GetDesc();
        if(!((resource_desc.Flags & usage_flag) != 0))
        {
            ID3D12Resource *resource = nullptr;
            D3D12MA::Allocation *allocation = nullptr;
            D3D12MA::ALLOCATION_DESC allocation_desc = {};
            allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            resource_desc.Flags |= usage_flag;  // add usage flag
            GFX_TRY(createResource(allocation_desc, resource_desc, D3D12_RESOURCE_STATE_COPY_DEST, &allocation, IID_PPV_ARGS(&resource)));
            if(texture.resource_state_ != D3D12_RESOURCE_STATE_COPY_SOURCE)
            {
                D3D12_RESOURCE_BARRIER resource_barrier = {};
                resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                resource_barrier.Transition.pResource = texture.resource_;
                resource_barrier.Transition.StateBefore = texture.resource_state_;
                resource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
                resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                command_list_->ResourceBarrier(1, &resource_barrier);
            }
            collect(texture);   // release previous texture
            command_list_->CopyResource(resource, texture.resource_);
            texture.resource_state_ = D3D12_RESOURCE_STATE_COPY_DEST;
            for(uint32_t i = 0; i < ARRAYSIZE(texture.dsv_descriptor_slots_); ++i)
            {
                texture.dsv_descriptor_slots_[i].resize(resource_desc.DepthOrArraySize);
                for(size_t j = 0; j < texture.dsv_descriptor_slots_[i].size(); ++j)
                    texture.dsv_descriptor_slots_[i][j] = 0xFFFFFFFFu;
            }
            for(uint32_t i = 0; i < ARRAYSIZE(texture.rtv_descriptor_slots_); ++i)
            {
                texture.rtv_descriptor_slots_[i].resize(resource_desc.DepthOrArraySize);
                for(size_t j = 0; j < texture.rtv_descriptor_slots_[i].size(); ++j)
                    texture.rtv_descriptor_slots_[i][j] = 0xFFFFFFFFu;
            }
            texture.Object::flags_ &= ~Object::kFlag_Named;
            texture.allocation_ = allocation;
            texture.resource_ = resource;
        }
        return kGfxResult_NoError;
    }

    GfxResult ensureTextureHasDepthStencilView(GfxTexture const &texture, Texture &gfx_texture, uint32_t mip_level, uint32_t slice)
    {
        GFX_TRY(ensureTextureHasUsageFlag(gfx_texture, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
        GFX_ASSERT(mip_level < ARRAYSIZE(gfx_texture.dsv_descriptor_slots_));
        GFX_ASSERT(slice < gfx_texture.dsv_descriptor_slots_[mip_level].size());
        if(gfx_texture.dsv_descriptor_slots_[mip_level][slice] == 0xFFFFFFFFu)
        {
            gfx_texture.dsv_descriptor_slots_[mip_level][slice] = allocateDSVDescriptor();
            if(gfx_texture.dsv_descriptor_slots_[mip_level][slice] == 0xFFFFFFFFu)
                return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to allocate DSV descriptor");
            D3D12_RESOURCE_DESC const resource_desc = gfx_texture.resource_->GetDesc();
            D3D12_DEPTH_STENCIL_VIEW_DESC
            dsv_desc        = {};
            dsv_desc.Format = resource_desc.Format;
            if(texture.is2D())
            {
                dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Texture2D.MipSlice = mip_level;
            }
            else
            {
                dsv_desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsv_desc.Texture2DArray.MipSlice        = mip_level;
                dsv_desc.Texture2DArray.FirstArraySlice = slice;
                dsv_desc.Texture2DArray.ArraySize       = 1;
            }
            device_->CreateDepthStencilView(gfx_texture.resource_, &dsv_desc, dsv_descriptors_.getCPUHandle(gfx_texture.dsv_descriptor_slots_[mip_level][slice]));
        }
        return kGfxResult_NoError;
    }

    GfxResult ensureTextureHasRenderTargetView(GfxTexture const &texture, Texture &gfx_texture, uint32_t mip_level, uint32_t slice)
    {
        GFX_TRY(ensureTextureHasUsageFlag(gfx_texture, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET));
        GFX_ASSERT(mip_level < ARRAYSIZE(gfx_texture.rtv_descriptor_slots_));
        GFX_ASSERT(slice < gfx_texture.rtv_descriptor_slots_[mip_level].size());
        if(gfx_texture.rtv_descriptor_slots_[mip_level][slice] == 0xFFFFFFFFu)
        {
            gfx_texture.rtv_descriptor_slots_[mip_level][slice] = allocateRTVDescriptor();
            if(gfx_texture.rtv_descriptor_slots_[mip_level][slice] == 0xFFFFFFFFu)
                return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to allocate RTV descriptor");
            D3D12_RESOURCE_DESC const resource_desc = gfx_texture.resource_->GetDesc();
            D3D12_RENDER_TARGET_VIEW_DESC
            rtv_desc        = {};
            rtv_desc.Format = resource_desc.Format;
            if(texture.is2D())
            {
                rtv_desc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtv_desc.Texture2D.MipSlice = mip_level;
            }
            else if(texture.is3D())
            {
                rtv_desc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
                rtv_desc.Texture3D.MipSlice    = mip_level;
                rtv_desc.Texture3D.FirstWSlice = slice;
                rtv_desc.Texture3D.WSize       = 1;
            }
            else
            {
                rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                rtv_desc.Texture2DArray.MipSlice        = mip_level;
                rtv_desc.Texture2DArray.FirstArraySlice = slice;
                rtv_desc.Texture2DArray.ArraySize       = 1;
            }
            device_->CreateRenderTargetView(gfx_texture.resource_, &rtv_desc, rtv_descriptors_.getCPUHandle(gfx_texture.rtv_descriptor_slots_[mip_level][slice]));
        }
        return kGfxResult_NoError;
    }

    D3D12_GPU_VIRTUAL_ADDRESS allocateConstantMemory(uint64_t data_size, void *&data)
    {
        D3D12_GPU_VIRTUAL_ADDRESS gpu_addr = {};
        Buffer *constant_buffer = buffers_.at(constant_buffer_pool_[fence_index_]);
        uint64_t constant_buffer_size = (constant_buffer != nullptr ? constant_buffer->resource_->GetDesc().Width : 0);
        if(constant_buffer_pool_cursors_[fence_index_] * 256 + data_size > constant_buffer_size)
        {
            constant_buffer_size += data_size;
            constant_buffer_size += ((constant_buffer_size + 2) >> 1);
            constant_buffer_size = GFX_ALIGN(constant_buffer_size, 65536);
            destroyBuffer(constant_buffer_pool_[fence_index_]); // release previous memory
            constant_buffer_pool_[fence_index_] = createBuffer(constant_buffer_size, nullptr, kGfxCpuAccess_Write);
            if(!constant_buffer_pool_[fence_index_]) { data = nullptr; return gpu_addr; }   // out of memory
            GFX_SNPRINTF(constant_buffer_pool_[fence_index_].name, sizeof(constant_buffer_pool_[fence_index_].name), "gfx_ConstantBufferPool%u", fence_index_);
            constant_buffer = &buffers_[constant_buffer_pool_[fence_index_]];
            SetObjectName(*constant_buffer, constant_buffer_pool_[fence_index_].name);
        }
        GFX_ASSERT(constant_buffer != nullptr && constant_buffer->data_ != nullptr);
        uint64_t const data_offset = constant_buffer_pool_cursors_[fence_index_] * 256;
        gpu_addr = constant_buffer->resource_->GetGPUVirtualAddress() + data_offset;
        constant_buffer_pool_cursors_[fence_index_] += (data_size + 255) / 256;
        data = (char *)constant_buffer->data_ + data_offset;
        return gpu_addr;
    }

    GfxResult allocateRaytracingScratch(uint64_t scratch_data_size)
    {
        if(scratch_data_size > raytracing_scratch_buffer_.size)
        {
            destroyBuffer(raytracing_scratch_buffer_);
            scratch_data_size = GFX_ALIGN(scratch_data_size, 65536);
            raytracing_scratch_buffer_ = createBuffer(scratch_data_size, nullptr, kGfxCpuAccess_None, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            if(!raytracing_scratch_buffer_)
                return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to create raytracing scratch buffer");
            strcpy(raytracing_scratch_buffer_.name, "gfx_RaytracingScratchBuffer");
            Buffer &gfx_buffer = buffers_[raytracing_scratch_buffer_];
            SetObjectName(gfx_buffer, raytracing_scratch_buffer_.name);
        }
        GFX_ASSERT(buffer_handles_.has_handle(raytracing_scratch_buffer_.handle));
        return kGfxResult_NoError;
    }

    GfxResult buildRaytracingPrimitive(GfxRaytracingPrimitive const &raytracing_primitive, RaytracingPrimitive &gfx_raytracing_primitive, bool update)
    {
        if(gfx_raytracing_primitive.index_stride_ != 0 && !buffer_handles_.has_handle(gfx_raytracing_primitive.index_buffer_.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot update a raytracing primitive that's pointing to an invalid index buffer object");
        if(!buffer_handles_.has_handle(gfx_raytracing_primitive.vertex_buffer_.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot update a raytracing primitive that's pointing to an invalid vertex buffer object");
        GFX_ASSERT(gfx_raytracing_primitive.index_stride_ == 0 || gfx_raytracing_primitive.index_buffer_.size / gfx_raytracing_primitive.index_stride_ <= 0xFFFFFFFFull);
        GFX_ASSERT(gfx_raytracing_primitive.vertex_stride_ > 0 && gfx_raytracing_primitive.vertex_buffer_.size / gfx_raytracing_primitive.vertex_stride_ <= 0xFFFFFFFFull);
        Buffer *gfx_index_buffer = (gfx_raytracing_primitive.index_stride_ != 0 ? &buffers_[gfx_raytracing_primitive.index_buffer_] : nullptr);
        if(gfx_index_buffer != nullptr) SetObjectName(*gfx_index_buffer, gfx_raytracing_primitive.index_buffer_.name);
        Buffer &gfx_vertex_buffer = buffers_[gfx_raytracing_primitive.vertex_buffer_];
        SetObjectName(gfx_vertex_buffer, gfx_raytracing_primitive.vertex_buffer_.name);
        GFX_TRY(updateRaytracingPrimitive(raytracing_primitive, gfx_raytracing_primitive));
        if((gfx_raytracing_primitive.index_stride_ != 0 && gfx_raytracing_primitive.index_buffer_.size == 0) ||
            gfx_raytracing_primitive.vertex_buffer_.size == 0)
        {
            destroyBuffer(gfx_raytracing_primitive.bvh_buffer_);
            gfx_raytracing_primitive.bvh_buffer_ = {};
            return kGfxResult_NoError;
        }
        D3D12_RAYTRACING_GEOMETRY_DESC geometry_desc = {};
        geometry_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        if(gfx_raytracing_primitive.index_stride_ != 0)
        {
            GFX_ASSERT(gfx_index_buffer != nullptr);    // should never happen
            geometry_desc.Triangles.IndexFormat = (gfx_raytracing_primitive.index_stride_ == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
            geometry_desc.Triangles.IndexCount = (uint32_t)(gfx_raytracing_primitive.index_buffer_.size / gfx_raytracing_primitive.index_stride_);
            geometry_desc.Triangles.IndexBuffer = gfx_index_buffer->resource_->GetGPUVirtualAddress() + gfx_index_buffer->data_offset_;
        }
        geometry_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        geometry_desc.Triangles.VertexCount = (uint32_t)(gfx_raytracing_primitive.vertex_buffer_.size / gfx_raytracing_primitive.vertex_stride_);
        geometry_desc.Triangles.VertexBuffer.StartAddress = gfx_vertex_buffer.resource_->GetGPUVirtualAddress() + gfx_vertex_buffer.data_offset_;
        geometry_desc.Triangles.VertexBuffer.StrideInBytes = gfx_raytracing_primitive.vertex_stride_;
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blas_inputs = {};
        blas_inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        blas_inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        blas_inputs.NumDescs = 1;
        blas_inputs.pGeometryDescs = &geometry_desc;
        if(update)
            blas_inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blas_info = {};
        device_->GetRaytracingAccelerationStructurePrebuildInfo(&blas_inputs, &blas_info);
        uint64_t const scratch_data_size = GFX_MAX(blas_info.ScratchDataSizeInBytes, blas_info.UpdateScratchDataSizeInBytes);
        GFX_TRY(allocateRaytracingScratch(scratch_data_size));  // ensure scratch is large enough
        uint64_t const bvh_data_size = GFX_ALIGN(blas_info.ResultDataMaxSizeInBytes, 65536);
        if(bvh_data_size > gfx_raytracing_primitive.bvh_buffer_.size)
        {
            destroyBuffer(gfx_raytracing_primitive.bvh_buffer_);
            blas_inputs.Flags &= ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
            gfx_raytracing_primitive.bvh_buffer_ = createBuffer(bvh_data_size, nullptr, kGfxCpuAccess_None, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
            if(!gfx_raytracing_primitive.bvh_buffer_)
                return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to create raytracing primitive buffer");
        }
        GFX_ASSERT(buffer_handles_.has_handle(gfx_raytracing_primitive.bvh_buffer_.handle));
        GFX_ASSERT(buffer_handles_.has_handle(raytracing_scratch_buffer_.handle));
        Buffer &gfx_buffer = buffers_[gfx_raytracing_primitive.bvh_buffer_];
        Buffer &gfx_scratch_buffer = buffers_[raytracing_scratch_buffer_];
        SetObjectName(gfx_buffer, raytracing_primitive.name);
        if(gfx_raytracing_primitive.index_stride_ != 0)
            transitionResource(buffers_[gfx_raytracing_primitive.index_buffer_], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        transitionResource(buffers_[gfx_raytracing_primitive.vertex_buffer_], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        transitionResource(gfx_scratch_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        submitPipelineBarriers();   // ensure scratch is not in use
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};
        build_desc.DestAccelerationStructureData = gfx_buffer.resource_->GetGPUVirtualAddress() + gfx_buffer.data_offset_;
        build_desc.Inputs = blas_inputs;
        if((blas_inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) != 0)
            build_desc.SourceAccelerationStructureData = gfx_buffer.resource_->GetGPUVirtualAddress() + gfx_buffer.data_offset_;
        build_desc.ScratchAccelerationStructureData = gfx_scratch_buffer.resource_->GetGPUVirtualAddress() + gfx_scratch_buffer.data_offset_;
        command_list_->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);
        return kGfxResult_NoError;
    }

    GfxResult updateRaytracingPrimitive(GfxRaytracingPrimitive const &raytracing_primitive, RaytracingPrimitive &gfx_raytracing_primitive)
    {
        GfxAccelerationStructure const &acceleration_structure = gfx_raytracing_primitive.acceleration_structure;
        if(!acceleration_structure_handles_.has_handle(acceleration_structure.handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot update a raytracing primitive that's pointing to an invalid acceleration structure object");
        AccelerationStructure &gfx_acceleration_structure = acceleration_structures_[acceleration_structure];
        if(gfx_raytracing_primitive.index_ >= (uint32_t)gfx_acceleration_structure.raytracing_primitives_.size() ||
           raytracing_primitive.handle != gfx_acceleration_structure.raytracing_primitives_[gfx_raytracing_primitive.index_].handle)
        {
            uint32_t const raytracing_primitive_count = (uint32_t)gfx_acceleration_structure.raytracing_primitives_.size();
            for(gfx_raytracing_primitive.index_ = 0; gfx_raytracing_primitive.index_ < raytracing_primitive_count; ++gfx_raytracing_primitive.index_)
                if(gfx_acceleration_structure.raytracing_primitives_[gfx_raytracing_primitive.index_].handle == raytracing_primitive.handle) break;
            if(gfx_raytracing_primitive.index_ >= raytracing_primitive_count)
                return GFX_SET_ERROR(kGfxResult_InternalError, "Cannot update a raytracing primitive that does not belong to the acceleration structure object it was created from");
        }
        gfx_acceleration_structure.raytracing_primitives_[gfx_raytracing_primitive.index_] = raytracing_primitive;
        gfx_acceleration_structure.needs_update_ = true;
        return kGfxResult_NoError;
    }

    void populateRootConstants(Program const &program, Kernel::Parameter &parameter, uint32_t *root_constants)
    {
        memset(root_constants, 0, parameter.variable_size_);
        for(uint32_t i = 0; i < parameter.variable_count_; ++i)
        {
            Kernel::Parameter::Variable &variable = parameter.variables_[i];
            if(variable.parameter_ == nullptr)
            {
                Program::Parameters::const_iterator const it = program.parameters_.find(variable.parameter_id_);
                if(it != program.parameters_.end()) variable.parameter_ = &(*it).second;
            }
            if(variable.parameter_ == nullptr) continue;
            if(variable.parameter_->type_ != Program::Parameter::kType_Constants)
            {
                if(variable.id_ != variable.parameter_->id_)
                    GFX_PRINT_ERROR(kGfxResult_InvalidParameter, "Found unrelated type `%s' for parameter `%s' of program `%s/%s'; expected constants", variable.parameter_->getTypeName(), variable.parameter_->name_.c_str(), program.file_path_.c_str(), program.file_name_.c_str());
                variable.id_ = variable.parameter_->id_;
                continue;   // invalid parameter
            }
            memcpy(&root_constants[variable.data_start_ / sizeof(uint32_t)], variable.parameter_->data_.constants_, GFX_MIN(variable.data_size_, variable.parameter_->data_size_));
            variable.id_ = variable.parameter_->id_;
        }
    }

    void bindDrawIdBuffer()
    {
        GFX_ASSERT(buffer_handles_.has_handle(draw_id_buffer_.handle));
        Buffer &gfx_buffer = buffers_[draw_id_buffer_];
        SetObjectName(buffers_[draw_id_buffer_], draw_id_buffer_.name);
        D3D12_VERTEX_BUFFER_VIEW
        vbv_desc                = {};
        vbv_desc.BufferLocation = gfx_buffer.resource_->GetGPUVirtualAddress() + gfx_buffer.data_offset_;
        vbv_desc.SizeInBytes    = (uint32_t)draw_id_buffer_.size;
        vbv_desc.StrideInBytes  = sizeof(uint32_t);
        command_list_->IASetVertexBuffers(1, 1, &vbv_desc);
    }

    GfxResult populateDrawIdBuffer(uint32_t args_count)
    {
        uint64_t draw_id_buffer_size = args_count * sizeof(uint32_t);
        if(draw_id_buffer_size <= draw_id_buffer_.size) return kGfxResult_NoError;
        GFX_TRY(destroyBuffer(draw_id_buffer_));
        draw_id_buffer_size += ((draw_id_buffer_size + 2) >> 1);
        draw_id_buffer_size = GFX_ALIGN(draw_id_buffer_size, 65536);
        std::vector<uint32_t> draw_ids((size_t)(draw_id_buffer_size / sizeof(uint32_t)));
        for(size_t i = 0; i < draw_ids.size(); ++i) draw_ids[i] = static_cast<uint32_t>(i);
        draw_id_buffer_ = createBuffer(draw_id_buffer_size, draw_ids.data(), kGfxCpuAccess_None, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        if(!draw_id_buffer_)
            return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to create the drawID buffer");
        draw_id_buffer_.setName("gfx_DrawIDBuffer");
        bindDrawIdBuffer(); // bind drawID buffer
        return kGfxResult_NoError;
    }

    void populateDummyDescriptors()
    {
        GFX_ASSERT(ARRAYSIZE(dummy_descriptors_) == Kernel::Parameter::kType_Count);
        for(uint32_t i = 0; i < ARRAYSIZE(dummy_descriptors_); ++i)
            switch(i)
            {
            case Kernel::Parameter::kType_Buffer:
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                    srv_desc.Buffer.StructureByteStride = sizeof(uint32_t);
                    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    device_->CreateShaderResourceView(nullptr, &srv_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_RWBuffer:
                {
                    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                    uav_desc.Buffer.StructureByteStride = sizeof(uint32_t);
                    device_->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_Texture2D:
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    device_->CreateShaderResourceView(nullptr, &srv_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_RWTexture2D:
                {
                    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                    uav_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    device_->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_Texture2DArray:
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    device_->CreateShaderResourceView(nullptr, &srv_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_RWTexture2DArray:
                {
                    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                    uav_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    device_->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_Texture3D:
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    device_->CreateShaderResourceView(nullptr, &srv_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_RWTexture3D:
                {
                    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                    uav_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                    device_->CreateUnorderedAccessView(nullptr, nullptr, &uav_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_TextureCube:
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    device_->CreateShaderResourceView(nullptr, &srv_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_AccelerationStructure:
                if(raytracing_support_)
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
                    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    device_->CreateShaderResourceView(nullptr, &srv_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_Constants:
                {
                    freeDescriptor(dummy_descriptors_[i]);
                    dummy_descriptors_[i] = 0xFFFFFFFFu;
                }
                break;
            case Kernel::Parameter::kType_ConstantBuffer:
                {
                    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
                    device_->CreateConstantBufferView(&cbv_desc, descriptors_.getCPUHandle(dummy_descriptors_[i]));
                }
                break;
            case Kernel::Parameter::kType_Sampler:
                break;  // the sampler dummy descriptor is (re-)populated every time the sampler descriptor heap is resized
            default:
                GFX_ASSERTMSG(0, "An unsupported parameter type `%u' was supplied", i);
                break;
            }
    }

    void createKernel(Program const &program, Kernel &kernel)
    {
        char buffer[2048];
        char const *kernel_type = nullptr;
        GFX_ASSERT(kernel.num_threads_ != nullptr);
        GFX_ASSERT(kernel.cs_bytecode_ == nullptr && kernel.cs_reflection_ == nullptr);
        GFX_ASSERT(kernel.vs_bytecode_ == nullptr && kernel.vs_reflection_ == nullptr);
        GFX_ASSERT(kernel.gs_bytecode_ == nullptr && kernel.gs_reflection_ == nullptr);
        GFX_ASSERT(kernel.ps_bytecode_ == nullptr && kernel.ps_reflection_ == nullptr);
        GFX_ASSERT(kernel.root_signature_ == nullptr);
        GFX_ASSERT(kernel.pipeline_state_ == nullptr);
        GFX_ASSERT(kernel.parameters_ == nullptr);
        if(kernel.isCompute())
        {
            kernel_type = "Compute";
            compileShader(program, kernel, kShaderType_CS, kernel.cs_bytecode_, kernel.cs_reflection_);
            createRootSignature(kernel);
            createComputePipelineState(kernel);
            if(kernel.cs_reflection_ == nullptr)
                for(uint32_t i = 0; i < 3; ++i) kernel.num_threads_[i] = 1;
            else
                kernel.cs_reflection_->GetThreadGroupSize(&kernel.num_threads_[0], &kernel.num_threads_[1], &kernel.num_threads_[2]);
        }
        else
        {
            kernel_type = "Graphics";
            compileShader(program, kernel, kShaderType_VS, kernel.vs_bytecode_, kernel.vs_reflection_);
            compileShader(program, kernel, kShaderType_GS, kernel.gs_bytecode_, kernel.gs_reflection_);
            compileShader(program, kernel, kShaderType_PS, kernel.ps_bytecode_, kernel.ps_reflection_);
            createRootSignature(kernel);
            createGraphicsPipelineState(kernel, kernel.draw_state_);
        }
        if(kernel.root_signature_ != nullptr)
        {
            GFX_SNPRINTF(buffer, sizeof(buffer), "%s::%s_%sRootSignature", program.file_name_ ? program.file_name_.c_str() : program.file_path_.c_str(), kernel.entry_point_.c_str(), kernel_type);
            SetDebugName(kernel.root_signature_, buffer);
        }
        if(kernel.pipeline_state_ != nullptr)
        {
            GFX_SNPRINTF(buffer, sizeof(buffer), "%s::%s_%sPipelineSignature", program.file_name_ ? program.file_name_.c_str() : program.file_path_.c_str(), kernel.entry_point_.c_str(), kernel_type);
            SetDebugName(kernel.pipeline_state_, buffer);
        }
    }

    void reloadKernel(Kernel &kernel)
    {
        if(!program_handles_.has_handle(kernel.program_.handle)) return;
        Program const &program = programs_[kernel.program_];
        kernel.descriptor_heap_id_ = 0;
        if(kernel.cs_bytecode_ != nullptr) { kernel.cs_bytecode_->Release(); kernel.cs_bytecode_ = nullptr; }
        if(kernel.vs_bytecode_ != nullptr) { kernel.vs_bytecode_->Release(); kernel.vs_bytecode_ = nullptr; }
        if(kernel.gs_bytecode_ != nullptr) { kernel.gs_bytecode_->Release(); kernel.gs_bytecode_ = nullptr; }
        if(kernel.ps_bytecode_ != nullptr) { kernel.ps_bytecode_->Release(); kernel.ps_bytecode_ = nullptr; }
        if(kernel.cs_reflection_ != nullptr) { kernel.cs_reflection_->Release(); kernel.cs_reflection_ = nullptr; }
        if(kernel.vs_reflection_ != nullptr) { kernel.vs_reflection_->Release(); kernel.vs_reflection_ = nullptr; }
        if(kernel.gs_reflection_ != nullptr) { kernel.gs_reflection_->Release(); kernel.gs_reflection_ = nullptr; }
        if(kernel.ps_reflection_ != nullptr) { kernel.ps_reflection_->Release(); kernel.ps_reflection_ = nullptr; }
        if(kernel.root_signature_ != nullptr) { collect(kernel.root_signature_); kernel.root_signature_ = nullptr; }
        if(kernel.pipeline_state_ != nullptr) { collect(kernel.pipeline_state_); kernel.pipeline_state_ = nullptr; }
        for(uint32_t i = 0; i < kernel.parameter_count_; ++i)
        {
            freeDescriptor(kernel.parameters_[i].descriptor_slot_);
            for(uint32_t j = 0; j < kernel.parameters_[i].variable_count_; ++j)
                kernel.parameters_[i].variables_[j].~Variable();
            free(kernel.parameters_[i].variables_);
            kernel.parameters_[i].~Parameter();
        }
        free(kernel.parameters_);
        kernel.parameters_ = nullptr;
        kernel.parameter_count_ = 0;
        kernel.vertex_stride_ = 0;
        createKernel(program, kernel);
    }

    void compileShader(Program const &program, Kernel const &kernel, ShaderType shader_type, IDxcBlob *&shader_bytecode, ID3D12ShaderReflection *&shader_reflection)
    {
        char shader_file[4096];
        DxcBuffer shader_source = {};
        IDxcBlobEncoding *dxc_source = nullptr;
        WCHAR wshader_file[ARRAYSIZE(shader_file)];
        GFX_ASSERT(shader_type < kShaderType_Count);

        if(program.file_name_)
        {
            GFX_SNPRINTF(shader_file, sizeof(shader_file), "%s/%s%s", program.file_path_.c_str(), program.file_name_.c_str(), shader_extensions_[shader_type]);
            mbstowcs(wshader_file, shader_file, ARRAYSIZE(shader_file));
            dxc_utils_->LoadFile(wshader_file, nullptr, &dxc_source);
            if(!dxc_source) return; // no source found for this shader
            shader_source.Ptr = dxc_source->GetBufferPointer();
            shader_source.Size = dxc_source->GetBufferSize();
        }
        else
        {
            GFX_SNPRINTF(shader_file, sizeof(shader_file), "%s%s", program.file_path_.c_str(), shader_extensions_[shader_type]);
            mbstowcs(wshader_file, shader_file, ARRAYSIZE(shader_file));
            switch(shader_type)
            {
            case kShaderType_CS:
                shader_source.Ptr = program.cs_.c_str();
                break;
            case kShaderType_VS:
                shader_source.Ptr = program.vs_.c_str();
                break;
            case kShaderType_GS:
                shader_source.Ptr = program.gs_.c_str();
                break;
            case kShaderType_PS:
                shader_source.Ptr = program.ps_.c_str();
                break;
            default:
                GFX_ASSERTMSG(0, "An unsupported shader type `%u' was supplied", (uint32_t)shader_type);
                return;
            }
            shader_source.Size = strlen((char const *)shader_source.Ptr);
            if(shader_source.Size == 0) return; // no source found for this shader
        }

        char shader_profiles[][16] =
        {
            "cs_",
            "vs_",
            "gs_",
            "ps_"
        };
        char const *shader_model = (raytracing_support_ ? "6_5" : "6_2");
        static_assert(ARRAYSIZE(shader_profiles) == kShaderType_Count, "An invalid number of shader profiles was supplied");
        for(uint32_t i = 0; i < ARRAYSIZE(shader_profiles); ++i) strcpy(shader_profiles[i] + strlen(shader_profiles[i]), shader_model);

        WCHAR wentry_point[2048], wshader_profile[16];
        mbstowcs(wentry_point, kernel.entry_point_.c_str(), ARRAYSIZE(wentry_point));
        mbstowcs(wshader_profile, shader_profiles[shader_type], ARRAYSIZE(wshader_profile));

        std::vector<LPCWSTR> shader_args;
        shader_args.push_back(wshader_file);
        shader_args.push_back(L"-E"); shader_args.push_back(wentry_point);
        shader_args.push_back(L"-I"); shader_args.push_back(L".");
        shader_args.push_back(L"-T"); shader_args.push_back(wshader_profile);

        std::vector<std::wstring> user_defines;
        if(!kernel.defines_.empty())
        {
            size_t max_define_length = 0;
            for(size_t i = 0; i < kernel.defines_.size(); ++i)
                max_define_length = GFX_MAX(max_define_length, strlen(kernel.defines_[i].c_str()));
            WCHAR *wdefine = (WCHAR *)alloca((max_define_length + 1) << 1);
            for(size_t i = 0; i < kernel.defines_.size(); ++i)
            {
                mbstowcs(wdefine, kernel.defines_[i].c_str(), max_define_length + 1);
                user_defines.push_back(wdefine);
            }
            for(size_t i = 0; i < user_defines.size(); ++i)
            {
                shader_args.push_back(L"-D");
                shader_args.push_back(user_defines[i].c_str());
            }
        }

        IDxcResult *dxc_result = nullptr;
        dxc_compiler_->Compile(&shader_source, shader_args.data(), (uint32_t)shader_args.size(), dxc_include_handler_, IID_PPV_ARGS(&dxc_result));
        if(dxc_source) dxc_source->Release();
        if(!dxc_result) return; // should never happen?

        HRESULT result_code = E_FAIL;
        dxc_result->GetStatus(&result_code);
        if(FAILED(result_code))
        {
            IDxcBlobEncoding *dxc_error = nullptr;
            dxc_result->GetErrorBuffer(&dxc_error);
            GFX_PRINTLN("Error: Failed to compile `%s' for entry point `%s'%s%s", shader_file, kernel.entry_point_.c_str(), dxc_error ? ":\r\n" : "", dxc_error ? (char const *)dxc_error->GetBufferPointer() : "");
            if(dxc_error) dxc_error->Release(); dxc_result->Release();
            return;
        }

        IDxcBlob *dxc_bytecode = nullptr;
        IDxcBlob *dxc_reflection = nullptr;
        dxc_result->GetResult(&dxc_bytecode);
        dxc_result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&dxc_reflection), nullptr);
        dxc_result->Release();
        if(!dxc_bytecode || !dxc_reflection)
        {
            if(dxc_bytecode) dxc_bytecode->Release();
            if(dxc_reflection) dxc_reflection->Release();
            return; // should never happen?
        }

        DxcBuffer reflection_data = {};
        reflection_data.Size = dxc_reflection->GetBufferSize();
        reflection_data.Ptr = dxc_reflection->GetBufferPointer();
        dxc_utils_->CreateReflection(&reflection_data, IID_PPV_ARGS(&shader_reflection));
        if(shader_reflection) shader_bytecode = dxc_bytecode;
        if(!shader_reflection) dxc_bytecode->Release();
        dxc_reflection->Release();
    }

    GfxResult createResource(D3D12MA::ALLOCATION_DESC const &allocation_desc, D3D12_RESOURCE_DESC const &resource_desc,
        D3D12_RESOURCE_STATES initial_resource_state, D3D12MA::Allocation **allocation, REFIID riid_resource, void** ppv_resource)
    {
        HRESULT result;
        D3D12_CLEAR_VALUE
        clear_value          = {};
        clear_value.Format   = resource_desc.Format;
        if(IsDepthStencilFormat(resource_desc.Format))
        {
            clear_value.DepthStencil.Depth   = 1.0f;
            clear_value.DepthStencil.Stencil = 0;
        }
        else
        {
            clear_value.Color[0] = 0.0f;
            clear_value.Color[1] = 0.0f;
            clear_value.Color[2] = 0.0f;
            clear_value.Color[3] = 1.0f;
        }
        result = mem_allocator_->CreateResource(&allocation_desc, &resource_desc, initial_resource_state,
            (resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 || (resource_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 ? &clear_value : nullptr,
            allocation, riid_resource, ppv_resource);
        if(!SUCCEEDED(result))
        {
            *allocation = nullptr;  // D3D12MemoryAllocator leaves a dangling pointer to the allocation object upon failure
            return GFX_SET_ERROR(kGfxResult_OutOfMemory, "Unable to allocate memory to create resource");
        }
        return kGfxResult_NoError;
    }

    void transitionResource(Buffer &buffer, D3D12_RESOURCE_STATES resource_state)
    {
        GFX_ASSERT(buffer.data_ == nullptr); if(buffer.data_ != nullptr) return;
        if(*buffer.resource_state_ == resource_state)
        {
            if(resource_state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            {
                D3D12_RESOURCE_BARRIER resource_barrier = {};
                resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                resource_barrier.UAV.pResource = buffer.resource_;
                resource_barriers_.push_back(resource_barrier);
            }
            return; // no need to transition
        }
        D3D12_RESOURCE_BARRIER resource_barrier = {};
        resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Transition.pResource = buffer.resource_;
        resource_barrier.Transition.StateBefore = *buffer.resource_state_;
        resource_barrier.Transition.StateAfter = resource_state;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        resource_barriers_.push_back(resource_barrier);
        *buffer.resource_state_ = resource_state;
    }

    void transitionResource(Texture &texture, D3D12_RESOURCE_STATES resource_state)
    {
        if(texture.resource_state_ == resource_state)
        {
            if(resource_state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            {
                D3D12_RESOURCE_BARRIER resource_barrier = {};
                resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                resource_barrier.UAV.pResource = texture.resource_;
                resource_barriers_.push_back(resource_barrier);
            }
            return; // no need to transition
        }
        D3D12_RESOURCE_BARRIER resource_barrier = {};
        resource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Transition.pResource = texture.resource_;
        resource_barrier.Transition.StateBefore = texture.resource_state_;
        resource_barrier.Transition.StateAfter = resource_state;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        resource_barriers_.push_back(resource_barrier);
        texture.resource_state_ = resource_state;
    }

    void submitPipelineBarriers()
    {
        if(resource_barriers_.empty()) return;  // no pending barriers
        command_list_->ResourceBarrier((uint32_t)resource_barriers_.size(),
                                       resource_barriers_.data());
        resource_barriers_.clear();
    }

    GfxResult acquireSwapChainBuffers()
    {
        for(uint32_t i = 0; i < ARRAYSIZE(back_buffers_); ++i)
        {
            char buffer[256];
            GFX_SNPRINTF(buffer, sizeof(buffer), "gfx_BackBuffer%u", i);
            if(!SUCCEEDED(swap_chain_->GetBuffer(i, IID_PPV_ARGS(&back_buffers_[i]))))
                return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to acquire back buffer");
            SetDebugName(back_buffers_[i], buffer);
        }

        for(uint32_t i = 0; i < ARRAYSIZE(back_buffer_rtvs_); ++i)
        {
            back_buffer_rtvs_[i] = allocateRTVDescriptor();
            if(back_buffer_rtvs_[i] == 0xFFFFFFFFu)
                return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to allocate RTV descriptors");
            device_->CreateRenderTargetView(back_buffers_[i], nullptr, rtv_descriptors_.getCPUHandle(back_buffer_rtvs_[i]));
        }

        return kGfxResult_NoError;
    }

    GfxResult sync()
    {
        for(uint32_t i = 0; i < kGfxConstant_BackBufferCount; ++i)
        {
            command_queue_->Signal(fences_[i], ++fence_values_[i]);
            if(fences_[i]->GetCompletedValue() < fence_values_[i])
            {
                fences_[i]->SetEventOnCompletion(fence_values_[i], fence_event_);
                WaitForSingleObject(fence_event_, INFINITE);    // wait for GPU to complete
            }
        }
        return forceGarbageCollection();
    }

    GfxResult resizeCallback(uint32_t window_width, uint32_t window_height)
    {
        if(!IsWindow(window_)) return kGfxResult_NoError;   // can't resize past window tear down
        for(uint32_t i = 0; i < textures_.size(); ++i)
        {
            Texture &texture = textures_.data()[i];
            if(!((texture.flags_ & Texture::kFlag_AutoResize) != 0))
                continue;   // no need to auto-resize
            ID3D12Resource *resource = nullptr;
            D3D12MA::Allocation *allocation = nullptr;
            D3D12_RESOURCE_DESC
            resource_desc        = texture.resource_->GetDesc();
            resource_desc.Width  = window_width;
            resource_desc.Height = window_height;
            D3D12MA::ALLOCATION_DESC allocation_desc = {};
            allocation_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            if(createResource(allocation_desc, resource_desc, D3D12_RESOURCE_STATE_COPY_DEST, &allocation, IID_PPV_ARGS(&resource)) != kGfxResult_NoError)
            {
                GFX_PRINT_ERROR(kGfxResult_OutOfMemory, "Unable to auto-resize texture object(s) to %ux%u", window_width, window_height);
                break;  // out of memory
            }
            collect(texture);   // release previous texture
            texture.resource_ = resource;
            texture.allocation_ = allocation;
            texture.Object::flags_ &= ~Object::kFlag_Named;
            for(uint32_t j = 0; j < ARRAYSIZE(texture.dsv_descriptor_slots_); ++j)
            {
                texture.dsv_descriptor_slots_[j].resize(resource_desc.DepthOrArraySize);
                for(size_t k = 0; k < texture.dsv_descriptor_slots_[j].size(); ++k)
                    texture.dsv_descriptor_slots_[j][k] = 0xFFFFFFFFu;
            }
            for(uint32_t j = 0; j < ARRAYSIZE(texture.rtv_descriptor_slots_); ++j)
            {
                texture.rtv_descriptor_slots_[j].resize(resource_desc.DepthOrArraySize);
                for(size_t k = 0; k < texture.rtv_descriptor_slots_[j].size(); ++k)
                    texture.rtv_descriptor_slots_[j][k] = 0xFFFFFFFFu;
            }
            texture.resource_state_ = D3D12_RESOURCE_STATE_COPY_DEST;
        }
        for(uint32_t i = 0; i < ARRAYSIZE(back_buffers_); ++i) { collect(back_buffers_[i]); back_buffers_[i] = nullptr; }
        for(uint32_t i = 0; i < ARRAYSIZE(back_buffer_rtvs_); ++i) { freeRTVDescriptor(back_buffer_rtvs_[i]); back_buffer_rtvs_[i] = 0xFFFFFFFFu; }
        sync(); // make sure the GPU is done with the previous swap chain before resizing
        HRESULT const hr = swap_chain_->ResizeBuffers(kGfxConstant_BackBufferCount, window_width, window_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        fence_index_ = swap_chain_->GetCurrentBackBufferIndex();
        GFX_TRY(acquireSwapChainBuffers());
        if(!SUCCEEDED(hr))
            return GFX_SET_ERROR(kGfxResult_InternalError, "Unable to resize the swap chain buffers");
        window_width_ = window_width;
        window_height_ = window_height;
        for(uint32_t i = 0; i < kernels_.size(); ++i)
            kernels_.data()[i].descriptor_heap_id_ = 0;
        return kGfxResult_NoError;
    }
};

#pragma warning(pop)

char const *GfxInternal::shader_extensions_[] =
{
    ".comp",
    ".vert",
    ".geom",
    ".frag"
};

GfxArray<GfxInternal::DrawState> GfxInternal::draw_states_;
GfxHandles                       GfxInternal::draw_state_handles_("draw state");

GfxContext gfxCreateContext(HWND window, GfxCreateContextFlags flags)
{
    GfxResult result;
    GfxContext context = {};
    if(!window) return context; // invalid param
    GfxInternal *gfx = new GfxInternal(context);
    if(!gfx) return context;    // out of memory
    result = gfx->initialize(window, context, flags);
    if(result != kGfxResult_NoError)
    {
        delete gfx;
        context = {};
        GFX_PRINT_ERROR(result, "Failed to initialize graphics context");
    }
    return context;
}

GfxResult gfxDestroyContext(GfxContext context)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_NoError; // nothing to destroy
    GFX_TRY(gfx->finish()); delete gfx;
    return kGfxResult_NoError;
}

uint32_t gfxGetBackBufferWidth(GfxContext context)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return 0;  // invalid context
    return gfx->getBackBufferWidth();
}

uint32_t gfxGetBackBufferHeight(GfxContext context)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return 0;  // invalid context
    return gfx->getBackBufferHeight();
}

uint32_t gfxGetBackBufferIndex(GfxContext context)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return 0;  // invalid context
    return gfx->getBackBufferIndex();
}

GfxBuffer gfxCreateBuffer(GfxContext context, uint64_t size, void const *data, GfxCpuAccess cpu_access)
{
    GfxBuffer const buffer = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return buffer; // invalid context
    return gfx->createBuffer(size, data, cpu_access);
}

GfxBuffer gfxCreateBufferRange(GfxContext context, GfxBuffer buffer, uint64_t byte_offset, uint64_t size)
{
    GfxBuffer const buffer_range = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return buffer_range;   // invalid context
    return gfx->createBufferRange(buffer, byte_offset, size);
}

GfxResult gfxDestroyBuffer(GfxContext context, GfxBuffer buffer)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->destroyBuffer(buffer);
}

void *gfxBufferGetData(GfxContext context, GfxBuffer buffer)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return nullptr;    // invalid context
    return gfx->getBufferData(buffer);
}

GfxTexture gfxCreateTexture2D(GfxContext context, DXGI_FORMAT format)
{
    GfxTexture const texture = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return texture;    // invalid context
    return gfx->createTexture2D(format);
}

GfxTexture gfxCreateTexture2D(GfxContext context, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t mip_levels)
{
    GfxTexture const texture = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return texture;    // invalid context
    return gfx->createTexture2D(width, height, format, mip_levels);
}

GfxTexture gfxCreateTexture2DArray(GfxContext context, uint32_t width, uint32_t height, uint32_t slice_count, DXGI_FORMAT format, uint32_t mip_levels)
{
    GfxTexture const texture = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return texture;    // invalid context
    return gfx->createTexture2DArray(width, height, slice_count, format, mip_levels);
}

GfxTexture gfxCreateTexture3D(GfxContext context, uint32_t width, uint32_t height, uint32_t depth, DXGI_FORMAT format, uint32_t mip_levels)
{
    GfxTexture const texture = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return texture;    // invalid context
    return gfx->createTexture3D(width, height, depth, format, mip_levels);
}

GfxTexture gfxCreateTextureCube(GfxContext context, uint32_t size, DXGI_FORMAT format, uint32_t mip_levels)
{
    GfxTexture const texture = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return texture;    // invalid context
    return gfx->createTextureCube(size, format, mip_levels);
}

GfxResult gfxDestroyTexture(GfxContext context, GfxTexture texture)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->destroyTexture(texture);
}

GfxSamplerState gfxCreateSamplerState(GfxContext context, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address_u, D3D12_TEXTURE_ADDRESS_MODE address_v, D3D12_TEXTURE_ADDRESS_MODE address_w, float mip_lod_bias, float min_lod, float max_lod)
{
    GfxSamplerState const sampler_state = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return sampler_state;  // invalid context
    return gfx->createSamplerState(filter, address_u, address_v, address_w, mip_lod_bias, min_lod, max_lod);
}

GfxSamplerState gfxCreateSamplerState(GfxContext context, D3D12_FILTER filter, D3D12_COMPARISON_FUNC comparison_func, D3D12_TEXTURE_ADDRESS_MODE address_u, D3D12_TEXTURE_ADDRESS_MODE address_v, D3D12_TEXTURE_ADDRESS_MODE address_w, float mip_lod_bias, float min_lod, float max_lod)
{
    GfxSamplerState const sampler_state = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return sampler_state;  // invalid context
    return gfx->createSamplerState(filter, comparison_func, address_u, address_v, address_w, mip_lod_bias, min_lod, max_lod);
}

GfxResult gfxDestroySamplerState(GfxContext context, GfxSamplerState sampler_state)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->destroySamplerState(sampler_state);
}

GfxAccelerationStructure gfxCreateAccelerationStructure(GfxContext context)
{
    GfxAccelerationStructure const acceleration_structure = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return acceleration_structure; // invalid context
    return gfx->createAccelerationStructure();
}

GfxResult gfxDestroyAccelerationStructure(GfxContext context, GfxAccelerationStructure acceleration_structure)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->destroyAccelerationStructure(acceleration_structure);
}

GfxResult gfxAccelerationStructureUpdate(GfxContext context, GfxAccelerationStructure acceleration_structure)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->updateAccelerationStructure(acceleration_structure);
}

uint32_t gfxAccelerationStructureGetRaytracingPrimitiveCount(GfxContext context, GfxAccelerationStructure acceleration_structure)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return 0;  // invalid context
    return gfx->getAccelerationStructureRaytracingPrimitiveCount(acceleration_structure);
}

GfxRaytracingPrimitive const *gfxAccelerationStructureGetRaytracingPrimitives(GfxContext context, GfxAccelerationStructure acceleration_structure)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return nullptr;    // invalid context
    return gfx->getAccelerationStructureRaytracingPrimitives(acceleration_structure);
}

GfxRaytracingPrimitive gfxCreateRaytracingPrimitive(GfxContext context, GfxAccelerationStructure acceleration_structure)
{
    GfxRaytracingPrimitive const raytracing_primitive = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return raytracing_primitive;   // invalid context
    return gfx->createRaytracingPrimitive(acceleration_structure);
}

GfxResult gfxDestroyRaytracingPrimitive(GfxContext context, GfxRaytracingPrimitive raytracing_primitive)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->destroyRaytracingPrimitive(raytracing_primitive);
}

GfxResult gfxRaytracingPrimitiveBuild(GfxContext context, GfxRaytracingPrimitive raytracing_primitive, GfxBuffer vertex_buffer, uint32_t vertex_stride)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->buildRaytracingPrimitive(raytracing_primitive, vertex_buffer, vertex_stride);
}

GfxResult gfxRaytracingPrimitiveBuild(GfxContext context, GfxRaytracingPrimitive raytracing_primitive, GfxBuffer index_buffer, GfxBuffer vertex_buffer, uint32_t vertex_stride)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->buildRaytracingPrimitive(raytracing_primitive, index_buffer, vertex_buffer, vertex_stride);
}

GfxResult gfxRaytracingPrimitiveSetTransform(GfxContext context, GfxRaytracingPrimitive raytracing_primitive, float const *row_major_4x4_transform)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->setRaytracingPrimitiveTransform(raytracing_primitive, row_major_4x4_transform);
}

GfxResult gfxRaytracingPrimitiveUpdate(GfxContext context, GfxRaytracingPrimitive raytracing_primitive)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->updateRaytracingPrimitive(raytracing_primitive);
}

GfxDrawState::GfxDrawState()
{
    GfxInternal::DispenseDrawState(*this);
}

GfxDrawState::GfxDrawState(GfxDrawState const &other)
    : handle(other.handle)
{
    GfxInternal::RetainDrawState(*this);
}

GfxDrawState &GfxDrawState::operator =(GfxDrawState const &other)
{
    GfxInternal::ReleaseDrawState(*this);
    handle = other.handle;
    GfxInternal::RetainDrawState(*this);
    return *this;
}

GfxDrawState::~GfxDrawState()
{
    GfxInternal::ReleaseDrawState(*this);
}

GfxResult gfxDrawStateSetColorTarget(GfxDrawState draw_state, uint32_t target_index, GfxTexture texture, uint32_t mip_level, uint32_t slice)
{
    return GfxInternal::SetDrawStateColorTarget(draw_state, target_index, texture, mip_level, slice);
}

GfxResult gfxDrawStateSetDepthStencilTarget(GfxDrawState draw_state, GfxTexture texture, uint32_t mip_level, uint32_t slice)
{
    return GfxInternal::SetDrawStateDepthStencilTarget(draw_state, texture, mip_level, slice);
}

GfxResult gfxDrawStateSetCullMode(GfxDrawState draw_state, D3D12_CULL_MODE cull_mode)
{
    return GfxInternal::SetDrawStateCullMode(draw_state, cull_mode);
}

GfxResult gfxDrawStateSetFillMode(GfxDrawState draw_state, D3D12_FILL_MODE fill_mode)
{
    return GfxInternal::SetDrawStateFillMode(draw_state, fill_mode);
}

GfxResult gfxDrawStateSetBlendMode(GfxDrawState draw_state, D3D12_BLEND src_blend, D3D12_BLEND dst_blend, D3D12_BLEND_OP blend_op, D3D12_BLEND src_blend_alpha, D3D12_BLEND dst_blend_alpha, D3D12_BLEND_OP blend_op_alpha)
{
    return GfxInternal::SetDrawStateBlendMode(draw_state, src_blend, dst_blend, blend_op, src_blend_alpha, dst_blend_alpha, blend_op_alpha);
}

GfxProgram gfxCreateProgram(GfxContext context, char const *file_name, char const *file_path)
{
    GfxProgram const program = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return program;    // invalid context
    return gfx->createProgram(file_name, file_path);
}

GfxProgram gfxCreateProgram(GfxContext context, GfxProgramDesc program_desc, char const *name)
{
    GfxProgram const program = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return program;    // invalid context
    return gfx->createProgram(program_desc, name);
}

GfxResult gfxDestroyProgram(GfxContext context, GfxProgram program)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->destroyProgram(program);
}

GfxResult gfxProgramSetBuffer(GfxContext context, GfxProgram program, char const *parameter_name, GfxBuffer buffer)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->setProgramBuffer(program, parameter_name, buffer);
}

GfxResult gfxProgramSetTexture(GfxContext context, GfxProgram program, char const *parameter_name, GfxTexture texture, uint32_t mip_level)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->setProgramTexture(program, parameter_name, texture, mip_level);
}

GfxResult gfxProgramSetTextures(GfxContext context, GfxProgram program, char const *parameter_name, GfxTexture const *textures, uint32_t texture_count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->setProgramTextures(program, parameter_name, textures, texture_count);
}

GfxResult gfxProgramSetSamplerState(GfxContext context, GfxProgram program, char const *parameter_name, GfxSamplerState sampler_state)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->setProgramSamplerState(program, parameter_name, sampler_state);
}

GfxResult gfxProgramSetAccelerationStructure(GfxContext context, GfxProgram program, char const *parameter_name, GfxAccelerationStructure acceleration_structure)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->setProgramAccelerationStructure(program, parameter_name, acceleration_structure);
}

GfxResult gfxProgramSetConstants(GfxContext context, GfxProgram program, char const *parameter_name, void const *data, uint32_t data_size)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->setProgramConstants(program, parameter_name, data, data_size);
}

GfxKernel gfxCreateComputeKernel(GfxContext context, GfxProgram program, char const *entry_point, char const **defines, uint32_t define_count)
{
    GfxKernel const compute_kernel = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return compute_kernel; // invalid context
    return gfx->createComputeKernel(program, entry_point, defines, define_count);
}

GfxKernel gfxCreateGraphicsKernel(GfxContext context, GfxProgram program, char const *entry_point, char const **defines, uint32_t define_count)
{
    GfxDrawState const default_draw_state = {};
    return gfxCreateGraphicsKernel(context, program, default_draw_state, entry_point, defines, define_count);
}

GfxKernel gfxCreateGraphicsKernel(GfxContext context, GfxProgram program, GfxDrawState draw_state, char const *entry_point, char const **defines, uint32_t define_count)
{
    GfxKernel const graphics_kernel = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return graphics_kernel;    // invalid context
    return gfx->createGraphicsKernel(program, draw_state, entry_point, defines, define_count);
}

GfxResult gfxDestroyKernel(GfxContext context, GfxKernel kernel)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->destroyKernel(kernel);
}

uint32_t const *gfxKernelGetNumThreads(GfxContext context, GfxKernel kernel)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return nullptr;    // invalid context
    return gfx->getKernelNumThreads(kernel);
}

GfxResult gfxKernelReloadAll(GfxContext context)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->reloadAllKernels();
}

GfxResult gfxCommandCopyBuffer(GfxContext context, GfxBuffer dst, GfxBuffer src)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeCopyBuffer(dst, src);
}

GfxResult gfxCommandCopyBuffer(GfxContext context, GfxBuffer dst, uint64_t dst_offset, GfxBuffer src, uint64_t src_offset, uint64_t size)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeCopyBuffer(dst, dst_offset, src, src_offset, size);
}

GfxResult gfxCommandClearBuffer(GfxContext context, GfxBuffer buffer, uint32_t clear_value)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeClearBuffer(buffer, clear_value);
}

GfxResult gfxCommandClearBackBuffer(GfxContext context)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeClearBackBuffer();
}

GfxResult gfxCommandClearTexture(GfxContext context, GfxTexture texture)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeClearTexture(texture);
}

GfxResult gfxCommandCopyTexture(GfxContext context, GfxTexture dst, GfxTexture src)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeCopyTexture(dst, src);
}

GfxResult gfxCommandClearImage(GfxContext context, GfxTexture texture, uint32_t mip_level, uint32_t slice)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeClearImage(texture, mip_level, slice);
}

GfxResult gfxCommandCopyBufferToTexture(GfxContext context, GfxTexture dst, GfxBuffer src)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeCopyBufferToTexture(dst, src);
}

GfxResult gfxCommandGenerateMips(GfxContext context, GfxTexture texture)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeGenerateMips(texture);
}

GfxResult gfxCommandBindKernel(GfxContext context, GfxKernel kernel)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeBindKernel(kernel);
}

GfxResult gfxCommandBindIndexBuffer(GfxContext context, GfxBuffer index_buffer)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeBindIndexBuffer(index_buffer);
}

GfxResult gfxCommandBindVertexBuffer(GfxContext context, GfxBuffer vertex_buffer)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeBindVertexBuffer(vertex_buffer);
}

GfxResult gfxCommandSetViewport(GfxContext context, float x, float y, float width, float height)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeSetViewport(x, y, width, height);
}

GfxResult gfxCommandSetScissorRect(GfxContext context, int32_t x, int32_t y, int32_t width, int32_t height)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeSetScissorRect(x, y, width, height);
}

GfxResult gfxCommandDraw(GfxContext context, uint32_t vertex_count, uint32_t instance_count, uint32_t base_vertex, uint32_t base_instance)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeDraw(vertex_count, instance_count, base_vertex, base_instance);
}

GfxResult gfxCommandDrawIndexed(GfxContext context, uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t base_vertex, uint32_t base_instance)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeDrawIndexed(index_count, instance_count, first_index, base_vertex, base_instance);
}

GfxResult gfxCommandMultiDrawIndirect(GfxContext context, GfxBuffer args_buffer, uint32_t args_count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeMultiDrawIndirect(args_buffer, args_count);
}

GfxResult gfxCommandMultiDrawIndexedIndirect(GfxContext context, GfxBuffer args_buffer, uint32_t args_count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeMultiDrawIndexedIndirect(args_buffer, args_count);
}

GfxResult gfxCommandDispatch(GfxContext context, uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeDispatch(num_groups_x, num_groups_y, num_groups_z);
}

GfxResult gfxCommandDispatchIndirect(GfxContext context, GfxBuffer args_buffer)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeDispatchIndirect(args_buffer);
}

GfxTimestampQuery gfxCreateTimestampQuery(GfxContext context)
{
    GfxTimestampQuery const timestamp_query = {};
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return timestamp_query;    // invalid context
    return gfx->createTimestampQuery();
}

GfxResult gfxDestroyTimestampQuery(GfxContext context, GfxTimestampQuery timestamp_query)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->destroyTimestampQuery(timestamp_query);
}

float gfxTimestampQueryGetDuration(GfxContext context, GfxTimestampQuery timestamp_query)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return 0.0f;   // invalid context
    return gfx->getTimestampQueryDuration(timestamp_query);
}

GfxResult gfxCommandBeginTimestampQuery(GfxContext context, GfxTimestampQuery timestamp_query)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeBeginTimestampQuery(timestamp_query);
}

GfxResult gfxCommandEndTimestampQuery(GfxContext context, GfxTimestampQuery timestamp_query)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeEndTimestampQuery(timestamp_query);
}

GfxResult gfxCommandBeginEvent(GfxContext context, char const *format, ...)
{
    va_list args;
    GfxResult result;
    va_start(args, format);
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) result = kGfxResult_InvalidParameter;
        else result = gfx->encodeBeginEvent(0, format, args);
    va_end(args);   // release variadic arguments
    return result;
}

GfxResult gfxCommandBeginEvent(GfxContext context, uint64_t color, char const *format, ...)
{
    va_list args;
    GfxResult result;
    va_start(args, format);
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) result = kGfxResult_InvalidParameter;
        else result = gfx->encodeBeginEvent(color, format, args);
    va_end(args);   // release variadic arguments
    return result;
}

GfxResult gfxCommandEndEvent(GfxContext context)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeEndEvent();
}

GfxResult gfxCommandScanMin(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeScan(GfxInternal::kOpType_Min, data_type, dst, src, count);
}

GfxResult gfxCommandScanMax(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeScan(GfxInternal::kOpType_Max, data_type, dst, src, count);
}

GfxResult gfxCommandScanSum(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeScan(GfxInternal::kOpType_Sum, data_type, dst, src, count);
}

GfxResult gfxCommandReduceMin(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeReduce(GfxInternal::kOpType_Min, data_type, dst, src, count);
}

GfxResult gfxCommandReduceMax(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeReduce(GfxInternal::kOpType_Max, data_type, dst, src, count);
}

GfxResult gfxCommandReduceSum(GfxContext context, GfxDataType data_type, GfxBuffer dst, GfxBuffer src, GfxBuffer const *count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeReduce(GfxInternal::kOpType_Sum, data_type, dst, src, count);
}

GfxResult gfxCommandSortRadix(GfxContext context, GfxBuffer keys_dst, GfxBuffer keys_src, GfxBuffer const *values_dst, GfxBuffer const *values_src, GfxBuffer const *count)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->encodeRadixSort(keys_dst, keys_src, values_dst, values_src, count);
}

GfxResult gfxFrame(GfxContext context, bool vsync)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->frame(vsync);
}

GfxResult gfxFinish(GfxContext context)
{
    GfxInternal *gfx = GfxInternal::GetGfx(context);
    if(!gfx) return kGfxResult_InvalidParameter;
    return gfx->finish();
}

#endif //! GFX_IMPLEMENTATION_DEFINE
