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
#include "glm/glm.hpp"

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
                   operator uint64_t() const { return handle; } };

template<typename TYPE>
class GfxRef : public GfxRefBase<TYPE, TYPE> { friend class GfxConstRef<TYPE>; };

template<typename TYPE>
class GfxConstRef : public GfxRefBase<TYPE, TYPE const> { public: GfxConstRef() {} GfxConstRef(GfxRef<TYPE> const &other) { handle = other.handle; scene = other.scene; }
                                                          operator GfxRef<TYPE>() const { GfxRef<TYPE> ref; ref.handle = handle; ref.scene = scene; return ref; } };

//!
//! Metadata API.
//!

class GfxMetadata { friend class GfxSceneInternal; bool is_valid; std::string asset_file; std::string object_name; public:
                    inline char const *getAssetFile() const { return asset_file.c_str(); }
                    inline char const *getObjectName() const { return object_name.c_str(); }
                    inline bool getIsValid() const { return is_valid; }
                    inline operator bool() const { return is_valid; } };

template<typename TYPE> GfxMetadata const &gfxSceneGetObjectMetadata(GfxScene scene, uint64_t object_handle);

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

GfxResult gfxSceneApplyAnimation(GfxScene scene, uint64_t animation_handle, float time_in_seconds);
GfxResult gfxSceneResetAllAnimation(GfxScene scene);

float gfxSceneGetAnimationLength(GfxScene scene, uint64_t animation_handle);    // in secs
float gfxSceneGetAnimationStart(GfxScene scene, uint64_t animation_handle);
float gfxSceneGetAnimationEnd(GfxScene scene, uint64_t animation_handle);

uint32_t gfxSceneGetAnimationCount(GfxScene scene);
GfxAnimation const *gfxSceneGetAnimations(GfxScene scene);
GfxAnimation *gfxSceneGetAnimation(GfxScene scene, uint64_t animation_handle);
GfxRef<GfxAnimation> gfxSceneGetAnimationHandle(GfxScene scene, uint32_t animation_index);
GfxMetadata const &gfxSceneGetAnimationMetadata(GfxScene scene, uint64_t animation_handle);

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

GfxResult gfxSceneSetActiveCamera(GfxScene scene, uint64_t camera_handle);
GfxConstRef<GfxCamera> gfxSceneGetActiveCamera(GfxScene scene);

uint32_t gfxSceneGetCameraCount(GfxScene scene);
GfxCamera const *gfxSceneGetCameras(GfxScene scene);
GfxCamera *gfxSceneGetCamera(GfxScene scene, uint64_t camera_handle);
GfxRef<GfxCamera> gfxSceneGetCameraHandle(GfxScene scene, uint32_t camera_index);
GfxMetadata const &gfxSceneGetCameraMetadata(GfxScene scene, uint64_t camera_handle);

//!
//! Image object.
//!

struct GfxImage
{
    uint32_t width             = 0;
    uint32_t height            = 0;
    uint32_t channel_count     = 0;
    uint32_t bytes_per_channel = 0;

    std::vector<uint8_t> data;
};

GfxRef<GfxImage> gfxSceneCreateImage(GfxScene scene);
GfxResult gfxSceneDestroyImage(GfxScene scene, uint64_t image_handle);

uint32_t gfxSceneGetImageCount(GfxScene scene);
GfxImage const *gfxSceneGetImages(GfxScene scene);
GfxImage *gfxSceneGetImage(GfxScene scene, uint64_t image_handle);
GfxRef<GfxImage> gfxSceneGetImageHandle(GfxScene scene, uint32_t image_index);
GfxMetadata const &gfxSceneGetImageMetadata(GfxScene scene, uint64_t image_handle);

//!
//! Image helpers.
//!

inline DXGI_FORMAT gfxImageGetFormat(GfxImage const &image)
{
    if(image.bytes_per_channel != 1 && image.bytes_per_channel != 2 && image.bytes_per_channel != 4) return DXGI_FORMAT_UNKNOWN;
    uint32_t const bits = (image.bytes_per_channel << 3);
    switch(image.channel_count)
    {
    case 1:
        return (bits == 8 ? DXGI_FORMAT_R8_UNORM       : bits == 16 ? DXGI_FORMAT_R16_UNORM          : DXGI_FORMAT_R32_FLOAT         );
    case 2:
        return (bits == 8 ? DXGI_FORMAT_R8G8_UNORM     : bits == 16 ? DXGI_FORMAT_R16G16_UNORM       : DXGI_FORMAT_R32G32_FLOAT      );
    case 4:
        return (bits == 8 ? DXGI_FORMAT_R8G8B8A8_UNORM : bits == 16 ? DXGI_FORMAT_R16G16B16A16_UNORM : DXGI_FORMAT_R32G32B32A32_FLOAT);
    default:
        break;
    }
    return DXGI_FORMAT_UNKNOWN;
}

//!
//! Material object.
//!

struct GfxMaterial
{
    glm::vec4 albedo      = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    float     roughness   = 1.0f;
    float     metallicity = 0.0f;
    glm::vec3 emissivity  = glm::vec3(0.0f, 0.0f, 0.0f);

    GfxConstRef<GfxImage> albedo_map;
    GfxConstRef<GfxImage> roughness_map;
    GfxConstRef<GfxImage> metallicity_map;
    GfxConstRef<GfxImage> emissivity_map;
    GfxConstRef<GfxImage> normal_map;
    GfxConstRef<GfxImage> ao_map;
};

GfxRef<GfxMaterial> gfxSceneCreateMaterial(GfxScene scene);
GfxResult gfxSceneDestroyMaterial(GfxScene scene, uint64_t material_handle);

uint32_t gfxSceneGetMaterialCount(GfxScene scene);
GfxMaterial const *gfxSceneGetMaterials(GfxScene scene);
GfxMaterial *gfxSceneGetMaterial(GfxScene scene, uint64_t material_handle);
GfxRef<GfxMaterial> gfxSceneGetMaterialHandle(GfxScene scene, uint32_t material_index);
GfxMetadata const &gfxSceneGetMaterialMetadata(GfxScene scene, uint64_t material_handle);

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

struct GfxMesh
{
    GfxConstRef<GfxMaterial> material;

    glm::vec3 bounds_min = glm::vec3(0.0f);
    glm::vec3 bounds_max = glm::vec3(0.0f);

    std::vector<GfxVertex> vertices;
    std::vector<uint32_t>  indices;
};

GfxRef<GfxMesh> gfxSceneCreateMesh(GfxScene scene);
GfxResult gfxSceneDestroyMesh(GfxScene scene, uint64_t mesh_handle);

uint32_t gfxSceneGetMeshCount(GfxScene scene);
GfxMesh const *gfxSceneGetMeshes(GfxScene scene);
GfxMesh *gfxSceneGetMesh(GfxScene scene, uint64_t mesh_handle);
GfxRef<GfxMesh> gfxSceneGetMeshHandle(GfxScene scene, uint32_t mesh_index);
GfxMetadata const &gfxSceneGetMeshMetadata(GfxScene scene, uint64_t mesh_handle);

//!
//! Instance object.
//!

struct GfxInstance
{
    GfxConstRef<GfxMesh> mesh;

    glm::mat4 transform = glm::mat4(1.0f);
};

GfxRef<GfxInstance> gfxSceneCreateInstance(GfxScene scene);
GfxResult gfxSceneDestroyInstance(GfxScene scene, uint64_t instance_handle);

uint32_t gfxSceneGetInstanceCount(GfxScene scene);
GfxInstance const *gfxSceneGetInstances(GfxScene scene);
GfxInstance *gfxSceneGetInstance(GfxScene scene, uint64_t instance_handle);
GfxRef<GfxInstance> gfxSceneGetInstanceHandle(GfxScene scene, uint32_t instance_index);
GfxMetadata const &gfxSceneGetInstanceMetadata(GfxScene scene, uint64_t instance_handle);

//!
//! Template specializations.
//!

template<> inline uint32_t gfxSceneGetObjectCount<GfxAnimation>(GfxScene scene) { return gfxSceneGetAnimationCount(scene); }
template<> inline GfxAnimation const *gfxSceneGetObjects<GfxAnimation>(GfxScene scene) { return gfxSceneGetAnimations(scene); }
template<> inline GfxAnimation *gfxSceneGetObject<GfxAnimation>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetAnimation(scene, object_handle); }
template<> inline GfxRef<GfxAnimation> gfxSceneGetObjectHandle<GfxAnimation>(GfxScene scene, uint32_t object_index) { return gfxSceneGetAnimationHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxAnimation>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetAnimationMetadata(scene, object_handle); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxCamera>(GfxScene scene) { return gfxSceneGetCameraCount(scene); }
template<> inline GfxCamera const *gfxSceneGetObjects<GfxCamera>(GfxScene scene) { return gfxSceneGetCameras(scene); }
template<> inline GfxCamera *gfxSceneGetObject<GfxCamera>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetCamera(scene, object_handle); }
template<> inline GfxRef<GfxCamera> gfxSceneGetObjectHandle<GfxCamera>(GfxScene scene, uint32_t object_index) { return gfxSceneGetCameraHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxCamera>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetCameraMetadata(scene, object_handle); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxImage>(GfxScene scene) { return gfxSceneGetImageCount(scene); }
template<> inline GfxImage const *gfxSceneGetObjects<GfxImage>(GfxScene scene) { return gfxSceneGetImages(scene); }
template<> inline GfxImage *gfxSceneGetObject<GfxImage>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetImage(scene, object_handle); }
template<> inline GfxRef<GfxImage> gfxSceneGetObjectHandle<GfxImage>(GfxScene scene, uint32_t object_index) { return gfxSceneGetImageHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxImage>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetImageMetadata(scene, object_handle); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxMaterial>(GfxScene scene) { return gfxSceneGetMaterialCount(scene); }
template<> inline GfxMaterial const *gfxSceneGetObjects<GfxMaterial>(GfxScene scene) { return gfxSceneGetMaterials(scene); }
template<> inline GfxMaterial *gfxSceneGetObject<GfxMaterial>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMaterial(scene, object_handle); }
template<> inline GfxRef<GfxMaterial> gfxSceneGetObjectHandle<GfxMaterial>(GfxScene scene, uint32_t object_index) { return gfxSceneGetMaterialHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxMaterial>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMaterialMetadata(scene, object_handle); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxMesh>(GfxScene scene) { return gfxSceneGetMeshCount(scene); }
template<> inline GfxMesh const *gfxSceneGetObjects<GfxMesh>(GfxScene scene) { return gfxSceneGetMeshes(scene); }
template<> inline GfxMesh *gfxSceneGetObject<GfxMesh>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMesh(scene, object_handle); }
template<> inline GfxRef<GfxMesh> gfxSceneGetObjectHandle<GfxMesh>(GfxScene scene, uint32_t object_index) { return gfxSceneGetMeshHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxMesh>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMeshMetadata(scene, object_handle); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxInstance>(GfxScene scene) { return gfxSceneGetInstanceCount(scene); }
template<> inline GfxInstance const *gfxSceneGetObjects<GfxInstance>(GfxScene scene) { return gfxSceneGetInstances(scene); }
template<> inline GfxInstance *gfxSceneGetObject<GfxInstance>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetInstance(scene, object_handle); }
template<> inline GfxRef<GfxInstance> gfxSceneGetObjectHandle<GfxInstance>(GfxScene scene, uint32_t object_index) { return gfxSceneGetInstanceHandle(scene, object_index); }
template<> inline GfxMetadata const &gfxSceneGetObjectMetadata<GfxInstance>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetInstanceMetadata(scene, object_handle); }

