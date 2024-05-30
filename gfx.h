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
#ifndef GFX_INCLUDE_GFX_H
#define GFX_INCLUDE_GFX_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include "gfx_core.h"

//!
//! Context creation/destruction.
//!

class GfxContext { friend class GfxInternal; uint64_t handle; char name[kGfxConstant_MaxNameLength + 1]; uint32_t vendor_id; public:
                   inline bool operator ==(GfxContext const &other) const { return handle == other.handle; }
                   inline bool operator !=(GfxContext const &other) const { return handle != other.handle; }
                   inline GfxContext() { memset(this, 0, sizeof(*this)); }
                   inline uint32_t getVendorId() const { return vendor_id; }
                   inline char const *getName() const { return name; }
                   inline operator bool() const { return !!handle; } };

enum GfxCreateContextFlag
{
    kGfxCreateContextFlag_EnableDebugLayer       = 1 << 0,
    kGfxCreateContextFlag_EnableShaderDebugging  = 1 << 1,
    kGfxCreateContextFlag_EnableStablePowerState = 1 << 2
};
typedef uint32_t GfxCreateContextFlags;

GfxContext gfxCreateContext(HWND window, GfxCreateContextFlags flags = 0, IDXGIAdapter *adapter = nullptr);
GfxContext gfxCreateContext(uint32_t width, uint32_t height, GfxCreateContextFlags flags, IDXGIAdapter *adapter = nullptr);
GfxResult gfxDestroyContext(GfxContext const &context);

uint32_t gfxGetBackBufferWidth(GfxContext const &context);
uint32_t gfxGetBackBufferHeight(GfxContext const &context);
uint32_t gfxGetBackBufferIndex(GfxContext const &context);
uint32_t gfxGetBackBufferCount(GfxContext const &context);

bool gfxIsRaytracingSupported(GfxContext const &context);
bool gfxIsMeshShaderSupported(GfxContext const &context);
bool gfxIsInteropContext(GfxContext const &context);

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

GfxBuffer gfxCreateBuffer(GfxContext const &context, uint64_t size, void const *data = nullptr, GfxCpuAccess cpu_access = kGfxCpuAccess_None);
GfxBuffer gfxCreateBufferRange(GfxContext const &context, GfxBuffer const &buffer, uint64_t byte_offset, uint64_t size = 0); // fast path for (sub-)allocating during a frame
GfxResult gfxDestroyBuffer(GfxContext const &context, GfxBuffer const &buffer);

void *gfxBufferGetData(GfxContext const &context, GfxBuffer const &buffer);

//!
//! Template helpers.
//!

template<typename TYPE>
GfxBuffer gfxCreateBuffer(GfxContext const &context, uint32_t element_count, void const *data = nullptr, GfxCpuAccess cpu_access = kGfxCpuAccess_None)
{
    GfxBuffer buffer = gfxCreateBuffer(context, element_count * sizeof(TYPE), data, cpu_access);
    if(buffer) buffer.setStride((uint32_t)sizeof(TYPE));
    return buffer;
}

template<typename TYPE>
GfxBuffer gfxCreateBufferRange(GfxContext const &context, GfxBuffer const &buffer, uint32_t element_offset, uint32_t element_count = 0)
{
    GfxBuffer buffer_range = gfxCreateBufferRange(context, buffer, element_offset * sizeof(TYPE), element_count * sizeof(TYPE));
    if(buffer_range) buffer_range.setStride((uint32_t)sizeof(TYPE));
    return buffer_range;
}

template<typename TYPE>
TYPE *gfxBufferGetData(GfxContext const &context, GfxBuffer const &buffer)
{
    return (TYPE *)gfxBufferGetData(context, buffer);
}

//!
//! Texture resources.
//!

class GfxTexture { GFX_INTERNAL_NAMED_HANDLE(GfxTexture); uint32_t width; uint32_t height; uint32_t depth; DXGI_FORMAT format; uint32_t mip_levels; enum { kType_2D, kType_2DArray, kType_3D, kType_Cube } type; float clear_value_[4]; public:
                   inline bool is2DArray() const { return type == kType_2DArray; }
                   inline bool isCube() const { return type == kType_Cube; }
                   inline bool is3D() const { return type == kType_3D; }
                   inline bool is2D() const { return type == kType_2D; }
                   inline uint32_t getWidth() const { return width; }
                   inline uint32_t getHeight() const { return height; }
                   inline uint32_t getDepth() const { return depth; }
                   inline DXGI_FORMAT getFormat() const { return format; }
                   inline uint32_t getMipLevels() const { return mip_levels; }
                   inline float const *getClearValue() const { return clear_value_; } };

