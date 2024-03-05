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
#if !defined(GFX_INCLUDE_GFX_SCENE_H) && defined(GFX_ENABLE_SCENE)
#define GFX_INCLUDE_GFX_SCENE_H

#include "gfx.h"
#include <glm/glm.hpp>

template<typename TYPE> class GfxRef;
template<typename TYPE> class GfxConstRef;

//!
//! Scene creation/destruction.
//!

class GfxScene { friend class GfxSceneInternal; uint64_t handle; public: inline GfxScene() : handle(0) {}
                 inline bool operator ==(GfxScene const &other) const { return handle == other.handle; }
                 inline bool operator !=(GfxScene const &other) const { return handle != other.handle; }
                 inline operator bool() const { return !!handle; } };

GfxScene gfxCreateScene();
GfxResult gfxDestroyScene(GfxScene scene);

GfxResult gfxSceneImport(GfxScene scene, char const *asset_file);
GfxResult gfxSceneClear(GfxScene scene);

//!
//! Object access and iteration.
//!

template<typename TYPE> TYPE const *gfxSceneGetObjects(GfxScene scene);
template<typename TYPE> uint32_t gfxSceneGetObjectCount(GfxScene scene);
template<typename TYPE> TYPE *gfxSceneGetObject(GfxScene scene, uint64_t object_handle);
template<typename TYPE> GfxRef<TYPE> gfxSceneGetObjectHandle(GfxScene scene, uint32_t object_index);

//!
//! Ref. primitives.
//!

template<typename TYPE, typename OBJECT_TYPE>
class GfxRefBase { friend class GfxSceneInternal; protected: uint64_t handle; GfxScene scene; public: GfxRefBase() : handle(0), scene{} {}
                   bool operator ==(GfxRefBase const &other) const { return handle == other.handle && scene == other.scene; }
                   bool operator !=(GfxRefBase const &other) const { return handle != other.handle || scene != other.scene; }
                   uint32_t getIndex() const { return (uint32_t)(handle ? handle & 0xFFFFFFFFull : 0xFFFFFFFFull); }
                   OBJECT_TYPE *operator ->() const { return gfxSceneGetObject<TYPE>(scene, handle); }
                   OBJECT_TYPE &operator *() const { return *operator ->(); }
                   operator bool() const { return !!operator ->(); }
                   operator uint32_t() const { return getIndex(); }
                   operator uint64_t() const { return handle; }
                   bool operator<(GfxRefBase const &other) const { return handle < other.handle; } };

template<typename TYPE>
class GfxRef : public GfxRefBase<TYPE, TYPE> { friend class GfxConstRef<TYPE>; };

template<typename TYPE>
class GfxConstRef : public GfxRefBase<TYPE, TYPE const> { public: GfxConstRef() {} GfxConstRef(GfxRef<TYPE> const &other) { this->handle = other.handle; this->scene = other.scene; }
                                                          operator GfxRef<TYPE>() const { GfxRef<TYPE> ref; ref.handle = this->handle; ref.scene = this->scene; return ref; } };

//!
//! Metadata API.
//!

class GfxMetadata { friend class GfxSceneInternal; bool is_valid; public: std::string asset_file; std::string object_name; GfxMetadata() : is_valid(false) {}
                    inline char const *getAssetFile() const { return asset_file.c_str(); }
                    inline char const *getObjectName() const { return object_name.c_str(); }
                    inline bool getIsValid() const { return is_valid; }
                    inline operator bool() const { return is_valid; } };

template<typename TYPE> GfxMetadata const &gfxSceneGetObjectMetadata(GfxScene scene, uint64_t object_handle);
template<typename TYPE> bool gfxSceneSetObjectMetadata(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata);

//!
//! Search API.
//!

template<typename TYPE>
GfxRef<TYPE> gfxSceneFindObjectByAssetFile(GfxScene scene, char const *asset_file)
{
    GfxRef<TYPE> const object_ref = {};
    if(asset_file == nullptr || *asset_file == '\0')
        return object_ref;  // invalid parameter
    uint32_t const object_count = gfxSceneGetObjectCount<TYPE>(scene);
    for(uint32_t i = 0; i < object_count; ++i)
    {
        GfxRef<TYPE> const object_handle = gfxSceneGetObjectHandle<TYPE>(scene, i);
        if(!strcmp(gfxSceneGetObjectMetadata<TYPE>(scene, object_handle).getAssetFile(), asset_file))
            return object_handle;   // found
    }
    return object_ref;  // not found
}