template<typename TYPE> uint32_t gfxSceneGetObjectCount(GfxScene scene) { static_assert("Cannot get object count for unsupported object type"); }
template<typename TYPE> TYPE const *gfxSceneGetObjects(GfxScene scene) { static_assert("Cannot get object list for unsupported object type"); }
template<typename TYPE> TYPE *gfxSceneGetObject(GfxScene scene, uint64_t object_handle) { static_assert("Cannot get scene object for unsupported object type"); }
template<typename TYPE> GfxRef<TYPE> gfxSceneGetObjectHandle(GfxScene scene, uint32_t object_index) { static_assert("Cannot get object handle for unsupported object type"); }
template<typename TYPE> GfxMetadata const &gfxSceneGetObjectMetadata(GfxScene scene, uint64_t object_handle) { static_assert("Cannot get object metadata for unsupported object type"); }

//!
//! Implementation details.
//!

#ifdef GFX_IMPLEMENTATION_DEFINE

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <map>                  // std::map
#include "tiny_gltf.h"          // glTF loader
#include "tiny_obj_loader.cc"   // obj loader
#include "glm/gtx/quaternion.hpp"

class GfxSceneInternal
{
    GFX_NON_COPYABLE(GfxSceneInternal);

    struct GltfNode
    {
        glm::dmat4 matrix_;

        glm::dvec3 translate_;
        glm::dquat rotate_;
        glm::dvec3 scale_;

        std::vector<uint64_t> children_;
        std::vector<GfxRef<GfxInstance>> instances_;
    };

    struct GltfAnimatedNode
    {
        glm::dvec3 translate_;
        glm::dquat rotate_;
        glm::dvec3 scale_;
    };

    enum GltfAnimationChannelMode
    {
        kGltfAnimationChannelMode_Linear = 0,
        kGltfAnimationChannelMode_Step,

        kGltfAnimationChannelMode_Count
    };

    enum GltfAnimationChannelType
    {
        kGltfAnimationChannelType_Translate = 0,
        kGltfAnimationChannelType_Rotate,
        kGltfAnimationChannelType_Scale,

        kGltfAnimationChannelType_Count
    };

    struct GltfAnimationChannel
    {
        uint64_t node_;
        std::vector<float> keyframes_;
        std::vector<glm::vec4> values_;
        GltfAnimationChannelMode mode_;
        GltfAnimationChannelType type_;
    };

    struct GltfAnimation
    {
        std::vector<uint64_t> nodes_;
        std::vector<GltfAnimationChannel> channels_;
    };

    GfxArray<GltfNode> gltf_nodes_;
    GfxArray<uint32_t> gltf_node_refs_;
    GfxArray<GltfAnimatedNode> gltf_animated_nodes_;
    GfxHandles gltf_node_handles_;
    GfxArray<GltfAnimation> gltf_animations_;

    GfxArray<GfxAnimation> animations_;
    GfxArray<uint64_t> animation_refs_;
    GfxArray<GfxMetadata> animation_metadata_;
    GfxHandles animation_handles_;

    GfxArray<GfxCamera> cameras_;
    GfxArray<uint64_t> camera_refs_;
    GfxArray<GfxMetadata> camera_metadata_;
    GfxConstRef<GfxCamera> active_camera_;
    GfxHandles camera_handles_;

    GfxArray<GfxImage> images_;
    GfxArray<uint64_t> image_refs_;
    GfxArray<GfxMetadata> image_metadata_;
    GfxHandles image_handles_;

    GfxArray<GfxMaterial> materials_;
    GfxArray<uint64_t> material_refs_;
    GfxArray<GfxMetadata> material_metadata_;
    GfxHandles material_handles_;

    GfxArray<GfxMesh> meshes_;
    GfxArray<uint64_t> mesh_refs_;
    GfxArray<GfxMetadata> mesh_metadata_;
    GfxHandles mesh_handles_;

    GfxArray<GfxInstance> instances_;
    GfxArray<uint64_t> instance_refs_;
    GfxArray<GfxMetadata> instance_metadata_;
    GfxHandles instance_handles_;

    static GfxArray<GfxScene> scenes_;
    static GfxHandles scene_handles_;

    template<typename TYPE> GfxArray<TYPE> &objects_();
    template<typename TYPE> GfxArray<uint64_t> &object_refs_();
    template<typename TYPE> GfxArray<GfxMetadata> &object_metadata_();
    template<typename TYPE> GfxHandles &object_handles_();

    template<> inline GfxArray<GfxAnimation> &objects_<GfxAnimation>() { return animations_; }
    template<> inline GfxArray<uint64_t> &object_refs_<GfxAnimation>() { return animation_refs_; }
    template<> inline GfxArray<GfxMetadata> &object_metadata_<GfxAnimation>() { return animation_metadata_; }
    template<> inline GfxHandles &object_handles_<GfxAnimation>() { return animation_handles_; }

    template<> inline GfxArray<GfxCamera> &objects_<GfxCamera>() { return cameras_; }
    template<> inline GfxArray<uint64_t> &object_refs_<GfxCamera>() { return camera_refs_; }
    template<> inline GfxArray<GfxMetadata> &object_metadata_<GfxCamera>() { return camera_metadata_; }
    template<> inline GfxHandles &object_handles_<GfxCamera>() { return camera_handles_; }

    template<> inline GfxArray<GfxImage> &objects_<GfxImage>() { return images_; }
    template<> inline GfxArray<uint64_t> &object_refs_<GfxImage>() { return image_refs_; }
    template<> inline GfxArray<GfxMetadata> &object_metadata_<GfxImage>() { return image_metadata_; }
    template<> inline GfxHandles &object_handles_<GfxImage>() { return image_handles_; }

    template<> inline GfxArray<GfxMaterial> &objects_<GfxMaterial>() { return materials_; }
    template<> inline GfxArray<uint64_t> &object_refs_<GfxMaterial>() { return material_refs_; }
    template<> inline GfxArray<GfxMetadata> &object_metadata_<GfxMaterial>() { return material_metadata_; }
    template<> inline GfxHandles &object_handles_<GfxMaterial>() { return material_handles_; }

    template<> inline GfxArray<GfxMesh> &objects_<GfxMesh>() { return meshes_; }
    template<> inline GfxArray<uint64_t> &object_refs_<GfxMesh>() { return mesh_refs_; }
    template<> inline GfxArray<GfxMetadata> &object_metadata_<GfxMesh>() { return mesh_metadata_; }
    template<> inline GfxHandles &object_handles_<GfxMesh>() { return mesh_handles_; }

    template<> inline GfxArray<GfxInstance> &objects_<GfxInstance>() { return instances_; }
    template<> inline GfxArray<uint64_t> &object_refs_<GfxInstance>() { return instance_refs_; }
    template<> inline GfxArray<GfxMetadata> &object_metadata_<GfxInstance>() { return instance_metadata_; }
    template<> inline GfxHandles &object_handles_<GfxInstance>() { return instance_handles_; }

public:
    GfxSceneInternal(GfxScene &scene) : animation_handles_("animation"), camera_handles_("camera"), image_handles_("image")
                                      , material_handles_("material"), mesh_handles_("mesh"), instance_handles_("instance")
                                      { scene.handle = reinterpret_cast<uint64_t>(this); }
    ~GfxSceneInternal() { terminate(); }

    GfxResult initialize()
    {
        return kGfxResult_NoError;
    }

    GfxResult terminate()
    {
        GFX_TRY(clear());

        return kGfxResult_NoError;
    }