GfxTexture gfxCreateTexture2D(GfxContext const &context, DXGI_FORMAT format, float const *clear_value = nullptr);  // creates auto-resize window-sized texture
GfxTexture gfxCreateTexture2D(GfxContext const &context, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t mip_levels = 1, float const *clear_value = nullptr);
GfxTexture gfxCreateTexture2DArray(GfxContext const &context, uint32_t width, uint32_t height, uint32_t slice_count, DXGI_FORMAT format, uint32_t mip_levels = 1, float const *clear_value = nullptr);
GfxTexture gfxCreateTexture3D(GfxContext const &context, uint32_t width, uint32_t height, uint32_t depth, DXGI_FORMAT format, uint32_t mip_levels = 1, float const *clear_value = nullptr);
GfxTexture gfxCreateTextureCube(GfxContext const &context, uint32_t size, DXGI_FORMAT format, uint32_t mip_levels = 1, float const *clear_value = nullptr);
GfxResult gfxDestroyTexture(GfxContext const &context, GfxTexture const &texture);

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

GfxSamplerState gfxCreateSamplerState(GfxContext const &context, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE address_u = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_v = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_w = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               float mip_lod_bias = 0.0f, float min_lod = 0.0f, float max_lod = (float)D3D12_REQ_MIP_LEVELS);
GfxSamplerState gfxCreateSamplerState(GfxContext const &context, D3D12_FILTER filter, D3D12_COMPARISON_FUNC comparison_func,   // for comparison samplers
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_u = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_v = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               D3D12_TEXTURE_ADDRESS_MODE address_w = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                                               float mip_lod_bias = 0.0f, float min_lod = 0.0f, float max_lod = (float)D3D12_REQ_MIP_LEVELS);
GfxResult gfxDestroySamplerState(GfxContext const &context, GfxSamplerState sampler_state);

//!
//! Acceleration structures.
//!

class GfxRaytracingPrimitive;   // forward declaration

class GfxAccelerationStructure { GFX_INTERNAL_NAMED_HANDLE(GfxAccelerationStructure); public: };

GfxAccelerationStructure gfxCreateAccelerationStructure(GfxContext const &context);
GfxResult gfxDestroyAccelerationStructure(GfxContext const &context, GfxAccelerationStructure const &acceleration_structure); // automatically releases all raytracing primitives

GfxResult gfxAccelerationStructureUpdate(GfxContext const &context, GfxAccelerationStructure const &acceleration_structure);
uint64_t gfxAccelerationStructureGetDataSize(GfxContext const &context, GfxAccelerationStructure const &acceleration_structure);  // in bytes
uint32_t gfxAccelerationStructureGetRaytracingPrimitiveCount(GfxContext const &context, GfxAccelerationStructure const &acceleration_structure);
GfxRaytracingPrimitive const *gfxAccelerationStructureGetRaytracingPrimitives(GfxContext const &context, GfxAccelerationStructure const &acceleration_structure);

//!
//! Raytracing primitives.
//!

enum GfxBuildRaytracingPrimitiveFlag
{
    kGfxBuildRaytracingPrimitiveFlag_Opaque = 1 << 0
};
typedef uint32_t GfxBuildRaytracingPrimitiveFlags;

class GfxRaytracingPrimitive { GFX_INTERNAL_NAMED_HANDLE(GfxRaytracingPrimitive); public: };

GfxRaytracingPrimitive gfxCreateRaytracingPrimitive(GfxContext const &context, GfxAccelerationStructure const &acceleration_structure);
GfxRaytracingPrimitive gfxCreateRaytracingPrimitive(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive);   // instantiates the raytracing primitive
GfxResult gfxDestroyRaytracingPrimitive(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive);