//!
//! Animation object.
//!

struct GfxAnimation
{
    // Opaque type; use gfxSceneApplyAnimation() to animate the instances in the scene.
};

GfxRef<GfxAnimation> gfxSceneCreateAnimation(GfxScene scene);
GfxResult gfxSceneDestroyAnimation(GfxScene scene, uint64_t animation_handle);
GfxResult gfxSceneDestroyAllAnimations(GfxScene scene);

GfxResult gfxSceneApplyAnimation(GfxScene scene, uint64_t animation_handle, float time_in_seconds);
GfxResult gfxSceneResetAllAnimation(GfxScene scene);

float gfxSceneGetAnimationLength(GfxScene scene, uint64_t animation_handle);    // in secs

uint32_t gfxSceneGetAnimationCount(GfxScene scene);
GfxAnimation const *gfxSceneGetAnimations(GfxScene scene);
GfxAnimation *gfxSceneGetAnimation(GfxScene scene, uint64_t animation_handle);
GfxRef<GfxAnimation> gfxSceneGetAnimationHandle(GfxScene scene, uint32_t animation_index);
GfxMetadata const &gfxSceneGetAnimationMetadata(GfxScene scene, uint64_t animation_handle);
bool gfxSceneSetAnimationMetadata(GfxScene scene, uint64_t animation_handle, GfxMetadata const &metadata);

//!
//! Skin object.
//!

struct GfxSkin
{
    std::vector<glm::mat4> joint_matrices;  // contains a list of `inverse(meshGlobalTransform) * jointGlobalTransform * inverseBindMatrix' matrices for each joint
};

GfxRef<GfxSkin> gfxSceneCreateSkin(GfxScene scene);
GfxResult gfxSceneDestroySkin(GfxScene scene, uint64_t skin_handle);
GfxResult gfxSceneDestroyAllSkins(GfxScene scene);

uint32_t gfxSceneGetSkinCount(GfxScene scene);
GfxSkin const *gfxSceneGetSkins(GfxScene scene);
GfxSkin *gfxSceneGetSkin(GfxScene scene, uint64_t skin_handle);
GfxRef<GfxSkin> gfxSceneGetSkinHandle(GfxScene scene, uint32_t skin_index);
GfxMetadata const &gfxSceneGetSkinMetadata(GfxScene scene, uint64_t skin_handle);
bool gfxSceneSetSkinMetadata(GfxScene scene, uint64_t skin_handle, GfxMetadata const &metadata);

//!
//! Camera object.
//!

enum GfxCameraType
{
    kGfxCameraType_Perspective = 0,

    kGfxCameraType_Count
};

struct GfxCamera
{
    GfxCameraType type = kGfxCameraType_Perspective;

    glm::vec3 eye    = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up     = glm::vec3(0.0f, 1.0f, 0.0f);

    float aspect = 16.0f / 9.0f;
    float fovY   = 1.05f;
    float nearZ  = 0.1f;
    float farZ   = 1e4f;
};

GfxRef<GfxCamera> gfxSceneCreateCamera(GfxScene scene);
GfxResult gfxSceneDestroyCamera(GfxScene scene, uint64_t camera_handle);
GfxResult gfxSceneDestroyAllCameras(GfxScene scene);

GfxResult gfxSceneSetActiveCamera(GfxScene scene, uint64_t camera_handle);
GfxRef<GfxCamera> gfxSceneGetActiveCamera(GfxScene scene);

uint32_t gfxSceneGetCameraCount(GfxScene scene);
GfxCamera const *gfxSceneGetCameras(GfxScene scene);
GfxCamera *gfxSceneGetCamera(GfxScene scene, uint64_t camera_handle);
GfxRef<GfxCamera> gfxSceneGetCameraHandle(GfxScene scene, uint32_t camera_index);
GfxMetadata const &gfxSceneGetCameraMetadata(GfxScene scene, uint64_t camera_handle);
bool gfxSceneSetCameraMetadata(GfxScene scene, uint64_t camera_handle, GfxMetadata const &metadata);