    GfxResult import(GfxScene const &scene, char const *asset_file)
    {
        if(asset_file == nullptr)
            return kGfxResult_InvalidParameter;
        char const *asset_extension = strrchr(asset_file, '.');
        if(asset_extension == nullptr)
            return GFX_SET_ERROR(kGfxResult_InvalidParameter, "Unable to determine extension for asset file `%s'", asset_file);
        if(CaseInsensitiveCompare(asset_extension, ".obj") ||
           CaseInsensitiveCompare(asset_extension, ".objm"))
            GFX_TRY(importObj(scene, asset_file));
        else if(CaseInsensitiveCompare(asset_extension, ".glb") ||
                CaseInsensitiveCompare(asset_extension, ".gltf"))
            GFX_TRY(importGltf(scene, asset_file));
        else if(CaseInsensitiveCompare(asset_extension, ".hdr"))
            GFX_TRY(importHdr(scene, asset_file));
        else if(CaseInsensitiveCompare(asset_extension, ".bmp") ||
                CaseInsensitiveCompare(asset_extension, ".png") ||
                CaseInsensitiveCompare(asset_extension, ".jpg") ||
                CaseInsensitiveCompare(asset_extension, ".jpeg"))
            GFX_TRY(importImage(scene, asset_file));
        else
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot import unsupported asset type `%s'", asset_extension);
        return kGfxResult_NoError;
    }

    GfxResult clear()
    {
        clearObjects<GfxAnimation>();
        clearObjects<GfxCamera>();
        clearObjects<GfxImage>();
        clearObjects<GfxMaterial>();
        clearObjects<GfxMesh>();
        clearObjects<GfxInstance>();

        return kGfxResult_NoError;
    }

    GfxResult applyAnimation(uint64_t animation_handle, float time_in_seconds)
    {
        if(!animation_handles_.has_handle(animation_handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot apply animation of an invalid object");
        GltfAnimation const *gltf_animation = gltf_animations_.at(GetObjectIndex(animation_handle));
        if(gltf_animation != nullptr)
        {
            for(size_t i = 0; i < gltf_animation->channels_.size(); ++i)
            {
                double interpolate = 1.0;
                glm::dvec4 previous_value, next_value;
                GltfAnimationChannel const &animation_channel = gltf_animation->channels_[i];
                if(!gltf_node_handles_.has_handle(animation_channel.node_)) continue;   // invalid target node
                GltfAnimatedNode *animated_node = gltf_animated_nodes_.at(GetObjectIndex(animation_channel.node_));
                if(animation_channel.keyframes_.empty() || animated_node == nullptr) { GFX_ASSERT(0); continue; }
                uint32_t const keyframe = (uint32_t)(std::lower_bound(animation_channel.keyframes_.data(),
                                                                      animation_channel.keyframes_.data() + animation_channel.keyframes_.size(), time_in_seconds)
                                                                    - animation_channel.keyframes_.data());
                if(keyframe == 0)
                    previous_value = next_value = glm::dvec4(animation_channel.values_.front());
                else if(keyframe >= (uint32_t)animation_channel.keyframes_.size())
                    previous_value = next_value = glm::dvec4(animation_channel.values_.back());
                else
                {
                    if(animation_channel.mode_ == kGltfAnimationChannelMode_Linear)
                    {
                        interpolate = ((double)time_in_seconds                        - (double)animation_channel.keyframes_[keyframe - 1])
                                    / ((double)animation_channel.keyframes_[keyframe] - (double)animation_channel.keyframes_[keyframe - 1]);
                    }
                    previous_value = glm::dvec4(animation_channel.values_[keyframe - 1]);
                    next_value     = glm::dvec4(animation_channel.values_[keyframe]);
                }
                switch(animation_channel.type_)
                {
                case kGltfAnimationChannelType_Translate:
                    animated_node->translate_ = glm::mix(glm::dvec3(previous_value.x, previous_value.y, previous_value.z),
                                                         glm::dvec3(next_value.x,     next_value.y,     next_value.z), interpolate);
                    break;
                case kGltfAnimationChannelType_Rotate:
                    animated_node->rotate_ = glm::slerp(glm::dquat(previous_value.w, previous_value.x, previous_value.y, previous_value.z),
                                                        glm::dquat(next_value.w,     next_value.x,     next_value.y,     next_value.z), interpolate);
                    break;
                case kGltfAnimationChannelType_Scale:
                    animated_node->scale_ = glm::mix(glm::dvec3(previous_value.x, previous_value.y, previous_value.z),
                                                     glm::dvec3(next_value.x,     next_value.y,     next_value.z), interpolate);
                    break;
                default:
                    GFX_ASSERT(0);
                    break;  // should never happen
                }
            }
            std::function<void(uint64_t, glm::dmat4 const &)> VisitNode;
            VisitNode = [&](uint64_t node_handle, glm::dmat4 const &parent_transform)
            {
                glm::dmat4 transform;
                if(!gltf_node_handles_.has_handle(node_handle)) return;
                GltfNode const &node = gltf_nodes_[GetObjectIndex(node_handle)];
                GltfAnimatedNode *animated_node = gltf_animated_nodes_.at(GetObjectIndex(node_handle));
                if(animated_node == nullptr)
                    transform = parent_transform * node.matrix_;
                else
                {
                    glm::dmat4 const translate = glm::translate(glm::dmat4(1.0), animated_node->translate_);
                    glm::dmat4 const rotate = glm::toMat4(animated_node->rotate_);
                    glm::dmat4 const scale = glm::scale(glm::dmat4(1.0), animated_node->scale_);
                    transform = parent_transform * translate * rotate * scale;
                    animated_node->translate_ = node.translate_;
                    animated_node->rotate_ = node.rotate_;
                    animated_node->scale_ = node.scale_;
                }
                for(size_t i = 0; i < node.children_.size(); ++i)
                    VisitNode(node.children_[i], transform);
                for(size_t i = 0; i < node.instances_.size(); ++i)
                    if(node.instances_[i])
                        node.instances_[i]->transform = glm::mat4(transform);
            };
            for(size_t i = 0; i < gltf_animation->nodes_.size(); ++i)
                VisitNode(gltf_animation->nodes_[i], glm::dmat4(1.0));
        }
        return kGfxResult_NoError;
    }

    GfxResult resetAnimation(uint64_t animation_handle)
    {
        if(!animation_handles_.has_handle(animation_handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot reset animation of an invalid object");
        GltfAnimation const *gltf_animation = gltf_animations_.at(GetObjectIndex(animation_handle));
        if(gltf_animation != nullptr)
        {
            std::function<void(uint64_t, glm::dmat4 const &)> VisitNode;
            VisitNode = [&](uint64_t node_handle, glm::dmat4 const &parent_transform)
            {
                if(!gltf_node_handles_.has_handle(node_handle)) return;
                GltfNode const &node = gltf_nodes_[GetObjectIndex(node_handle)];
                glm::dmat4 const transform = parent_transform * node.matrix_;
                for(size_t i = 0; i < node.children_.size(); ++i)
                    VisitNode(node.children_[i], transform);
                for(size_t i = 0; i < node.instances_.size(); ++i)
                    if(node.instances_[i])
                        node.instances_[i]->transform = glm::mat4(transform);
            };
            for(size_t i = 0; i < gltf_animation->nodes_.size(); ++i)
                VisitNode(gltf_animation->nodes_[i], glm::dmat4(1.0));
        }
        return kGfxResult_NoError;
    }

    GfxResult resetAllAnimation()
    {
        uint32_t const animation_count = animation_refs_.size();
        for(uint32_t i = 0; i < animation_count; ++i)
        {
            uint64_t const animation_handle = animation_refs_[animation_refs_.get_index(i)];
            resetAnimation(animation_handle);   // reset animation
        }
        return kGfxResult_NoError;
    }

    float getAnimationLength(uint64_t animation_handle)
    {
        if(!animation_handles_.has_handle(animation_handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot get the duration of an invalid animation object");
            return 0.0f;    // invalid operation
        }
        return GFX_MAX(getAnimationEnd(animation_handle) - getAnimationStart(animation_handle), 0.0f);
    }

    float getAnimationStart(uint64_t animation_handle)
    {
        float animation_start = 0.0f;
        if(!animation_handles_.has_handle(animation_handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot get the start of an invalid animation object");
            return animation_start; // invalid operation
        }
        GltfAnimation const *gltf_animation = gltf_animations_.at(GetObjectIndex(animation_handle));
        if(gltf_animation != nullptr)
            for(size_t i = 0; i < gltf_animation->channels_.size(); ++i)
                if(!gltf_animation->channels_[i].keyframes_.empty())
                    animation_start = GFX_MIN(animation_start, gltf_animation->channels_[i].keyframes_.front());
        return animation_start;
    }

    float getAnimationEnd(uint64_t animation_handle)
    {
        float animation_end = 0.0f;
        if(!animation_handles_.has_handle(animation_handle))
        {
            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Cannot get the end of an invalid animation object");
            return animation_end;   // invalid operation
        }
        GltfAnimation const *gltf_animation = gltf_animations_.at(GetObjectIndex(animation_handle));
        if(gltf_animation != nullptr)
            for(size_t i = 0; i < gltf_animation->channels_.size(); ++i)
                if(!gltf_animation->channels_[i].keyframes_.empty())
                    animation_end = GFX_MAX(animation_end, gltf_animation->channels_[i].keyframes_.back());
        return animation_end;
    }

    GfxResult setActiveCamera(GfxScene const &scene, uint64_t camera_handle)
    {
        active_camera_.handle = camera_handle;
        active_camera_.scene = scene;

        return kGfxResult_NoError;
    }

    GfxConstRef<GfxCamera> getActiveCamera()
    {
        return active_camera_;
    }

    template<typename TYPE>
    GfxRef<TYPE> createObject(GfxScene const &scene)
    {
        GfxRef<TYPE> object_ref = {};
        object_ref.handle = object_handles_<TYPE>().allocate_handle();
        object_refs_<TYPE>().insert(GetObjectIndex(object_ref.handle), object_ref.handle);
        object_metadata_<TYPE>().insert(GetObjectIndex(object_ref.handle)).is_valid = true;
        objects_<TYPE>().insert(GetObjectIndex(object_ref.handle)) = {};
        object_ref.scene = scene;
        return object_ref;
    }

    template<typename TYPE>
    GfxResult destroyObjectCallback(uint64_t object_handle);

    template<>
    GfxResult destroyObjectCallback<GfxAnimation>(uint64_t object_handle)
    {
        GFX_ASSERT(animation_handles_.has_handle(object_handle));
        GltfAnimation const *gltf_animation = gltf_animations_.at(GetObjectIndex(object_handle));
        if(gltf_animation != nullptr)
        {
            std::function<void(uint64_t)> VisitNode;
            VisitNode = [&](uint64_t node_handle)
            {
                if(!gltf_node_handles_.has_handle(node_handle)) return;
                uint32_t const children_count = (uint32_t)gltf_nodes_[GetObjectIndex(node_handle)].children_.size();
                for(size_t i = 0; i < children_count; ++i)
                {
                    GltfNode const &node = gltf_nodes_[GetObjectIndex(node_handle)];
                    VisitNode(node.children_[i]);   // release child nodes
                }
                uint32_t *node_ref = gltf_node_refs_.at(GetObjectIndex(node_handle));
                if(node_ref == nullptr || --(*node_ref) == 0)
                {
                    gltf_node_handles_.free_handle(node_handle);
                    if(gltf_nodes_.has(GetObjectIndex(node_handle)))
                        gltf_nodes_.erase(GetObjectIndex(node_handle));
                    if(gltf_node_refs_.has(GetObjectIndex(node_handle)))
                        gltf_node_refs_.erase(GetObjectIndex(node_handle));
                    if(gltf_animated_nodes_.has(GetObjectIndex(node_handle)))
                        gltf_animated_nodes_.erase(GetObjectIndex(node_handle));
                }
            };
            for(size_t i = 0; i < gltf_animation->nodes_.size(); ++i)
                VisitNode(gltf_animation->nodes_[i]);
            gltf_animations_.erase(GetObjectIndex(object_handle));
        }
        return kGfxResult_NoError;
    }

    template<typename TYPE>
    GfxResult destroyObject(uint64_t object_handle)
    {
        if(object_handle == 0)
            return kGfxResult_NoError;
        if(!object_handles_<TYPE>().has_handle(object_handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid scene object");
        destroyObjectCallback<TYPE>(object_handle);
        objects_<TYPE>().erase(GetObjectIndex(object_handle));
        object_refs_<TYPE>().erase(GetObjectIndex(object_handle));
        object_metadata_<TYPE>().erase(GetObjectIndex(object_handle));
        object_handles_<TYPE>().free_handle(object_handle);
        return kGfxResult_NoError;
    }

    template<typename TYPE>
    GfxResult clearObjects()
    {
        while(getObjectCount<TYPE>() > 0)
            GFX_TRY(destroyObject<TYPE>(object_refs_<TYPE>().data()[0]));
        return kGfxResult_NoError;
    }

    template<typename TYPE>
    uint32_t getObjectCount()
    {
        return objects_<TYPE>().size();
    }

    template<typename TYPE>
    TYPE const *getObjects()
    {
        return objects_<TYPE>().data();
    }

    template<typename TYPE>
    TYPE *getObject(uint64_t object_handle)
    {
        if(!object_handles_<TYPE>().has_handle(object_handle))
            return nullptr; // invalid object handle
        return objects_<TYPE>().at(GetObjectIndex(object_handle));
    }

    template<typename TYPE>
    GfxRef<TYPE> getObjectHandle(GfxScene const &scene, uint32_t object_index)
    {
        GfxRef<TYPE> object_ref = {};
        if(object_index >= object_refs_<TYPE>().size())
            return object_ref;  // out of bounds
        object_ref.handle = object_refs_<TYPE>()[object_refs_<TYPE>().get_index(object_index)];
        object_ref.scene = scene;
        return object_ref;
    }

    template<typename TYPE>
    GfxMetadata const &getObjectMetadata(uint64_t object_handle)
    {
        static GfxMetadata const metadata = {};
        if(!object_handles_<TYPE>().has_handle(object_handle))
            return metadata;    // invalid object handle
        return object_metadata_<TYPE>()[GetObjectIndex(object_handle)];
    }

    static inline GfxSceneInternal *GetGfxScene(GfxScene scene) { return reinterpret_cast<GfxSceneInternal *>(scene.handle); }

private:
    static inline uint32_t GetObjectIndex(uint64_t object_handle)
    {
        return (uint32_t)(object_handle != 0 ? object_handle & 0xFFFFFFFFull : 0xFFFFFFFFull);
    }

    static inline bool CaseInsensitiveCompare(char const *str1, char const *str2)
    {
        GFX_ASSERT(str1 != nullptr && str2 != nullptr);
        for(uint32_t i = 0;; ++i)
            if(tolower(str1[i]) != tolower(str2[i]))
                return false;
            else if(str1[i] == '\0')
                break;
        return true;
    }

    GfxResult importObj(GfxScene const &scene, char const *asset_file)
    {
        tinyobj::ObjReader obj_reader;
        GFX_ASSERT(asset_file != nullptr);
        tinyobj::ObjReaderConfig obj_reader_config;
        obj_reader_config.vertex_color = false;
        if(!obj_reader.ParseFromFile(asset_file, obj_reader_config))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Failed to load obj file `%s'", obj_reader.Error().c_str());
        if(!obj_reader.Warning().empty())
            GFX_PRINTLN("Parsed obj file `%s' with warnings:\r\n%s", asset_file, obj_reader.Warning().c_str());
        char const *file = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '\\'));    // retrieve file name
        std::string const texture_path = (file == nullptr ? "./" : std::string(asset_file, file - asset_file + 1));
        auto const LoadImage = [&](std::string const &texname, GfxConstRef<GfxImage> &image)
        {
            if(texname.empty()) return; // no image to be loaded
            std::string const texture_file = texture_path + texname;
            if(gfxSceneImport(scene, texture_file.c_str()) != kGfxResult_NoError)
                return; // unable to load image file
            image = gfxSceneFindObjectByAssetFile<GfxImage>(scene, texture_file.c_str());
        };
        std::vector<GfxConstRef<GfxMaterial>> materials(obj_reader.GetMaterials().size());
        for(size_t i = 0; i < obj_reader.GetMaterials().size(); ++i)
        {
            GfxRef<GfxMaterial> material_ref = gfxSceneCreateMaterial(scene);
            tinyobj::material_t const &obj_material = obj_reader.GetMaterials()[i];
            GfxMetadata &material_metadata = material_metadata_[material_ref];
            material_metadata.asset_file = asset_file;  // set up metadata
            material_metadata.object_name = obj_material.name;
            material_ref->albedo = glm::vec4(obj_material.diffuse[0], obj_material.diffuse[1], obj_material.diffuse[2], obj_material.dissolve);
            material_ref->roughness = obj_material.roughness;
            material_ref->metallicity = obj_material.metallic;
            material_ref->emissivity = glm::vec3(obj_material.emission[0], obj_material.emission[1], obj_material.emission[2]);
            LoadImage(obj_material.diffuse_texname, material_ref->albedo_map);
            if(material_ref->albedo_map) material_ref->albedo = glm::vec4(glm::vec3(1.0f), material_ref->albedo.w);
            LoadImage(obj_material.roughness_texname, material_ref->roughness_map);
            if(material_ref->roughness_map) material_ref->roughness = 1.0f;
            LoadImage(obj_material.metallic_texname, material_ref->metallicity_map);
            if(material_ref->metallicity_map) material_ref->metallicity = 1.0f;
            LoadImage(obj_material.emissive_texname, material_ref->emissivity_map);
            if(material_ref->emissivity_map) material_ref->emissivity = glm::vec3(1.0f);
            materials[i] = material_ref;    // append the new material
        }
        for(size_t i = 0; i < obj_reader.GetShapes().size(); ++i)
        {
            uint32_t first_index = 0;
            tinyobj::shape_t const &obj_shape = obj_reader.GetShapes()[i];
            if(obj_shape.mesh.indices.empty()) continue;    // only support indexed meshes for now
            typedef std::map<std::tuple<int32_t, int32_t, int32_t>, std::pair<uint32_t, GfxVertex>> VertexMap;
            std::map<uint32_t, VertexMap> mesh_map; // based on material index
            std::map<uint32_t, std::vector<uint32_t>> index_map;
            for(size_t j = 0; j < obj_shape.mesh.num_face_vertices.size(); ++j)
            {
                uint32_t const index_offset = first_index;
                first_index += obj_shape.mesh.num_face_vertices[j];
                if(obj_shape.mesh.num_face_vertices[j] != 3) continue;  // only support triangle meshes
                uint32_t const material_idx = (j < obj_shape.mesh.material_ids.size() ? obj_shape.mesh.material_ids[j] : 0xFFFFFFFFu);
                VertexMap &vertex_map = mesh_map[material_idx]; // get hold of the vertex map
                std::vector<uint32_t> &indices = index_map[material_idx];
                for(uint32_t k = index_offset; k < index_offset + 3; ++k)
                {
                    std::tuple<int32_t, int32_t, int32_t> const key(obj_shape.mesh.indices[k].vertex_index,
                                                                    obj_shape.mesh.indices[k].normal_index,
                                                                    obj_shape.mesh.indices[k].texcoord_index);
                    VertexMap::const_iterator const it = vertex_map.find(key);
                    if(it != vertex_map.end())
                        indices.push_back((*it).second.first);
                    else
                    {
                        GfxVertex vertex = {};
                        if(std::get<0>(key) >= 0)
                        {
                            vertex.position.x = obj_reader.GetAttrib().vertices[3 * std::get<0>(key) + 0];
                            vertex.position.y = obj_reader.GetAttrib().vertices[3 * std::get<0>(key) + 1];
                            vertex.position.z = obj_reader.GetAttrib().vertices[3 * std::get<0>(key) + 2];
                        }
                        if(std::get<1>(key) >= 0)
                        {
                            vertex.normal.x = obj_reader.GetAttrib().normals[3 * std::get<1>(key) + 0];
                            vertex.normal.y = obj_reader.GetAttrib().normals[3 * std::get<1>(key) + 1];
                            vertex.normal.z = obj_reader.GetAttrib().normals[3 * std::get<1>(key) + 2];
                        }
                        if(std::get<2>(key) >= 0)
                        {
                            vertex.uv.x = obj_reader.GetAttrib().texcoords[2 * std::get<2>(key) + 0];
                            vertex.uv.y = obj_reader.GetAttrib().texcoords[2 * std::get<2>(key) + 1];
                        }
                        uint32_t const index = (uint32_t)vertex_map.size();
                        vertex_map[key] = std::pair<uint32_t, GfxVertex>(index, vertex);
                        indices.push_back(index);   // append index to new vertex
                    }
                }
            }
            uint32_t mesh_id = 0;
            for(std::map<uint32_t, VertexMap>::const_iterator it = mesh_map.begin(); it != mesh_map.end(); ++it, ++mesh_id)
            {
                VertexMap const &vertices = (*it).second;
                std::vector<uint32_t> &indices = index_map[(*it).first];
                if(indices.empty()) continue;   // do not create empty meshes
                GfxRef<GfxMesh> mesh_ref = gfxSceneCreateMesh(scene);
                std::swap(mesh_ref->indices, indices);
                mesh_ref->vertices.resize(vertices.size());
                GfxMetadata &mesh_metadata = mesh_metadata_[mesh_ref];
                mesh_metadata.asset_file = asset_file;  // set up metadata
                mesh_metadata.object_name = obj_shape.name;
                if(mesh_id > 0)
                {
                    mesh_metadata.object_name += ".";
                    mesh_metadata.object_name += std::to_string(mesh_id);
                }
                if((*it).first < materials.size())
                    mesh_ref->material = materials[(*it).first];
                glm::vec3 bounds_min(FLT_MAX), bounds_max(-FLT_MAX);
                for(VertexMap::const_iterator it2 = vertices.begin(); it2 != vertices.end(); ++it2)
                {
                    uint32_t const index = (*it2).second.first;
                    GfxVertex const &vertex = (*it2).second.second;
                    bounds_min = glm::min(bounds_min, vertex.position);
                    bounds_max = glm::max(bounds_max, vertex.position);
                    mesh_ref->vertices[index] = vertex; // append vertex
                }
                mesh_ref->bounds_min = bounds_min;
                mesh_ref->bounds_max = bounds_max;
                GfxRef<GfxInstance> instance_ref = gfxSceneCreateInstance(scene);
                instance_ref->mesh = mesh_ref;  // .obj does not support instancing, so simply create one instance per mesh
                GfxMetadata &instance_metadata = instance_metadata_[instance_ref];
                instance_metadata.asset_file = asset_file;  // set up metadata
                instance_metadata.object_name = obj_shape.name;
                if(mesh_id > 0)
                {
                    instance_metadata.object_name += ".";
                    instance_metadata.object_name += std::to_string(mesh_id);
                }
            }
        }
        return kGfxResult_NoError;
    }

    struct GltfBuffer
    {
        uint32_t type_;
        uint32_t count_;
        uint32_t stride_;
        uint32_t normalize_;
        uint8_t const *data_;
        uint32_t component_type_;
    };

    template<typename TYPE>
    struct GltfComponent {};

    template<> struct GltfComponent<float>    { typedef float    Type; };
    template<> struct GltfComponent<int32_t>  { typedef int32_t  Type; };
    template<> struct GltfComponent<uint32_t> { typedef uint32_t Type; };

    template<> struct GltfComponent<glm::vec2> { typedef float Type; };
    template<> struct GltfComponent<glm::vec3> { typedef float Type; };
    template<> struct GltfComponent<glm::vec4> { typedef float Type; };

    template<> struct GltfComponent<glm::mat2> { typedef float Type; };
    template<> struct GltfComponent<glm::mat3> { typedef float Type; };
    template<> struct GltfComponent<glm::mat4> { typedef float Type; };

    template<typename TYPE>
    TYPE ReadAs(GltfBuffer const &buffer, uint32_t index)
    {
        TYPE value = {};
        if(index >= buffer.count_) return value;
        using ComponentType = GltfComponent<TYPE>::Type;
        uint8_t const *data = (buffer.data_ + index * buffer.stride_);
        uint32_t const num_components = tinygltf::GetNumComponentsInType(buffer.type_);
        uint32_t const component_size = tinygltf::GetComponentSizeInBytes(buffer.component_type_);
        GFX_ASSERT((buffer.type_ != TINYGLTF_TYPE_MAT2 || component_size != 1) &&
                   (buffer.type_ != TINYGLTF_TYPE_MAT3 || component_size != 1) &&
                   (buffer.type_ != TINYGLTF_TYPE_MAT3 || component_size != 2));    // unsupported alignment
        for(uint32_t i = 0; i < num_components; ++i)
        {
            if((i + 1) * component_size > buffer.stride_) break;
            if((i + 1) * sizeof(ComponentType) > sizeof(TYPE)) break;
            ComponentType &output = ((ComponentType *)((void *)&value))[i];
            switch(buffer.component_type_)
            {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
                {
                    int8_t const t = ((int8_t *)data)[i];
                    if(buffer.normalize_) output = (ComponentType)GFX_MAX(t / 127.0, -1.0);
                                     else output = (ComponentType)(t);
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                {
                    uint8_t const t = ((uint8_t *)data)[i];
                    if(buffer.normalize_) output = (ComponentType)(t / 255.0);
                                     else output = (ComponentType)(t);
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
                {
                    int16_t const t = ((int16_t *)data)[i];
                    if(buffer.normalize_) output = (ComponentType)GFX_MAX(t / 32727.0, -1.0);
                                     else output = (ComponentType)(t);
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                {
                    uint16_t const t = ((uint16_t *)data)[i];
                    if(buffer.normalize_) output = (ComponentType)(t / 65535.0);
                                     else output = (ComponentType)(t);
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_INT:
                {
                    GFX_ASSERT(!buffer.normalize_);
                    output = (ComponentType)((int32_t *)data)[i];
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                {
                    GFX_ASSERT(!buffer.normalize_);
                    output = (ComponentType)((uint32_t *)data)[i];
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                {
                    GFX_ASSERT(!buffer.normalize_);
                    output = (ComponentType)((float *)data)[i];
                }
                break;
            case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                {
                    GFX_ASSERT(!buffer.normalize_);
                    output = (ComponentType)((double *)data)[i];
                }
                break;
            default:
                GFX_ASSERT(0);
                break;  // unsupported component type
            }
        }
        return value;
    }

    GfxResult importGltf(GfxScene const &scene, char const *asset_file)
    {
        bool result;
        tinygltf::Model gltf_model;
        std::string errors, warnings;
        tinygltf::TinyGLTF gltf_reader;
        GFX_ASSERT(asset_file != nullptr);
        char const *asset_extension = strrchr(asset_file, '.');
        GFX_ASSERT(asset_extension != nullptr);
        if(CaseInsensitiveCompare(asset_extension, ".glb"))
            result = gltf_reader.LoadBinaryFromFile(&gltf_model, &errors, &warnings, asset_file);
        else
            result = gltf_reader.LoadASCIIFromFile(&gltf_model, &errors, &warnings, asset_file);
        if(!result)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Failed to load gltf file `%s'", errors.c_str());
        if(!warnings.empty())
            GFX_PRINTLN("Parsed gltf file `%s' with warnings:\r\n%s", asset_file, warnings.c_str());
        if(gltf_model.scenes.empty())
            return kGfxResult_NoError;  // nothing needs loading
        std::map<int32_t, GfxConstRef<GfxImage>> images;
        for(size_t i = 0; i < gltf_model.textures.size(); ++i)
        {
            tinygltf::Texture const &gltf_texture = gltf_model.textures[i];
            if(gltf_texture.source < 0 || gltf_texture.source >= (int32_t)gltf_model.images.size()) continue;
            tinygltf::Image const &gltf_image = gltf_model.images[gltf_texture.source];
            if(gltf_image.image.empty()) continue;  // failed to load image
            GfxRef<GfxImage> image_ref = gfxSceneCreateImage(scene);
            image_ref->data = gltf_image.image;
            image_ref->width = gltf_image.width;
            image_ref->height = gltf_image.height;
            image_ref->channel_count = gltf_image.component;
            image_ref->bytes_per_channel = (gltf_image.bits >> 3);
            image_ref->data.resize(image_ref->width * image_ref->height * image_ref->channel_count * image_ref->bytes_per_channel);
            char const *image_name = gltf_texture.name.c_str();
            std::string image_file = asset_file;
            if(!gltf_image.uri.empty())
            {
                char const *filename = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '//'));
                image_file = (filename != nullptr ? std::string(asset_file, 0, filename - asset_file) : std::string(""));
                if(!image_file.empty() && image_file.back() != '/' && image_file.back() != '\\') image_file += '/';
                filename = GFX_MAX(strrchr(gltf_image.uri.c_str(), '/'), strrchr(gltf_image.uri.c_str(), '//'));
                image_name  = (filename != nullptr ? filename + 1 : gltf_image.uri.c_str());
                image_file += gltf_image.uri.c_str();
            }
            GfxMetadata &image_metadata = image_metadata_[image_ref];
            image_metadata.asset_file = image_file; // set up metadata
            image_metadata.object_name = image_name;
            images[(int32_t)i] = image_ref;
        }
        auto const GetParameter = [&](tinygltf::Material const &gltf_material, char const *parameter_name) -> std::pair<bool, glm::vec4>
        {
            std::pair<bool, glm::vec4> parameter = std::pair<bool, glm::vec4>(false, glm::vec4(0.0f));
            tinygltf::ParameterMap::const_iterator it = gltf_material.values.find(parameter_name);
            if(it == gltf_material.values.end())
            {
                it = gltf_material.additionalValues.find(parameter_name);
                if(it == gltf_material.additionalValues.end())
                    return parameter;   // not found
            }
            if(!(*it).second.number_array.empty())
                parameter.second = glm::vec4(                                       (float)(*it).second.number_array[0],
                                             (*it).second.number_array.size() > 1 ? (float)(*it).second.number_array[1] : 1.0f,
                                             (*it).second.number_array.size() > 2 ? (float)(*it).second.number_array[2] : 1.0f,
                                             (*it).second.number_array.size() > 3 ? (float)(*it).second.number_array[3] : 1.0f);
            else if(!(*it).second.has_number_value)
                return parameter;   // not found
            else
                parameter.second = glm::vec4((float)(*it).second.number_value);
            parameter.first = true;
            return parameter;
        };
        auto const GetTextureIndex = [&](tinygltf::Material const &gltf_material, char const *texture_name) -> int32_t
        {
            tinygltf::ParameterMap::const_iterator it = gltf_material.values.find(texture_name);
            if(it == gltf_material.values.end())
            {
                it = gltf_material.additionalValues.find(texture_name);
                if(it == gltf_material.additionalValues.end())
                    return -1;  // not found
            }
            std::map<std::string, double>::const_iterator const it2 = (*it).second.json_double_value.find("index");
            if(it2 == (*it).second.json_double_value.end())
                return -1;  // not found
            std::map<std::string, double>::const_iterator const it3 = (*it).second.json_double_value.find("texCoord");
            if(it3 != (*it).second.json_double_value.end() && (*it3).second != 0.0)
                return -1;  // only a single uv stream is supported for now
            return (int32_t)(*it2).second;
        };
        std::pair<bool, glm::vec4> parameter;
        std::map<int32_t, GfxConstRef<GfxMaterial>> materials;
        std::map<int32_t, std::pair<GfxConstRef<GfxImage>, GfxConstRef<GfxImage>>> maps;
        for(size_t i = 0; i < gltf_model.materials.size(); ++i)
        {
            std::map<int32_t, GfxConstRef<GfxImage>>::const_iterator it;
            tinygltf::Material const &gltf_material = gltf_model.materials[i];
            GfxRef<GfxMaterial> material_ref = gfxSceneCreateMaterial(scene);
            GfxMetadata &material_metadata = material_metadata_[material_ref];
            material_metadata.asset_file = asset_file;  // set up metadata
            material_metadata.object_name = gltf_material.name;
            GfxMaterial &material = *material_ref;
            parameter = GetParameter(gltf_material, "baseColorFactor");
            if(parameter.first) material.albedo = glm::vec4(parameter.second);
                           else material.albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            parameter = GetParameter(gltf_material, "roughnessFactor");
            if(parameter.first) material.roughness = parameter.second.x;
                           else material.roughness = 1.0f;
            parameter = GetParameter(gltf_material, "metallicFactor");
            if(parameter.first) material.metallicity = parameter.second.x;
                           else material.metallicity = 1.0f;
            parameter = GetParameter(gltf_material, "emissiveFactor");
            if(parameter.first) material.emissivity = glm::vec3(parameter.second);
                           else material.emissivity = glm::vec3(0.0f, 0.0f, 0.0f);
            int32_t const albedo_map = GetTextureIndex(gltf_material, "baseColorTexture");
            it = (albedo_map >= 0 ? images.find(albedo_map) : images.end());
            if(it != images.end()) material.albedo_map = (*it).second;
            int32_t const metallicity_roughness_map = GetTextureIndex(gltf_material, "metallicRoughnessTexture");
            it = (metallicity_roughness_map >= 0 ? images.find(metallicity_roughness_map) : images.end());
            if(it != images.end())
            {
                std::map<int32_t, std::pair<GfxConstRef<GfxImage>, GfxConstRef<GfxImage>>>::const_iterator const it2 = maps.find(metallicity_roughness_map);
                if(it2 != maps.end())
                {
                    material_ref->roughness_map = (*it2).second.first;
                    material_ref->metallicity_map = (*it2).second.second;
                }
                else
                {
                    tinygltf::Texture const &gltf_texture = gltf_model.textures[metallicity_roughness_map];
                    tinygltf::Image const &gltf_image = gltf_model.images[gltf_texture.source];
                    GfxRef<GfxImage> metallicity_map_ref = gfxSceneCreateImage(scene);
                    GfxRef<GfxImage> roughness_map_ref = gfxSceneCreateImage(scene);
                    GfxMetadata const &image_metadata = image_metadata_[(*it).second];
                    GfxMetadata &metallicity_map_metadata = image_metadata_[metallicity_map_ref];
                    GfxMetadata &roughness_map_metadata = image_metadata_[roughness_map_ref];   // set up metadata
                    metallicity_map_metadata = image_metadata;
                    metallicity_map_metadata.asset_file += ".x";
                    metallicity_map_metadata.object_name += ".x";
                    roughness_map_metadata = image_metadata;
                    roughness_map_metadata.asset_file += ".y";
                    roughness_map_metadata.object_name += ".y";
                    GfxImage &metallicity_map = *metallicity_map_ref;
                    GfxImage &roughness_map = *roughness_map_ref;
                    GfxImage const &image = *(*it).second;
                    metallicity_map.width = image.width;
                    metallicity_map.height = image.height;
                    metallicity_map.channel_count = 1;
                    metallicity_map.bytes_per_channel = image.bytes_per_channel;
                    metallicity_map.data.resize(metallicity_map.width * metallicity_map.height * metallicity_map.bytes_per_channel);
                    roughness_map.width = image.width;
                    roughness_map.height = image.height;
                    roughness_map.channel_count = 1;
                    roughness_map.bytes_per_channel = image.bytes_per_channel;
                    roughness_map.data.resize(roughness_map.width * roughness_map.height * roughness_map.bytes_per_channel);
                    uint32_t const texel_count = image.width * image.height * image.bytes_per_channel;
                    uint32_t const byte_stride = image.channel_count * image.bytes_per_channel;
                    for(uint32_t j = 0; j < texel_count; ++j)
                    {
                        uint32_t index = j * byte_stride;
                        for(uint32_t k = 0; k < image.bytes_per_channel; ++k)
                            metallicity_map.data[j * image.bytes_per_channel + k] = image.data[index++];
                        for(uint32_t k = 0; k < image.bytes_per_channel; ++k)
                            roughness_map.data[j * image.bytes_per_channel + k] = image.data[index++];
                    }
                    material.roughness_map = roughness_map_ref;
                    material.metallicity_map = metallicity_map_ref;
                    maps[metallicity_roughness_map] =
                        std::pair<GfxConstRef<GfxImage>, GfxConstRef<GfxImage>>(roughness_map_ref, metallicity_map_ref);
                }
            }
            int32_t const emissivity_map = GetTextureIndex(gltf_material, "emissiveTexture");
            it = (emissivity_map >= 0 ? images.find(emissivity_map) : images.end());
            if(it != images.end()) material.emissivity_map = (*it).second;
            int32_t const normal_map = GetTextureIndex(gltf_material, "normalTexture");
            it = (normal_map >= 0 ? images.find(normal_map) : images.end());
            if(it != images.end()) material.normal_map = (*it).second;
            int32_t const ao_map = GetTextureIndex(gltf_material, "occlusionTexture");
            it = (ao_map >= 0 ? images.find(ao_map) : images.end());
            if(it != images.end()) material.ao_map = (*it).second;
            materials[(int32_t)i] = material_ref;
        }
        auto const GetBuffer = [&](int32_t accessor_index)
        {
            GltfBuffer buffer = {};
            if(accessor_index < 0 || accessor_index >= (int32_t)gltf_model.accessors.size())
                return buffer;  // out of bounds
            tinygltf::Accessor const &gltf_accessor = gltf_model.accessors[accessor_index];
            if(gltf_accessor.sparse.isSparse) { GFX_ASSERT(0); return buffer; } // sparse accessors aren't supported yet
            if(tinygltf::GetNumComponentsInType((uint32_t)gltf_accessor.type) < 0 ||
               tinygltf::GetComponentSizeInBytes((uint32_t)gltf_accessor.componentType) < 0)
                return buffer;  // unrecognized data type
            if(gltf_accessor.bufferView < 0 || gltf_accessor.bufferView >= (int32_t)gltf_model.bufferViews.size())
                return buffer;  // out of bounds
            tinygltf::BufferView const &gltf_buffer_view = gltf_model.bufferViews[gltf_accessor.bufferView];
            if(gltf_buffer_view.buffer < 0 || gltf_buffer_view.buffer >= (int32_t)gltf_model.buffers.size())
                return buffer;  // ouf of bounds
            tinygltf::Buffer const &gltf_buffer = gltf_model.buffers[gltf_buffer_view.buffer];
            if(gltf_buffer_view.byteOffset + gltf_buffer_view.byteLength > gltf_buffer.data.size())
                return buffer;  // out of bounds
            if(gltf_accessor.byteOffset +
               tinygltf::GetNumComponentsInType((uint32_t)gltf_accessor.type) *
               tinygltf::GetComponentSizeInBytes((uint32_t)gltf_accessor.componentType) *
               gltf_accessor.count > gltf_buffer_view.byteLength)
                return buffer;  // out of bounds
            buffer.type_ = (uint32_t)gltf_accessor.type;
            buffer.count_ = (uint32_t)gltf_accessor.count;
            buffer.normalize_ = (gltf_accessor.normalized ? 1 : 0);
            buffer.data_ = gltf_buffer.data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
            buffer.stride_ = (uint32_t)gltf_buffer_view.byteStride;
            buffer.component_type_ = (uint32_t)gltf_accessor.componentType;
            if(buffer.stride_ == 0)
            {
                buffer.stride_ = tinygltf::GetNumComponentsInType(buffer.type_)
                               * tinygltf::GetComponentSizeInBytes(buffer.component_type_);
            }
            return buffer;
        };
        std::map<int32_t, std::vector<GfxConstRef<GfxMesh>>> meshes;
        for(size_t i = 0; i < gltf_model.meshes.size(); ++i)
        {
            tinygltf::Mesh const &gltf_mesh = gltf_model.meshes[i];
            std::vector<GfxConstRef<GfxMesh>> &mesh_list = meshes[(int32_t)i];
            for(size_t j = 0; j < gltf_mesh.primitives.size(); ++j)
            {
                std::map<std::string, int>::const_iterator it;
                static std::string const position_attribute = "POSITION";
                static std::string const normal_attribute   = "NORMAL";
                static std::string const uv_attribute       = "TEXCOORD_0";
                tinygltf::Primitive const &gltf_primitive = gltf_mesh.primitives[j];
                if(!gltf_primitive.targets.empty()) continue;   // morph targets aren't supported
                if(gltf_primitive.mode != TINYGLTF_MODE_TRIANGLES) continue;    // only support triangle meshes
                GltfBuffer const index_buffer = GetBuffer(gltf_primitive.indices);
                it = gltf_primitive.attributes.find(position_attribute);    // locate position stream
                GltfBuffer const position_buffer = GetBuffer(it != gltf_primitive.attributes.end() ? (*it).second : -1);
                it = gltf_primitive.attributes.find(normal_attribute);  // locate normal stream
                GltfBuffer const normal_buffer = GetBuffer(it != gltf_primitive.attributes.end() ? (*it).second : -1);
                it = gltf_primitive.attributes.find(uv_attribute);  // locate uv stream
                GltfBuffer const uv_buffer = GetBuffer(it != gltf_primitive.attributes.end() ? (*it).second : -1);
                if(!index_buffer.data_ || !position_buffer.data_) continue; // invalid mesh primitive
                GfxRef<GfxMesh> mesh_ref = gfxSceneCreateMesh(scene);
                mesh_list.push_back(mesh_ref);
                GfxMesh &mesh = *mesh_ref;
                std::map<uint32_t, uint32_t> indices;
                for(uint32_t k = 0; k < index_buffer.count_; ++k)
                {
                    uint32_t const gltf_index = ReadAs<uint32_t>(index_buffer, k);
                    std::map<uint32_t, uint32_t>::const_iterator const it2 = indices.find(gltf_index);
                    if(it2 != indices.end())
                        mesh.indices.push_back((*it2).second);
                    else
                    {
                        GfxVertex vertex = {};
                        vertex.position = ReadAs<glm::vec3>(position_buffer, gltf_index);
                        vertex.normal   = ReadAs<glm::vec3>(normal_buffer, gltf_index);
                        vertex.uv       = ReadAs<glm::vec2>(uv_buffer, gltf_index);
                        uint32_t const index = (uint32_t)mesh.vertices.size();
                        if(index == 0)
                        {
                            mesh.bounds_min = vertex.position;
                            mesh.bounds_max = vertex.position;
                        }
                        else
                        {
                            mesh.bounds_min = glm::min(mesh.bounds_min, vertex.position);
                            mesh.bounds_max = glm::max(mesh.bounds_max, vertex.position);
                        }
                        mesh.vertices.push_back(vertex);
                        mesh.indices.push_back(index);
                        indices[gltf_index] = index;
                    }
                }
                GfxMetadata &mesh_metadata = mesh_metadata_[mesh_ref];
                mesh_metadata.asset_file = asset_file;  // set up metadata
                mesh_metadata.object_name = gltf_mesh.name;
                if(j > 0)
                {
                    mesh_metadata.object_name += ".";
                    mesh_metadata.object_name += std::to_string(j);
                }
                if(gltf_primitive.material >= 0 && gltf_primitive.material < (int32_t)gltf_model.materials.size())
                {
                    std::map<int32_t, GfxConstRef<GfxMaterial>>::const_iterator const it2 = materials.find(gltf_primitive.material);
                    if(it2 != materials.end()) mesh.material = (*it2).second;
                }
            }
        }
        std::map<int32_t, uint64_t> animated_nodes;
        std::map<size_t, GfxConstRef<GfxAnimation>> animations;
        for(size_t i = 0; i < gltf_model.animations.size(); ++i)
        {
            GfxRef<GfxAnimation> animation_ref;
            GltfAnimation *animation_object = nullptr;
            tinygltf::Animation const &gltf_animation = gltf_model.animations[i];
            for(size_t j = 0; j < gltf_animation.channels.size(); ++j)
            {
                uint64_t animated_node_handle;
                tinygltf::AnimationChannel const &gltf_animation_channel = gltf_animation.channels[j];
                if(gltf_animation_channel.target_node < 0 || gltf_animation_channel.target_node >= (int32_t)gltf_model.nodes.size()) continue;
                GltfAnimationChannelType type = kGltfAnimationChannelType_Count;
                     if(gltf_animation_channel.target_path == "translation") type = kGltfAnimationChannelType_Translate;
                else if(gltf_animation_channel.target_path == "rotation")    type = kGltfAnimationChannelType_Rotate;
                else if(gltf_animation_channel.target_path == "scale")       type = kGltfAnimationChannelType_Scale;
                if(type == kGltfAnimationChannelType_Count) continue;   // unsupported animation channel type
                if(gltf_animation_channel.sampler < 0 || gltf_animation_channel.sampler >= (int32_t)gltf_animation.samplers.size()) continue;
                tinygltf::AnimationSampler const &gltf_animation_sampler = gltf_animation.samplers[gltf_animation_channel.sampler];
                GltfAnimationChannelMode mode = kGltfAnimationChannelMode_Count;
                     if(gltf_animation_sampler.interpolation == "LINEAR")      mode = kGltfAnimationChannelMode_Linear;
                else if(gltf_animation_sampler.interpolation == "STEP")        mode = kGltfAnimationChannelMode_Step;
                else if(gltf_animation_sampler.interpolation == "CUBICSPLINE") mode = kGltfAnimationChannelMode_Count;  // not supported yet
                if(mode == kGltfAnimationChannelMode_Count) continue;   // unsupported animation channel mode
                GltfBuffer const input_buffer  = GetBuffer(gltf_animation_sampler.input);
                GltfBuffer const output_buffer = GetBuffer(gltf_animation_sampler.output);
                if(!input_buffer.count_ || !output_buffer.count_ || input_buffer.count_ != output_buffer.count_) continue;
                std::map<int32_t, uint64_t>::const_iterator const it = animated_nodes.find(gltf_animation_channel.target_node);
                if(it != animated_nodes.end())
                    animated_node_handle = (*it).second;
                else
                {
                    animated_node_handle = gltf_node_handles_.allocate_handle();
                    animated_nodes[gltf_animation_channel.target_node] = animated_node_handle;
                    gltf_nodes_.insert(GetObjectIndex(animated_node_handle)) = {};  // flag animated node
                    gltf_animated_nodes_.insert(GetObjectIndex(animated_node_handle)) = {};
                }
                if(!animation_ref)
                {
                    animation_ref = gfxSceneCreateAnimation(scene);
                    animations[i] = animation_ref;  // insert into map
                    animation_object = &gltf_animations_.insert(GetObjectIndex(animation_ref));
                    GfxMetadata &animation_metadata = animation_metadata_[animation_ref];
                    animation_metadata.asset_file = asset_file; // set up metadata
                    animation_metadata.object_name = gltf_animation.name;
                }
                GFX_ASSERT(animation_object != nullptr);
                GltfAnimationChannel &animation_channel = animation_object->channels_.emplace_back();
                animation_channel.keyframes_.resize(input_buffer.count_);
                for(uint32_t k = 0; k < input_buffer.count_; ++k)
                    animation_channel.keyframes_[k] = ReadAs<float>(input_buffer, k);
                animation_channel.values_.resize(output_buffer.count_);
                for(uint32_t k = 0; k < output_buffer.count_; ++k)
                    animation_channel.values_[k]    = ReadAs<glm::vec4>(output_buffer, k);
                animation_channel.node_ = animated_node_handle;
                animation_channel.mode_ = mode;
                animation_channel.type_ = type;
            }
        }
        std::function<bool(int32_t, glm::dmat4 const &, bool)> VisitNode;
        VisitNode = [&](int32_t node_index, glm::dmat4 const &parent_transform, bool is_parent_animated)
        {
            glm::dvec3 T(0.0), S(1.0);
            glm::dquat R(1.0, 0.0, 0.0, 0.0);
            glm::dmat4 local_transform(1.0);    // default to identity
            if(node_index < 0 || node_index >= (int32_t)gltf_model.nodes.size())
                return false;   // out of bounds
            tinygltf::Node const &gltf_node = gltf_model.nodes[node_index];
            if(!gltf_node.matrix.empty())
            {
                for(uint32_t col = 0; col < 4; ++col)
                    for(uint32_t row = 0; row < 4; ++row)
                        if(4 * col + row < gltf_node.matrix.size())
                            local_transform[col][row] = gltf_node.matrix[4 * col + row];
            }
            else
            {
                glm::dmat4 translate(1.0), rotate(1.0), scale(1.0);
                if(!gltf_node.translation.empty())
                {
                    for(uint32_t i = 0; i < 3; ++i)
                        if(i < (uint32_t)gltf_node.translation.size())
                            T[i] = gltf_node.translation[i];
                    translate = glm::translate(glm::dmat4(1.0), T);
                }
                if(!gltf_node.rotation.empty())
                {
                    for(uint32_t i = 0; i < 4; ++i)
                        if(i < (uint32_t)gltf_node.rotation.size())
                            R[i] = gltf_node.rotation[i];
                    rotate = glm::toMat4(R);
                }
                if(!gltf_node.scale.empty())
                {
                    for(uint32_t i = 0; i < 3; ++i)
                        if(i < (uint32_t)gltf_node.scale.size())
                            S[i] = gltf_node.scale[i];
                    scale = glm::scale(glm::dmat4(1.0), S);
                }
                local_transform = translate * rotate * scale;
            }
            std::vector<GfxRef<GfxInstance>> instances;
            glm::dmat4 const transform = parent_transform * local_transform;
            if(gltf_node.mesh >= 0 && gltf_node.weights.empty())
            {
                std::map<int32_t, std::vector<GfxConstRef<GfxMesh>>>::const_iterator const it = meshes.find(gltf_node.mesh);
                if(it != meshes.end())
                    for(size_t i = 0; i < (*it).second.size(); ++i)
                    {
                        GfxRef<GfxInstance> instance_ref = gfxSceneCreateInstance(scene);
                        instances.push_back(instance_ref);
                        instance_ref->mesh = (*it).second[i];
                        instance_ref->transform = glm::mat4(transform);
                        GfxMetadata &instance_metadata = instance_metadata_[instance_ref];
                        GfxMetadata const *mesh_metadata = mesh_metadata_.at((*it).second[i]);
                        if(mesh_metadata != nullptr)
                            instance_metadata = *mesh_metadata; // set up metadata
                        else
                            instance_metadata.asset_file = asset_file;
                    }
            }
            bool is_any_children_animated = false;
            std::map<int32_t, uint64_t>::const_iterator const it = animated_nodes.find(node_index);
            bool is_node_animated = (is_parent_animated || it != animated_nodes.end());
            for(size_t i = 0; i < gltf_node.children.size(); ++i)
                is_any_children_animated = (VisitNode(gltf_node.children[i], transform, is_node_animated) || is_any_children_animated);
            is_node_animated = (is_node_animated || is_any_children_animated);
            if(is_node_animated)
            {
                GltfNode *node = nullptr;
                std::vector<uint64_t> children;
                GltfAnimatedNode *animated_node = nullptr;
                for(size_t i = 0; i < gltf_node.children.size(); ++i)
                {
                    std::map<int32_t, uint64_t>::const_iterator const it2 = animated_nodes.find(gltf_node.children[i]);
                    if(it2 != animated_nodes.end()) children.push_back((*it2).second);
                }
                if(it != animated_nodes.end())
                {
                    node = &gltf_nodes_[GetObjectIndex((*it).second)];
                    animated_node = gltf_animated_nodes_.at(GetObjectIndex((*it).second));
                    gltf_node_refs_.insert(GetObjectIndex((*it).second), (uint32_t)animations.size());
                }
                else
                {
                    uint64_t const animated_node_handle = gltf_node_handles_.allocate_handle();
                    gltf_node_refs_.insert(GetObjectIndex(animated_node_handle), (uint32_t)animations.size());
                    node = &gltf_nodes_.insert(GetObjectIndex(animated_node_handle));
                    animated_nodes[node_index] = animated_node_handle;
                    *node = {};
                }
                if(animated_node != nullptr)
                {
                    animated_node->scale_ = S;
                    animated_node->rotate_ = R;
                    animated_node->translate_ = T;
                }
                GFX_ASSERT(node != nullptr);
                node->scale_ = S;
                node->rotate_ = R;
                node->translate_ = T;
                node->matrix_ = local_transform;
                std::swap(node->children_, children);
                std::swap(node->instances_, instances);
            }
            return is_node_animated;
        };
        tinygltf::Scene const &gltf_scene = gltf_model.scenes[glm::clamp(gltf_model.defaultScene, 0, (int32_t)gltf_model.scenes.size() - 1)];
        for(size_t i = 0; i < gltf_scene.nodes.size(); ++i)
            VisitNode(gltf_scene.nodes[i], glm::dmat4(1.0), false);
        for(size_t i = 0; i < gltf_model.animations.size(); ++i)
        {
            std::map<size_t, GfxConstRef<GfxAnimation>>::const_iterator const it = animations.find(i);
            if(it == animations.end()) continue;    // not a valid animation
            GltfAnimation &animation_object = gltf_animations_[GetObjectIndex((*it).second)];
            for(size_t j = 0; j < gltf_scene.nodes.size(); ++j)
            {
                std::map<int32_t, uint64_t>::const_iterator const it2 = animated_nodes.find(gltf_scene.nodes[j]);
                if(it2 != animated_nodes.end())
                    animation_object.nodes_.push_back((*it2).second);
            }
        }
        return kGfxResult_NoError;
    }

    GfxResult importHdr(GfxScene const &scene, char const *asset_file)
    {
        GFX_ASSERT(asset_file != nullptr);
        int32_t image_width, image_height, channel_count;
        if(gfxSceneFindObjectByAssetFile<GfxImage>(scene, asset_file))
            return kGfxResult_NoError;  // image was already imported
        stbi_us *image_data = stbi_load_16(asset_file, &image_width, &image_height, &channel_count, 0);
        if(image_data == nullptr)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Unable to load image `%s': %s", asset_file, stbi_failure_reason());
        uint32_t const resolved_channel_count = (uint32_t)(channel_count != 3 ? channel_count : 4);
        char const *file = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '\\'));
        file = (file == nullptr ? asset_file : file + 1);   // retrieve file name
        size_t const image_data_size = (size_t)image_width * image_height * resolved_channel_count * sizeof(stbi_us);
        GfxRef<GfxImage> image_ref = gfxSceneCreateImage(scene);
        image_ref->data.resize(image_data_size);
        image_ref->width = (uint32_t)image_width;
        image_ref->height = (uint32_t)image_height;
        image_ref->channel_count = resolved_channel_count;
        image_ref->bytes_per_channel = (uint32_t)2;
        uint16_t *data = (uint16_t *)image_ref->data.data();
        for(int32_t y = 0; y < image_height; ++y)
            for(int32_t x = 0; x < image_width; ++x)
                for(int32_t k = 0; k < (int32_t)resolved_channel_count; ++k)
                {
                    int32_t const dst_index = (int32_t)resolved_channel_count * (x + (image_height - y - 1) * image_width) + k;
                    int32_t const src_index = channel_count * (x + y * image_width) + k;
                    data[dst_index] = (k < channel_count ? image_data[src_index] : (uint16_t)65535);
                }
        GfxMetadata &image_metadata = image_metadata_[image_ref];
        image_metadata.asset_file = asset_file; // set up metadata
        image_metadata.object_name = file;
        stbi_image_free(image_data);
        return kGfxResult_NoError;
    }

    GfxResult importImage(GfxScene const &scene, char const *asset_file)
    {
        GFX_ASSERT(asset_file != nullptr);
        int32_t image_width, image_height, channel_count;
        if(gfxSceneFindObjectByAssetFile<GfxImage>(scene, asset_file))
            return kGfxResult_NoError;  // image was already imported
        stbi_uc *image_data = stbi_load(asset_file, &image_width, &image_height, &channel_count, 0);
        if(image_data == nullptr)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Unable to load image `%s': %s", asset_file, stbi_failure_reason());
        uint32_t const resolved_channel_count = (uint32_t)(channel_count != 3 ? channel_count : 4);
        char const *file = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '\\'));
        file = (file == nullptr ? asset_file : file + 1);   // retrieve file name
        size_t const image_data_size = (size_t)image_width * image_height * resolved_channel_count;
        GfxRef<GfxImage> image_ref = gfxSceneCreateImage(scene);
        image_ref->data.resize(image_data_size);
        image_ref->width = (uint32_t)image_width;
        image_ref->height = (uint32_t)image_height;
        image_ref->channel_count = resolved_channel_count;
        image_ref->bytes_per_channel = (uint32_t)1;
        for(int32_t y = 0; y < image_height; ++y)
            for(int32_t x = 0; x < image_width; ++x)
                for(int32_t k = 0; k < (int32_t)resolved_channel_count; ++k)
                {
                    int32_t const dst_index = (int32_t)resolved_channel_count * (x + (image_height - y - 1) * image_width) + k;
                    int32_t const src_index = channel_count * (x + y * image_width) + k;
                    image_ref->data[dst_index] = (k < channel_count ? image_data[src_index] : (uint8_t)255);
                }
        GfxMetadata &image_metadata = image_metadata_[image_ref];
        image_metadata.asset_file = asset_file; // set up metadata
        image_metadata.object_name = file;
        stbi_image_free(image_data);
        return kGfxResult_NoError;
    }
};

GfxArray<GfxScene> GfxSceneInternal::scenes_;
GfxHandles         GfxSceneInternal::scene_handles_("scene");

template<typename TYPE>
GfxResult GfxSceneInternal::destroyObjectCallback(uint64_t object_handle)
{
    (void)object_handle;

    return kGfxResult_NoError;
}

GfxScene gfxCreateScene()
{
    GfxResult result;
    GfxScene scene = {};
    GfxSceneInternal *gfx_scene = new GfxSceneInternal(scene);
    if(!gfx_scene) return scene;    // out of memory
    result = gfx_scene->initialize();
    if(result != kGfxResult_NoError)
    {
        scene = {};
        delete gfx_scene;
        GFX_PRINT_ERROR(result, "Failed to create new scene");
    }
    return scene;
}

GfxResult gfxDestroyScene(GfxScene scene)
{
    delete GfxSceneInternal::GetGfxScene(scene);
    return kGfxResult_NoError;
}

GfxResult gfxSceneImport(GfxScene scene, char const *asset_file)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->import(scene, asset_file);
}