GfxResult gfxRaytracingPrimitiveBuild(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive, GfxBuffer const &vertex_buffer, uint32_t vertex_stride = 0, GfxBuildRaytracingPrimitiveFlags build_flags = 0);
GfxResult gfxRaytracingPrimitiveBuild(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive, GfxBuffer const &index_buffer, GfxBuffer const &vertex_buffer, uint32_t vertex_stride = 0, GfxBuildRaytracingPrimitiveFlags build_flags = 0);
GfxResult gfxRaytracingPrimitiveSetTransform(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive, float const *row_major_4x4_transform);
GfxResult gfxRaytracingPrimitiveSetInstanceID(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive, uint32_t instance_id);   // retrieved through `ray_query.CommittedInstanceID()`
GfxResult gfxRaytracingPrimitiveSetInstanceMask(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive, uint8_t instance_mask);
GfxResult gfxRaytracingPrimitiveSetInstanceContributionToHitGroupIndex(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive, uint32_t instance_contribution_to_hit_group_index);
uint64_t gfxRaytracingPrimitiveGetDataSize(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive);    // in bytes
GfxResult gfxRaytracingPrimitiveUpdate(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive);
GfxResult gfxRaytracingPrimitiveUpdate(GfxContext const &context, GfxRaytracingPrimitive const &raytracing_primitive, GfxBuffer const &index_buffer, GfxBuffer const &vertex_buffer, uint32_t vertex_stride = 0);

//!
//! Draw state manipulation.
//!

class GfxDrawState { friend class GfxInternal; uint64_t handle; public: GfxDrawState(); GfxDrawState(GfxDrawState const &); GfxDrawState &operator =(GfxDrawState const &); ~GfxDrawState(); };

GfxResult gfxDrawStateSetColorTarget(GfxDrawState draw_state, uint32_t target_index, DXGI_FORMAT texture_format);
GfxResult gfxDrawStateSetDepthStencilTarget(GfxDrawState draw_state, DXGI_FORMAT texture_format);

GfxResult gfxDrawStateSetCullMode(GfxDrawState draw_state, D3D12_CULL_MODE cull_mode);
GfxResult gfxDrawStateSetFillMode(GfxDrawState draw_state, D3D12_FILL_MODE fill_mode);
GfxResult gfxDrawStateSetDepthFunction(GfxDrawState draw_state, D3D12_COMPARISON_FUNC depth_function);
GfxResult gfxDrawStateSetDepthWriteMask(GfxDrawState draw_state, D3D12_DEPTH_WRITE_MASK depth_write_mask);
GfxResult gfxDrawStateSetPrimitiveTopologyType(GfxDrawState draw_state, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topology_type);
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

class GfxProgramDesc { public: inline GfxProgramDesc() : cs(nullptr), as(nullptr), ms(nullptr), vs(nullptr), gs(nullptr), ps(nullptr), lib(nullptr) {}
                       char const *cs;
                       char const *as;
                       char const *ms;
                       char const *vs;
                       char const *gs;
                       char const *ps;
                       char const *lib; };

GfxProgram gfxCreateProgram(GfxContext const &context, char const *file_name, char const *file_path = nullptr, char const *shader_model = nullptr, char const **include_paths = nullptr, uint32_t include_path_count = 0);
GfxProgram gfxCreateProgram(GfxContext const &context, GfxProgramDesc program_desc, char const *name = nullptr, char const *shader_model = nullptr, char const **include_paths = nullptr, uint32_t include_path_count = 0);
GfxResult gfxDestroyProgram(GfxContext const &context, GfxProgram const &program);

GfxResult gfxProgramSetBuffer(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxBuffer const &buffer);
GfxResult gfxProgramSetBuffers(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxBuffer const *buffers, uint32_t buffer_count);
GfxResult gfxProgramSetTexture(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxTexture const &texture, uint32_t mip_level = 0);
GfxResult gfxProgramSetTextures(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxTexture const *textures, uint32_t texture_count);
GfxResult gfxProgramSetTextures(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxTexture const *textures, uint32_t const *mip_levels, uint32_t texture_count);
GfxResult gfxProgramSetSamplerState(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxSamplerState sampler_state);
GfxResult gfxProgramSetAccelerationStructure(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxAccelerationStructure const &acceleration_structure);
GfxResult gfxProgramSetConstants(GfxContext const &context, GfxProgram const &program, char const *parameter_name, void const *data, uint32_t data_size);

//!
//! Template helpers.
//!