//!
//! Light object.
//!

enum GfxLightType
{
    kGfxLightType_Point = 0,
    kGfxLightType_Spot = 1,
    kGfxLightType_Directional = 2,

    kGfxLightType_Count
};

struct GfxLight
{
    GfxLightType type = kGfxLightType_Point;

    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    float intensity = 1.0f;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f); //only valid for point+spot lights
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f); //only valid for directional+spot lights
    float range = FLT_MAX; //only valid for point+spot lights
    float inner_cone_angle = 0.0f; //only valid for spot lights
    float outer_cone_angle = 3.1415926535897932384626433832795f / 4.0f; //only valid for spot lights
};

GfxRef<GfxLight> gfxSceneCreateLight(GfxScene scene);
GfxResult gfxSceneDestroyLight(GfxScene scene, uint64_t light_handle);
GfxResult gfxSceneDestroyAllLights(GfxScene scene);

uint32_t gfxSceneGetLightCount(GfxScene scene);
GfxLight const *gfxSceneGetLights(GfxScene scene);
GfxLight *gfxSceneGetLight(GfxScene scene, uint64_t light_handle);
GfxRef<GfxLight> gfxSceneGetLightHandle(GfxScene scene, uint32_t light_index);
GfxMetadata const &gfxSceneGetLightMetadata(GfxScene scene, uint64_t light_handle);
bool gfxSceneSetLightMetadata(GfxScene scene, uint64_t light_handle, GfxMetadata const &metadata);

//!
//! Image object.
//!

enum GfxImageFlag
{
    kGfxImageFlag_HasAlphaChannel = 1 << 0,
    kGfxImageFlag_HasMipLevels    = 1 << 1
};
typedef uint32_t GfxImageFlags;

struct GfxImage
{
    uint32_t      width             = 0;
    uint32_t      height            = 0;
    uint32_t      channel_count     = 0;
    uint32_t      bytes_per_channel = 0;
    DXGI_FORMAT   format            = DXGI_FORMAT_UNKNOWN;
    GfxImageFlags flags             = 0;

    std::vector<uint8_t> data;
};

GfxRef<GfxImage> gfxSceneCreateImage(GfxScene scene);
GfxResult gfxSceneDestroyImage(GfxScene scene, uint64_t image_handle);
GfxResult gfxSceneDestroyAllImages(GfxScene scene);

uint32_t gfxSceneGetImageCount(GfxScene scene);
GfxImage const *gfxSceneGetImages(GfxScene scene);
GfxImage *gfxSceneGetImage(GfxScene scene, uint64_t image_handle);
GfxRef<GfxImage> gfxSceneGetImageHandle(GfxScene scene, uint32_t image_index);
GfxMetadata const &gfxSceneGetImageMetadata(GfxScene scene, uint64_t image_handle);
bool gfxSceneSetImageMetadata(GfxScene scene, uint64_t image_handle, GfxMetadata const &metadata);

//!
//! Image helpers.
//!

inline bool gfxImageIsFormatCompressed(GfxImage const& image)
{
    switch(image.format)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM: // fall through
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;
    default:
        break;
    }
    return false;
}

//!
//! Material object.
//!

enum GfxMaterialFlag
{
    kGfxMaterialFlag_DoubleSided = 1 << 0
};
typedef uint32_t GfxMaterialFlags;

struct GfxMaterial
{
    glm::vec4        albedo              = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    float            roughness           = 1.0f;
    float            metallicity         = 0.0f;
    glm::vec3        emissivity          = glm::vec3(0.0f, 0.0f, 0.0f);
    float            ior                 = 1.5f;
    glm::vec4        specular            = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); //.w=specular factor
    float            transmission        = 0.0f;
    glm::vec4        sheen               = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); //.w=sheen roughness
    float            clearcoat           = 0.0f;
    float            clearcoat_roughness = 0.0f;
    GfxMaterialFlags flags               = 0;

    GfxConstRef<GfxImage> albedo_map;
    GfxConstRef<GfxImage> roughness_map;
    GfxConstRef<GfxImage> metallicity_map;
    GfxConstRef<GfxImage> emissivity_map;
    GfxConstRef<GfxImage> specular_map;
    GfxConstRef<GfxImage> normal_map;
    GfxConstRef<GfxImage> transmission_map;
    GfxConstRef<GfxImage> sheen_map;
    GfxConstRef<GfxImage> clearcoat_map;
    GfxConstRef<GfxImage> clearcoat_roughness_map;
    GfxConstRef<GfxImage> ao_map;
};