GfxResult gfxSceneClear(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->clear();
}

GfxRef<GfxAnimation> gfxSceneCreateAnimation(GfxScene scene)
{
    GfxRef<GfxAnimation> const animation_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return animation_ref;    // invalid parameter
    return gfx_scene->createObject<GfxAnimation>(scene);
}

GfxResult gfxSceneDestroyAnimation(GfxScene scene, uint64_t animation_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->destroyObject<GfxAnimation>(animation_handle);
}

GfxResult gfxSceneApplyAnimation(GfxScene scene, uint64_t animation_handle, float time_in_seconds)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->applyAnimation(animation_handle, time_in_seconds);
}

GfxResult gfxSceneResetAllAnimation(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->resetAllAnimation();
}

float gfxSceneGetAnimationLength(GfxScene scene, uint64_t animation_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0.0f; // invalid parameter
    return gfx_scene->getAnimationLength(animation_handle);
}

float gfxSceneGetAnimationStart(GfxScene scene, uint64_t animation_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0.0f; // invalid parameter
    return gfx_scene->getAnimationStart(animation_handle);
}

float gfxSceneGetAnimationEnd(GfxScene scene, uint64_t animation_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0.0f; // invalid parameter
    return gfx_scene->getAnimationEnd(animation_handle);
}