template<typename TYPE>
inline GfxResult gfxProgramSetParameter(GfxContext const &context, GfxProgram const &program, char const *parameter_name, TYPE const &value);
inline GfxResult gfxProgramSetParameter(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxBuffer const *buffers, uint32_t buffer_count) { return gfxProgramSetBuffers(context, program, parameter_name, buffers, buffer_count); }
inline GfxResult gfxProgramSetParameter(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxTexture const &texture, uint32_t mip_level) { return gfxProgramSetTexture(context, program, parameter_name, texture, mip_level); }
inline GfxResult gfxProgramSetParameter(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxTexture const *textures, uint32_t texture_count) { return gfxProgramSetTextures(context, program, parameter_name, textures, texture_count); }
inline GfxResult gfxProgramSetParameter(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxTexture const *textures, uint32_t const *mip_levels, uint32_t texture_count) { return gfxProgramSetTextures(context, program, parameter_name, textures, mip_levels, texture_count); }

template<> inline GfxResult gfxProgramSetParameter<GfxBuffer>(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxBuffer const &value) { return gfxProgramSetBuffer(context, program, parameter_name, value); }
template<> inline GfxResult gfxProgramSetParameter<GfxTexture>(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxTexture const &value) { return gfxProgramSetTexture(context, program, parameter_name, value); }
template<> inline GfxResult gfxProgramSetParameter<GfxSamplerState>(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxSamplerState const &value) { return gfxProgramSetSamplerState(context, program, parameter_name, value); }
template<> inline GfxResult gfxProgramSetParameter<GfxAccelerationStructure>(GfxContext const &context, GfxProgram const &program, char const *parameter_name, GfxAccelerationStructure const &value) { return gfxProgramSetAccelerationStructure(context, program, parameter_name, value); }
template<typename TYPE> inline GfxResult gfxProgramSetParameter(GfxContext const &context, GfxProgram const &program, char const *parameter_name, TYPE const &value)
{
    static_assert(!std::is_pointer<TYPE>::value, "Program parameters must be passed by value, not by pointer");
    return gfxProgramSetConstants(context, program, parameter_name, &value, sizeof(value));
}

//!
//! Kernel compilation.
//!

class GfxKernel { GFX_INTERNAL_HANDLE(GfxKernel); char name[kGfxConstant_MaxNameLength + 1]; enum { kType_Mesh, kType_Compute, kType_Graphics, kType_Raytracing } type; public:
                  inline bool isMesh() const { return type == kType_Mesh; }
                  inline bool isCompute() const { return type == kType_Compute; }
                  inline bool isGraphics() const { return type == kType_Graphics; }
                  inline bool isRaytracing() const { return type == kType_Raytracing; }
                  inline char const *getName() const { return name; } };

enum GfxShaderGroupType
{
    kGfxShaderGroupType_Raygen = 0,
    kGfxShaderGroupType_Miss,
    kGfxShaderGroupType_Hit,
    kGfxShaderGroupType_Callable,

    kGfxShaderGroupType_Count
};

struct GfxLocalRootSignatureAssociation
{
    uint32_t local_root_signature_space = 0;
    GfxShaderGroupType shader_group_type = kGfxShaderGroupType_Count;
    char const *shader_group_name = nullptr;
};

GfxKernel gfxCreateMeshKernel(GfxContext const &context, GfxProgram const &program, char const *entry_point = nullptr, char const **defines = nullptr, uint32_t define_count = 0);    // draws to back buffer
GfxKernel gfxCreateMeshKernel(GfxContext const &context, GfxProgram const &program, GfxDrawState draw_state, char const *entry_point = nullptr, char const **defines = nullptr, uint32_t define_count = 0);
GfxKernel gfxCreateComputeKernel(GfxContext const &context, GfxProgram const &program, char const *entry_point = nullptr, char const **defines = nullptr, uint32_t define_count = 0);
GfxKernel gfxCreateGraphicsKernel(GfxContext const &context, GfxProgram const &program, char const *entry_point = nullptr, char const **defines = nullptr, uint32_t define_count = 0);    // draws to back buffer
GfxKernel gfxCreateGraphicsKernel(GfxContext const &context, GfxProgram const &program, GfxDrawState draw_state, char const *entry_point = nullptr, char const **defines = nullptr, uint32_t define_count = 0);
GfxKernel gfxCreateRaytracingKernel(GfxContext const &context, GfxProgram const &program,
    GfxLocalRootSignatureAssociation const *local_root_signature_associations = nullptr, uint32_t local_root_signature_association_count = 0,
    char const **exports = nullptr, uint32_t export_count = 0,
    char const **subobjects = nullptr, uint32_t subobject_count = 0,
    char const **defines = nullptr, uint32_t define_count = 0);