GfxRef<GfxMaterial> gfxSceneCreateMaterial(GfxScene scene);
GfxResult gfxSceneDestroyMaterial(GfxScene scene, uint64_t material_handle);
GfxResult gfxSceneDestroyAllMaterials(GfxScene scene);

uint32_t gfxSceneGetMaterialCount(GfxScene scene);
GfxMaterial const *gfxSceneGetMaterials(GfxScene scene);
GfxMaterial *gfxSceneGetMaterial(GfxScene scene, uint64_t material_handle);
GfxRef<GfxMaterial> gfxSceneGetMaterialHandle(GfxScene scene, uint32_t material_index);
GfxMetadata const &gfxSceneGetMaterialMetadata(GfxScene scene, uint64_t material_handle);
bool gfxSceneSetMaterialMetadata(GfxScene scene, uint64_t material_handle, GfxMetadata const &metadata);

//!
//! Material helpers.
//!

inline bool gfxMaterialIsEmissive(GfxMaterial const &material)
{
    return glm::dot(material.emissivity, material.emissivity) > 0.0f || material.emissivity_map;
}

//!
//! Mesh object.
//!

struct GfxVertex
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal   = glm::vec3(0.0f);
    glm::vec2 uv       = glm::vec3(0.0f);
};

struct GfxJoint
{
    glm::uvec4 joints  = glm::uvec4(UINT_MAX);
    glm::vec4  weights = glm::vec4(0.0f);
};

struct GfxMesh
{
    glm::vec3 bounds_min = glm::vec3(0.0f);
    glm::vec3 bounds_max = glm::vec3(0.0f);

    std::vector<GfxVertex> vertices;
    std::vector<GfxVertex> morph_targets;
    std::vector<uint32_t>  indices;
    std::vector<GfxJoint>  joints;  // contains a list of per-vertex joint index and weight values for skinning on the GPU (see `GfxJoint' structure)
    std::vector<float>     default_weights;
};

GfxRef<GfxMesh> gfxSceneCreateMesh(GfxScene scene);
GfxResult gfxSceneDestroyMesh(GfxScene scene, uint64_t mesh_handle);
GfxResult gfxSceneDestroyAllMeshes(GfxScene scene);

uint32_t gfxSceneGetMeshCount(GfxScene scene);
GfxMesh const *gfxSceneGetMeshes(GfxScene scene);
GfxMesh *gfxSceneGetMesh(GfxScene scene, uint64_t mesh_handle);
GfxRef<GfxMesh> gfxSceneGetMeshHandle(GfxScene scene, uint32_t mesh_index);
GfxMetadata const &gfxSceneGetMeshMetadata(GfxScene scene, uint64_t mesh_handle);
bool gfxSceneSetMeshMetadata(GfxScene scene, uint64_t mesh_handle, GfxMetadata const &metadata);

//!
//! Instance object.
//!

struct GfxInstance
{
    GfxConstRef<GfxMesh>     mesh;
    GfxConstRef<GfxMaterial> material;
    GfxConstRef<GfxSkin>     skin;
    std::vector<float>       weights;

    glm::mat4 transform = glm::mat4(1.0f);
};

GfxRef<GfxInstance> gfxSceneCreateInstance(GfxScene scene);
GfxResult gfxSceneDestroyInstance(GfxScene scene, uint64_t instance_handle);
GfxResult gfxSceneDestroyAllInstances(GfxScene scene);