uint32_t gfxSceneGetAnimationCount(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0;    // invalid parameter
    return gfx_scene->getObjectCount<GfxAnimation>();
}

GfxAnimation const *gfxSceneGetAnimations(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObjects<GfxAnimation>();
}

GfxAnimation *gfxSceneGetAnimation(GfxScene scene, uint64_t animation_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObject<GfxAnimation>(animation_handle);
}

GfxRef<GfxAnimation> gfxSceneGetAnimationHandle(GfxScene scene, uint32_t animation_index)
{
    GfxRef<GfxAnimation> const animation_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return animation_ref;    // invalid parameter
    return gfx_scene->getObjectHandle<GfxAnimation>(scene, animation_index);
}

GfxMetadata const &gfxSceneGetAnimationMetadata(GfxScene scene, uint64_t animation_handle)
{
    static GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxAnimation>(animation_handle);
}

GfxRef<GfxCamera> gfxSceneCreateCamera(GfxScene scene)
{
    GfxRef<GfxCamera> const camera_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return camera_ref;   // invalid parameter
    return gfx_scene->createObject<GfxCamera>(scene);
}

GfxResult gfxSceneDestroyCamera(GfxScene scene, uint64_t camera_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->destroyObject<GfxCamera>(camera_handle);
}

GfxResult gfxSceneSetActiveCamera(GfxScene scene, uint64_t camera_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->setActiveCamera(scene, camera_handle);
}