GfxResult gfxDestroyKernel(GfxContext const &context, GfxKernel const &kernel);

uint32_t const *gfxKernelGetNumThreads(GfxContext const &context, GfxKernel const &kernel);
GfxResult gfxKernelReloadAll(GfxContext const &context);   // hot-reloads all kernel objects

//!
//! Shader binding tables.
//!

class GfxSbt { GFX_INTERNAL_HANDLE(GfxSbt); public: };

GfxSbt gfxCreateSbt(GfxContext const &context, GfxKernel const *kernels, uint32_t kernel_count, uint32_t entry_count[kGfxShaderGroupType_Count]);
GfxResult gfxDestroySbt(GfxContext const &context, GfxSbt sbt);

GfxResult gfxSbtSetShaderGroup(GfxContext const &context, GfxSbt sbt, GfxShaderGroupType shader_group_type, uint32_t index, char const *group_name);
GfxResult gfxSbtSetConstants(GfxContext const &context, GfxSbt sbt, GfxShaderGroupType shader_group_type, uint32_t index, char const *parameter_name, void const *data, uint32_t data_size);
GfxResult gfxSbtSetTexture(GfxContext const &context, GfxSbt sbt, GfxShaderGroupType shader_group_type, uint32_t index, char const *parameter_name, GfxTexture const &texture, uint32_t mip_level = 0);
GfxResult gfxSbtGetGpuVirtualAddressRangeAndStride(GfxContext const &context,
    GfxSbt sbt,
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE *ray_generation_shader_record,
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE *miss_shader_table,
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE *hit_group_table,
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE *callable_shader_table);

//!
//! Command encoding.
//!

GfxResult gfxCommandCopyBuffer(GfxContext const &context, GfxBuffer const &dst, GfxBuffer const &src);
GfxResult gfxCommandCopyBuffer(GfxContext const &context, GfxBuffer const &dst, uint64_t dst_offset, GfxBuffer const &src, uint64_t src_offset, uint64_t size);
GfxResult gfxCommandClearBuffer(GfxContext const &context, GfxBuffer const &buffer, uint32_t clear_value = 0);

GfxResult gfxCommandClearBackBuffer(GfxContext const &context);    // clears to (0.0, 0.0, 0.0, 1.0)
GfxResult gfxCommandClearTexture(GfxContext const &context, GfxTexture const &texture);
GfxResult gfxCommandCopyTexture(GfxContext const &context, GfxTexture const &dst, GfxTexture const &src);
GfxResult gfxCommandClearImage(GfxContext const &context, GfxTexture const &texture, uint32_t mip_level = 0, uint32_t slice = 0);

GfxResult gfxCommandCopyTextureToBackBuffer(GfxContext const &context, GfxTexture const &texture);
GfxResult gfxCommandCopyBufferToTexture(GfxContext const &context, GfxTexture const &dst, GfxBuffer const &src);
GfxResult gfxCommandCopyBufferToCubeFace(GfxContext const &context, GfxTexture const &dst, GfxBuffer const &src, uint32_t face);
GfxResult gfxCommandCopyTextureToBuffer(GfxContext const &context, GfxBuffer const &dst, GfxTexture const &src);
GfxResult gfxCommandGenerateMips(GfxContext const &context, GfxTexture const &texture);   // expects mip level 0 to be populated, generates the others

GfxResult gfxCommandBindColorTarget(GfxContext const &context, uint32_t target_index, GfxTexture const &target_texture, uint32_t mip_level = 0, uint32_t slice = 0);
GfxResult gfxCommandBindDepthStencilTarget(GfxContext const &context, GfxTexture const &target_texture, uint32_t mip_level = 0, uint32_t slice = 0);
GfxResult gfxCommandBindKernel(GfxContext const &context, GfxKernel const &kernel);
GfxResult gfxCommandBindIndexBuffer(GfxContext const &context, GfxBuffer const &index_buffer);
GfxResult gfxCommandBindVertexBuffer(GfxContext const &context, GfxBuffer const &vertex_buffer);