uint32_t gfxSceneGetInstanceCount(GfxScene scene);
GfxInstance const *gfxSceneGetInstances(GfxScene scene);
GfxInstance *gfxSceneGetInstance(GfxScene scene, uint64_t instance_handle);
GfxRef<GfxInstance> gfxSceneGetInstanceHandle(GfxScene scene, uint32_t instance_index);
GfxMetadata const &gfxSceneGetInstanceMetadata(GfxScene scene, uint64_t instance_handle);
bool gfxSceneSetInstanceMetadata(GfxScene scene, uint64_t instance_handle, GfxMetadata const &metadata);

//!
//! Template specializations.
//!

template<> inline uint32_t gfxSceneGetObjectCount<GfxAnimation>(GfxScene scene) { return gfxSceneGetAnimationCount(scene); }
template<> inline GfxAnimation const *gfxSceneGetObjects<GfxAnimation>(GfxScene scene) { return gfxSceneGetAnimations(scene); }
template<> inline GfxAnimation *gfxSceneGetObject<GfxAnimation>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetAnimation(scene, object_handle); }
template<> inline GfxRef<GfxAnimation> gfxSceneGetObjectHandle<GfxAnimation>(GfxScene scene, uint32_t object_index) { return gfxSceneGetAnimationHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxAnimation>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetAnimationMetadata(scene, object_handle); }
template<> inline bool gfxSceneSetObjectMetadata<GfxAnimation>(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata) { return gfxSceneSetAnimationMetadata(scene, object_handle, metadata); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxSkin>(GfxScene scene) { return gfxSceneGetSkinCount(scene); }
template<> inline GfxSkin const *gfxSceneGetObjects<GfxSkin>(GfxScene scene) { return gfxSceneGetSkins(scene); }
template<> inline GfxSkin *gfxSceneGetObject<GfxSkin>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetSkin(scene, object_handle); }
template<> inline GfxRef<GfxSkin> gfxSceneGetObjectHandle<GfxSkin>(GfxScene scene, uint32_t object_index) { return gfxSceneGetSkinHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxSkin>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetSkinMetadata(scene, object_handle); }
template<> inline bool gfxSceneSetObjectMetadata<GfxSkin>(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata) { return gfxSceneSetSkinMetadata(scene, object_handle, metadata); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxCamera>(GfxScene scene) { return gfxSceneGetCameraCount(scene); }
template<> inline GfxCamera const *gfxSceneGetObjects<GfxCamera>(GfxScene scene) { return gfxSceneGetCameras(scene); }
template<> inline GfxCamera *gfxSceneGetObject<GfxCamera>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetCamera(scene, object_handle); }
template<> inline GfxRef<GfxCamera> gfxSceneGetObjectHandle<GfxCamera>(GfxScene scene, uint32_t object_index) { return gfxSceneGetCameraHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxCamera>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetCameraMetadata(scene, object_handle); }
template<> inline bool gfxSceneSetObjectMetadata<GfxCamera>(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata) { return gfxSceneSetCameraMetadata(scene, object_handle, metadata); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxLight>(GfxScene scene) { return gfxSceneGetLightCount(scene); }
template<> inline GfxLight const *gfxSceneGetObjects<GfxLight>(GfxScene scene) { return gfxSceneGetLights(scene); }
template<> inline GfxLight *gfxSceneGetObject<GfxLight>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetLight(scene, object_handle); }
template<> inline GfxRef<GfxLight> gfxSceneGetObjectHandle<GfxLight>(GfxScene scene, uint32_t object_index) { return gfxSceneGetLightHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxLight>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetLightMetadata(scene, object_handle); }
template<> inline bool gfxSceneSetObjectMetadata<GfxLight>(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata) { return gfxSceneSetLightMetadata(scene, object_handle, metadata); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxImage>(GfxScene scene) { return gfxSceneGetImageCount(scene); }
template<> inline GfxImage const *gfxSceneGetObjects<GfxImage>(GfxScene scene) { return gfxSceneGetImages(scene); }
template<> inline GfxImage *gfxSceneGetObject<GfxImage>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetImage(scene, object_handle); }
template<> inline GfxRef<GfxImage> gfxSceneGetObjectHandle<GfxImage>(GfxScene scene, uint32_t object_index) { return gfxSceneGetImageHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxImage>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetImageMetadata(scene, object_handle); }
template<> inline bool gfxSceneSetObjectMetadata<GfxImage>(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata) { return gfxSceneSetImageMetadata(scene, object_handle, metadata); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxMaterial>(GfxScene scene) { return gfxSceneGetMaterialCount(scene); }
template<> inline GfxMaterial const *gfxSceneGetObjects<GfxMaterial>(GfxScene scene) { return gfxSceneGetMaterials(scene); }
template<> inline GfxMaterial *gfxSceneGetObject<GfxMaterial>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMaterial(scene, object_handle); }
template<> inline GfxRef<GfxMaterial> gfxSceneGetObjectHandle<GfxMaterial>(GfxScene scene, uint32_t object_index) { return gfxSceneGetMaterialHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxMaterial>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMaterialMetadata(scene, object_handle); }
template<> inline bool gfxSceneSetObjectMetadata<GfxMaterial>(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata) { return gfxSceneSetMaterialMetadata(scene, object_handle, metadata); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxMesh>(GfxScene scene) { return gfxSceneGetMeshCount(scene); }
template<> inline GfxMesh const *gfxSceneGetObjects<GfxMesh>(GfxScene scene) { return gfxSceneGetMeshes(scene); }
template<> inline GfxMesh *gfxSceneGetObject<GfxMesh>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMesh(scene, object_handle); }
template<> inline GfxRef<GfxMesh> gfxSceneGetObjectHandle<GfxMesh>(GfxScene scene, uint32_t object_index) { return gfxSceneGetMeshHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxMesh>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMeshMetadata(scene, object_handle); }
template<> inline bool gfxSceneSetObjectMetadata<GfxMesh>(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata) { return gfxSceneSetMeshMetadata(scene, object_handle, metadata); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxInstance>(GfxScene scene) { return gfxSceneGetInstanceCount(scene); }
template<> inline GfxInstance const *gfxSceneGetObjects<GfxInstance>(GfxScene scene) { return gfxSceneGetInstances(scene); }
template<> inline GfxInstance *gfxSceneGetObject<GfxInstance>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetInstance(scene, object_handle); }
template<> inline GfxRef<GfxInstance> gfxSceneGetObjectHandle<GfxInstance>(GfxScene scene, uint32_t object_index) { return gfxSceneGetInstanceHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxInstance>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetInstanceMetadata(scene, object_handle); }
template<> inline bool gfxSceneSetObjectMetadata<GfxInstance>(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata) { return gfxSceneSetInstanceMetadata(scene, object_handle, metadata); }