GfxConstRef<GfxCamera> gfxSceneGetActiveCamera(GfxScene scene)
{
    GfxConstRef<GfxCamera> const camera_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return camera_ref;   // invalid parameter
    return gfx_scene->getActiveCamera();
}

uint32_t gfxSceneGetCameraCount(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0;    // invalid parameter
    return gfx_scene->getObjectCount<GfxCamera>();
}

GfxCamera const *gfxSceneGetCameras(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObjects<GfxCamera>();
}

GfxCamera *gfxSceneGetCamera(GfxScene scene, uint64_t camera_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObject<GfxCamera>(camera_handle);
}

GfxRef<GfxCamera> gfxSceneGetCameraHandle(GfxScene scene, uint32_t camera_index)
{
    GfxRef<GfxCamera> const camera_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return camera_ref;   // invalid parameter
    return gfx_scene->getObjectHandle<GfxCamera>(scene, camera_index);
}

GfxMetadata const &gfxSceneGetCameraMetadata(GfxScene scene, uint64_t camera_handle)
{
    static GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxCamera>(camera_handle);
}

GfxRef<GfxImage> gfxSceneCreateImage(GfxScene scene)
{
    GfxRef<GfxImage> const image_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return image_ref;    // invalid parameter
    return gfx_scene->createObject<GfxImage>(scene);
}