GfxResult gfxCommandSetViewport(GfxContext const &context, float x = 0.0f, float y = 0.0f, float width = 0.0f, float height = 0.0f);   // leave at 0 to draw to render target/back buffer size
GfxResult gfxCommandSetScissorRect(GfxContext const &context, int32_t x = 0, int32_t y = 0, int32_t width = 0, int32_t height = 0);    // leave at 0 to draw to render target/back buffer size

GfxResult gfxCommandDraw(GfxContext const &context, uint32_t vertex_count, uint32_t instance_count = 1, uint32_t base_vertex = 0, uint32_t base_instance = 0);
GfxResult gfxCommandDrawIndexed(GfxContext const &context, uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, uint32_t base_vertex = 0, uint32_t base_instance = 0);
GfxResult gfxCommandMultiDrawIndirect(GfxContext const &context, GfxBuffer const &args_buffer, uint32_t args_count);          // expects a buffer of D3D12_DRAW_ARGUMENTS elements
GfxResult gfxCommandMultiDrawIndexedIndirect(GfxContext const &context, GfxBuffer const &args_buffer, uint32_t args_count);   // expects a buffer of D3D12_DRAW_INDEXED_ARGUMENTS elements
GfxResult gfxCommandDispatch(GfxContext const &context, uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z);
GfxResult gfxCommandDispatchIndirect(GfxContext const &context, GfxBuffer const &args_buffer);                                // expects a buffer of D3D12_DISPATCH_ARGUMENTS elements
GfxResult gfxCommandMultiDispatchIndirect(GfxContext const &context, GfxBuffer const &args_buffer, uint32_t args_count);      // expects a buffer of D3D12_DISPATCH_ARGUMENTS elements
GfxResult gfxCommandDispatchRays(GfxContext const &context, GfxSbt sbt, uint32_t width, uint32_t height, uint32_t depth);
GfxResult gfxCommandDispatchRaysIndirect(GfxContext const &context, GfxSbt sbt, GfxBuffer const &args_buffer);                // expects a buffer of D3D12_DISPATCH_RAYS_DESC elements
GfxResult gfxCommandDrawMesh(GfxContext const &context, uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z);
GfxResult gfxCommandDrawMeshIndirect(GfxContext const &context, GfxBuffer const &args_buffer);                                // expects a buffer of D3D12_DISPATCH_MESH_ARGUMENTS elements

//!
//! Debug/profile API.
//!

class GfxTimestampQuery { GFX_INTERNAL_HANDLE(GfxTimestampQuery); public: };

GfxTimestampQuery gfxCreateTimestampQuery(GfxContext const &context);
GfxResult gfxDestroyTimestampQuery(GfxContext const &context, GfxTimestampQuery timestamp_query);
float gfxTimestampQueryGetDuration(GfxContext const &context, GfxTimestampQuery timestamp_query);  // in milliseconds
GfxResult gfxCommandBeginTimestampQuery(GfxContext const &context, GfxTimestampQuery timestamp_query);
GfxResult gfxCommandEndTimestampQuery(GfxContext const &context, GfxTimestampQuery timestamp_query);

GfxResult gfxCommandBeginEvent(GfxContext const &context, char const *format, ...);
GfxResult gfxCommandBeginEvent(GfxContext const &context, uint64_t color, char const *format, ...);
GfxResult gfxCommandEndEvent(GfxContext const &context);

//!
//! RAII helpers.
//!

class GfxTimedSection { GFX_NON_COPYABLE(GfxTimedSection); GfxContext const &context; GfxTimestampQuery timestamp_query; char const *section_name; public:
                        inline GfxTimedSection(GfxContext const &context, GfxTimestampQuery const &timestamp_query, char const *section_name = nullptr) : context(context), timestamp_query(timestamp_query), section_name(section_name) { gfxCommandBeginTimestampQuery(context, timestamp_query); }
                        inline ~GfxTimedSection() { gfxCommandEndTimestampQuery(context, timestamp_query); if(section_name != nullptr) GFX_PRINTLN("Completed `%s' in %.3fms", section_name, gfxTimestampQueryGetDuration(context, timestamp_query)); } };