template<typename TYPE> uint32_t gfxSceneGetObjectCount(GfxScene scene) { static_assert(std::is_void_v<TYPE>, "Cannot get object count for unsupported object type"); }
template<typename TYPE> TYPE const *gfxSceneGetObjects(GfxScene scene) { static_assert(std::is_void_v<TYPE>, "Cannot get object list for unsupported object type"); }
template<typename TYPE> TYPE *gfxSceneGetObject(GfxScene scene, uint64_t object_handle) { static_assert(std::is_void_v<TYPE>, "Cannot get scene object for unsupported object type"); }
template<typename TYPE> GfxRef<TYPE> gfxSceneGetObjectHandle(GfxScene scene, uint32_t object_index) { static_assert(std::is_void_v<TYPE>, "Cannot get object handle for unsupported object type"); }
template<typename TYPE> GfxMetadata const &gfxSceneGetObjectMetadata(GfxScene scene, uint64_t object_handle) { static_assert(std::is_void_v<TYPE>, "Cannot get object metadata for unsupported object type"); }
template<typename TYPE> bool gfxSceneSetObjectMetadata(GfxScene scene, uint64_t object_handle, GfxMetadata const &metadata) { static_assert(std::is_void_v<TYPE>, "Cannot set object metadata for unsupported object type"); }

#endif //! GFX_INCLUDE_GFX_SCENE_H

//!
//! Implementation details.
//!

#ifdef GFX_IMPLEMENTATION_DEFINE

#pragma once
#include "gfx_scene.cpp"

#endif //! GFX_IMPLEMENTATION_DEFINE