GfxResult gfxSceneDestroyImage(GfxScene scene, uint64_t image_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->destroyObject<GfxImage>(image_handle);
}

uint32_t gfxSceneGetImageCount(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0;    // invalid parameter
    return gfx_scene->getObjectCount<GfxImage>();
}

GfxImage const *gfxSceneGetImages(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObjects<GfxImage>();
}

GfxImage *gfxSceneGetImage(GfxScene scene, uint64_t image_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObject<GfxImage>(image_handle);
}

GfxRef<GfxImage> gfxSceneGetImageHandle(GfxScene scene, uint32_t image_index)
{
    GfxRef<GfxImage> const image_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return image_ref;    // invalid parameter
    return gfx_scene->getObjectHandle<GfxImage>(scene, image_index);
}

GfxMetadata const &gfxSceneGetImageMetadata(GfxScene scene, uint64_t image_handle)
{
    static GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxImage>(image_handle);
}

GfxRef<GfxMaterial> gfxSceneCreateMaterial(GfxScene scene)
{
    GfxRef<GfxMaterial> const material_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return material_ref; // invalid parameter
    return gfx_scene->createObject<GfxMaterial>(scene);
}

GfxResult gfxSceneDestroyMaterial(GfxScene scene, uint64_t material_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->destroyObject<GfxMaterial>(material_handle);
}