class GfxCommandEvent { GFX_NON_COPYABLE(GfxCommandEvent); GfxContext const &context; public:
                        inline GfxCommandEvent(GfxContext const &context, char const *format, ...) : context(context) { va_list args; va_start(args, format); char buffer[4096]; vsnprintf(buffer, sizeof(buffer), format, args); va_end(args); gfxCommandBeginEvent(context, buffer); }
                        inline GfxCommandEvent(GfxContext const &context, uint64_t color, char const *format, ...) : context(context) { va_list args; va_start(args, format); char buffer[4096]; vsnprintf(buffer, sizeof(buffer), format, args); va_end(args); gfxCommandBeginEvent(context, color, buffer); }
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

GfxResult gfxCommandScanMin(GfxContext const &context, GfxDataType data_type, GfxBuffer const &dst, GfxBuffer const &src, GfxBuffer const *count = nullptr); // dst can be the same than src for all parallel primitives
GfxResult gfxCommandScanMax(GfxContext const &context, GfxDataType data_type, GfxBuffer const &dst, GfxBuffer const &src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandScanSum(GfxContext const &context, GfxDataType data_type, GfxBuffer const &dst, GfxBuffer const &src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandReduceMin(GfxContext const &context, GfxDataType data_type, GfxBuffer const &dst, GfxBuffer const &src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandReduceMax(GfxContext const &context, GfxDataType data_type, GfxBuffer const &dst, GfxBuffer const &src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandReduceSum(GfxContext const &context, GfxDataType data_type, GfxBuffer const &dst, GfxBuffer const &src, GfxBuffer const *count = nullptr);
GfxResult gfxCommandSortRadix(GfxContext const &context, GfxBuffer const &keys_dst, GfxBuffer const &keys_src, GfxBuffer const *values_dst = nullptr, GfxBuffer const *values_src = nullptr, GfxBuffer const *count = nullptr);

//!
//! Frame processing.
//!

GfxResult gfxFrame(GfxContext const &context, bool vsync = true);
GfxResult gfxFinish(GfxContext const &context);

//!
//! Interop interface.
//!

GfxContext gfxCreateContext(ID3D12Device *device, uint32_t max_frames_in_flight = kGfxConstant_BackBufferCount);

ID3D12Device *gfxGetDevice(GfxContext const &context);
ID3D12CommandQueue *gfxGetCommandQueue(GfxContext const &context);
ID3D12GraphicsCommandList *gfxGetCommandList(GfxContext const &context);
GfxResult gfxSetCommandList(GfxContext const &context, ID3D12GraphicsCommandList *command_list);
GfxResult gfxResetCommandListState(GfxContext const &context); // call this function before returning to gfx after externally modifying the state on the command list

GfxBuffer gfxCreateBuffer(GfxContext const &context, ID3D12Resource *resource, D3D12_RESOURCE_STATES resource_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
GfxTexture gfxCreateTexture(GfxContext const &context, ID3D12Resource *resource, D3D12_RESOURCE_STATES resource_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
GfxAccelerationStructure gfxCreateAccelerationStructure(GfxContext const &context, ID3D12Resource *resource, uint64_t byte_offset = 0);    // resource must be in D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE state

ID3D12Resource *gfxBufferGetResource(GfxContext const &context, GfxBuffer const &buffer);
ID3D12Resource *gfxTextureGetResource(GfxContext const &context, GfxTexture const &texture);
ID3D12Resource *gfxAccelerationStructureGetResource(GfxContext const &context, GfxAccelerationStructure const &acceleration_structure);

D3D12_RESOURCE_STATES gfxBufferGetResourceState(GfxContext const &context, GfxBuffer const &buffer);
D3D12_RESOURCE_STATES gfxTextureGetResourceState(GfxContext const &context, GfxTexture const &texture);

HANDLE gfxBufferCreateSharedHandle(GfxContext const &context, GfxBuffer const &buffer);

//!
//! Template helpers.
//!

template<typename TYPE>
GfxBuffer gfxCreateBuffer(GfxContext const &context, ID3D12Resource *resource, D3D12_RESOURCE_STATES resource_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
{
    GfxBuffer buffer = gfxCreateBuffer(context, resource, resource_state);
    if(buffer) buffer.setStride((uint32_t)sizeof(TYPE));
    return buffer;
}

#endif //! GFX_INCLUDE_GFX_H

//!
//! Implementation details.
//!

#ifdef GFX_IMPLEMENTATION_DEFINE

#pragma once
#include "gfx.cpp"

#endif //! GFX_IMPLEMENTATION_DEFINE