uint32_t gfxSceneGetMaterialCount(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0;    // invalid parameter
    return gfx_scene->getObjectCount<GfxMaterial>();
}

GfxMaterial const *gfxSceneGetMaterials(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObjects<GfxMaterial>();
}

GfxMaterial *gfxSceneGetMaterial(GfxScene scene, uint64_t material_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObject<GfxMaterial>(material_handle);
}

GfxRef<GfxMaterial> gfxSceneGetMaterialHandle(GfxScene scene, uint32_t material_index)
{
    GfxRef<GfxMaterial> const material_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return material_ref; // invalid parameter
    return gfx_scene->getObjectHandle<GfxMaterial>(scene, material_index);
}

GfxMetadata const &gfxSceneGetMaterialMetadata(GfxScene scene, uint64_t material_handle)
{
    static GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxMaterial>(material_handle);
}

GfxRef<GfxMesh> gfxSceneCreateMesh(GfxScene scene)
{
    GfxRef<GfxMesh> const mesh_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return mesh_ref; // invalid parameter
    return gfx_scene->createObject<GfxMesh>(scene);
}

GfxResult gfxSceneDestroyMesh(GfxScene scene, uint64_t mesh_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->destroyObject<GfxMesh>(mesh_handle);
}

uint32_t gfxSceneGetMeshCount(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0;    // invalid parameter
    return gfx_scene->getObjectCount<GfxMesh>();
}

GfxMesh const *gfxSceneGetMeshes(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObjects<GfxMesh>();
}

GfxMesh *gfxSceneGetMesh(GfxScene scene, uint64_t mesh_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObject<GfxMesh>(mesh_handle);
}

GfxRef<GfxMesh> gfxSceneGetMeshHandle(GfxScene scene, uint32_t mesh_index)
{
    GfxRef<GfxMesh> const mesh_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return mesh_ref; // invalid parameter
    return gfx_scene->getObjectHandle<GfxMesh>(scene, mesh_index);
}

GfxMetadata const &gfxSceneGetMeshMetadata(GfxScene scene, uint64_t mesh_handle)
{
    static GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxMesh>(mesh_handle);
}

GfxRef<GfxInstance> gfxSceneCreateInstance(GfxScene scene)
{
    GfxRef<GfxInstance> const instance_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return instance_ref; // invalid parameter
    return gfx_scene->createObject<GfxInstance>(scene);
}

GfxResult gfxSceneDestroyInstance(GfxScene scene, uint64_t instance_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->destroyObject<GfxInstance>(instance_handle);
}

uint32_t gfxSceneGetInstanceCount(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0;    // invalid parameter
    return gfx_scene->getObjectCount<GfxInstance>();
}

GfxInstance const *gfxSceneGetInstances(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObjects<GfxInstance>();
}

GfxInstance *gfxSceneGetInstance(GfxScene scene, uint64_t instance_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObject<GfxInstance>(instance_handle);
}

GfxRef<GfxInstance> gfxSceneGetInstanceHandle(GfxScene scene, uint32_t instance_index)
{
    GfxRef<GfxInstance> const instance_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return instance_ref; // invalid parameter
    return gfx_scene->getObjectHandle<GfxInstance>(scene, instance_index);
}

GfxMetadata const &gfxSceneGetInstanceMetadata(GfxScene scene, uint64_t instance_handle)
{
    static GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxInstance>(instance_handle);
}

#endif //! GFX_IMPLEMENTATION_DEFINE
