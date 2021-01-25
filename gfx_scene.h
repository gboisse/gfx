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

template<typename TYPE> class GfxConstRef;

//!
//! Scene creation/destruction.
//!

class GfxScene { friend class GfxSceneInternal; uint64_t handle; public:
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
template<typename TYPE> GfxConstRef<TYPE> gfxSceneGetObjectHandle(GfxScene scene, uint32_t object_index);

//!
//! Ref. primitives.
//!

template<typename TYPE, typename OBJECT_TYPE>
class GfxRefBase { friend class GfxSceneInternal; protected: uint64_t handle; GfxScene scene; public: GfxRefBase() : handle(0), scene{} {}
                   uint32_t getIndex() const { return (uint32_t)(handle ? handle & 0xFFFFFFFFull : 0xFFFFFFFFull); }
                   OBJECT_TYPE *operator ->() const { return gfxSceneGetObject<TYPE>(scene, handle); }
                   OBJECT_TYPE &operator *() const { return *operator ->(); }
                   operator bool() const { return !!operator ->(); }
                   operator uint32_t() const { return getIndex(); }
                   operator uint64_t() const { return handle; } };

template<typename TYPE>
class GfxRef : public GfxRefBase<TYPE, TYPE> { friend class GfxConstRef<TYPE>; };

template<typename TYPE>
class GfxConstRef : public GfxRefBase<TYPE, TYPE const> { public: GfxConstRef() {} GfxConstRef(GfxRef<TYPE> const &other) { handle = other.handle; scene = other.scene; } };

//!
//! Metadata API.
//!

class GfxMetadata { friend class GfxSceneInternal; bool is_valid; std::string asset_file; std::string object_name; public:
                    inline char const *getAssetFile() const { return asset_file.c_str(); }
                    inline char const *getObjectName() const { return object_name.c_str(); }
                    inline bool getIsValid() const { return is_valid; }
                    inline operator bool() const { return is_valid; } };

template<typename TYPE> GfxMetadata gfxSceneGetObjectMetadata(GfxScene scene, uint64_t object_handle);

//!
//! Search API.
//!

template<typename TYPE>
GfxConstRef<TYPE> gfxSceneFindObjectByAssetFile(GfxScene scene, char const *asset_file)
{
    GfxConstRef<TYPE> const object_ref = {};
    if(asset_file == nullptr || *asset_file == '\0')
        return object_ref;  // invalid parameter
    uint32_t const object_count = gfxSceneGetObjectCount<TYPE>(scene);
    for(uint32_t i = 0; i < object_count; ++i)
    {
        GfxConstRef<TYPE> const object_handle = gfxSceneGetObjectHandle<TYPE>(scene, i);
        if(!strcmp(gfxSceneGetObjectMetadata<TYPE>(scene, object_handle).getAssetFile(), asset_file))
            return object_handle;   // found
    }
    return object_ref;  // not found
}

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
GfxMetadata gfxSceneGetCameraMetadata(GfxScene scene, uint64_t camera_handle);
GfxConstRef<GfxCamera> gfxSceneGetCameraHandle(GfxScene scene, uint32_t camera_index);

//!
//! Image object.
//!

struct GfxImage
{
    uint32_t width         = 0;
    uint32_t height        = 0;
    uint32_t channel_count = 0;

    std::vector<uint8_t> data;
};

GfxRef<GfxImage> gfxSceneCreateImage(GfxScene scene);
GfxResult gfxSceneDestroyImage(GfxScene scene, uint64_t image_handle);

uint32_t gfxSceneGetImageCount(GfxScene scene);
GfxImage const *gfxSceneGetImages(GfxScene scene);
GfxImage *gfxSceneGetImage(GfxScene scene, uint64_t image_handle);
GfxMetadata gfxSceneGetImageMetadata(GfxScene scene, uint64_t image_handle);
GfxConstRef<GfxImage> gfxSceneGetImageHandle(GfxScene scene, uint32_t image_index);

//!
//! Material object.
//!

struct GfxMaterial
{
    glm::vec3 albedo      = glm::vec3(0.7f);
    float     roughness   = 1.0f;
    float     metallicity = 0.0f;
    glm::vec3 emissivity  = glm::vec3(0.0f);

    GfxConstRef<GfxImage> albedo_map;
    GfxConstRef<GfxImage> roughness_map;
    GfxConstRef<GfxImage> metallicity_map;
    GfxConstRef<GfxImage> emissivity_map;
};

GfxRef<GfxMaterial> gfxSceneCreateMaterial(GfxScene scene);
GfxResult gfxSceneDestroyMaterial(GfxScene scene, uint64_t material_handle);

uint32_t gfxSceneGetMaterialCount(GfxScene scene);
GfxMaterial const *gfxSceneGetMaterials(GfxScene scene);
GfxMaterial *gfxSceneGetMaterial(GfxScene scene, uint64_t material_handle);
GfxMetadata gfxSceneGetMaterialMetadata(GfxScene scene, uint64_t material_handle);
GfxConstRef<GfxMaterial> gfxSceneGetMaterialHandle(GfxScene scene, uint32_t material_index);

//!
//! Mesh object.
//!

struct GfxVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
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
GfxMetadata gfxSceneGetMeshMetadata(GfxScene scene, uint64_t mesh_handle);
GfxConstRef<GfxMesh> gfxSceneGetMeshHandle(GfxScene scene, uint32_t mesh_index);

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
GfxMetadata gfxSceneGetInstanceMetadata(GfxScene scene, uint64_t instance_handle);
GfxConstRef<GfxInstance> gfxSceneGetInstanceHandle(GfxScene scene, uint32_t instance_index);

//!
//! Template specializations.
//!

template<> inline uint32_t gfxSceneGetObjectCount<GfxCamera>(GfxScene scene) { return gfxSceneGetCameraCount(scene); }
template<> inline GfxCamera const *gfxSceneGetObjects<GfxCamera>(GfxScene scene) { return gfxSceneGetCameras(scene); }
template<> inline GfxCamera *gfxSceneGetObject<GfxCamera>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetCamera(scene, object_handle); }
template<> inline GfxMetadata gfxSceneGetObjectMetadata<GfxCamera>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetCameraMetadata(scene, object_handle); }
template<> inline GfxConstRef<GfxCamera> gfxSceneGetObjectHandle<GfxCamera>(GfxScene scene, uint32_t object_index) { return gfxSceneGetCameraHandle(scene, object_index); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxImage>(GfxScene scene) { return gfxSceneGetImageCount(scene); }
template<> inline GfxImage const *gfxSceneGetObjects<GfxImage>(GfxScene scene) { return gfxSceneGetImages(scene); }
template<> inline GfxImage *gfxSceneGetObject<GfxImage>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetImage(scene, object_handle); }
template<> inline GfxMetadata gfxSceneGetObjectMetadata<GfxImage>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetImageMetadata(scene, object_handle); }
template<> inline GfxConstRef<GfxImage> gfxSceneGetObjectHandle<GfxImage>(GfxScene scene, uint32_t object_index) { return gfxSceneGetImageHandle(scene, object_index); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxMaterial>(GfxScene scene) { return gfxSceneGetMaterialCount(scene); }
template<> inline GfxMaterial const *gfxSceneGetObjects<GfxMaterial>(GfxScene scene) { return gfxSceneGetMaterials(scene); }
template<> inline GfxMaterial *gfxSceneGetObject<GfxMaterial>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMaterial(scene, object_handle); }
template<> inline GfxMetadata gfxSceneGetObjectMetadata<GfxMaterial>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMaterialMetadata(scene, object_handle); }
template<> inline GfxConstRef<GfxMaterial> gfxSceneGetObjectHandle<GfxMaterial>(GfxScene scene, uint32_t object_index) { return gfxSceneGetMaterialHandle(scene, object_index); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxMesh>(GfxScene scene) { return gfxSceneGetMeshCount(scene); }
template<> inline GfxMesh const *gfxSceneGetObjects<GfxMesh>(GfxScene scene) { return gfxSceneGetMeshes(scene); }
template<> inline GfxMesh *gfxSceneGetObject<GfxMesh>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMesh(scene, object_handle); }
template<> inline GfxMetadata gfxSceneGetObjectMetadata<GfxMesh>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetMeshMetadata(scene, object_handle); }
template<> inline GfxConstRef<GfxMesh> gfxSceneGetObjectHandle<GfxMesh>(GfxScene scene, uint32_t object_index) { return gfxSceneGetMeshHandle(scene, object_index); }

template<> inline uint32_t gfxSceneGetObjectCount<GfxInstance>(GfxScene scene) { return gfxSceneGetInstanceCount(scene); }
template<> inline GfxInstance const *gfxSceneGetObjects<GfxInstance>(GfxScene scene) { return gfxSceneGetInstances(scene); }
template<> inline GfxInstance *gfxSceneGetObject<GfxInstance>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetInstance(scene, object_handle); }
template<> inline GfxMetadata gfxSceneGetObjectMetadata<GfxInstance>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetInstanceMetadata(scene, object_handle); }
template<> inline GfxConstRef<GfxInstance> gfxSceneGetObjectHandle<GfxInstance>(GfxScene scene, uint32_t object_index) { return gfxSceneGetInstanceHandle(scene, object_index); }

template<typename TYPE> uint32_t gfxSceneGetObjectCount(GfxScene scene) { static_assert("Cannot get object count for unsupported object type"); }
template<typename TYPE> TYPE const *gfxSceneGetObjects(GfxScene scene) { static_assert("Cannot get object list for unsupported object type"); }
template<typename TYPE> TYPE *gfxSceneGetObject(GfxScene scene, uint64_t object_handle) { static_assert("Cannot get scene object for unsupported object type"); }
template<typename TYPE> GfxMetadata gfxSceneGetObjectMetadata(GfxScene scene, uint64_t object_handle) { static_assert("Cannot get object metadata for unsupported object type"); }
template<typename TYPE> GfxConstRef<TYPE> gfxSceneGetObjectHandle(GfxScene scene, uint32_t object_index) { static_assert("Cannot get object handle for unsupported object type"); }

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

class GfxSceneInternal
{
    GFX_NON_COPYABLE(GfxSceneInternal);

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
    GfxSceneInternal(GfxScene &scene) : camera_handles_("camera"), image_handles_("image")
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
        clearObjects<GfxCamera>();
        clearObjects<GfxImage>();
        clearObjects<GfxMaterial>();
        clearObjects<GfxMesh>();
        clearObjects<GfxInstance>();

        return kGfxResult_NoError;
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
    GfxResult destroyObject(uint64_t object_handle)
    {
        if(object_handle == 0)
            return kGfxResult_NoError;
        if(!object_handles_<TYPE>().has_handle(object_handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot destroy invalid scene object");
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
    GfxMetadata getObjectMetadata(uint64_t object_handle)
    {
        GfxMetadata const metadata = {};
        if(!object_handles_<TYPE>().has_handle(object_handle))
            return metadata;    // invalid object handle
        return object_metadata_<TYPE>()[GetObjectIndex(object_handle)];
    }

    template<typename TYPE>
    GfxConstRef<TYPE> getObjectHandle(GfxScene const &scene, uint32_t object_index)
    {
        GfxConstRef<TYPE> object_ref = {};
        if(object_index >= object_refs_<TYPE>().size())
            return object_ref;  // out of bounds
        object_ref.handle = object_refs_<TYPE>()[object_refs_<TYPE>().get_index(object_index)];
        object_ref.scene = scene;
        return object_ref;
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
            GFX_PRINTLN("Parse obj file `%s' with warnings:\r\n%s", asset_file, obj_reader.Warning().c_str());
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
            material_ref->albedo = glm::vec3(obj_material.diffuse[0], obj_material.diffuse[1], obj_material.diffuse[2]);
            material_ref->roughness = obj_material.roughness;
            material_ref->metallicity = obj_material.metallic;
            material_ref->emissivity = glm::vec3(obj_material.emission[0], obj_material.emission[1], obj_material.emission[2]);
            LoadImage(obj_material.diffuse_texname, material_ref->albedo_map);
            LoadImage(obj_material.roughness_texname, material_ref->roughness_map);
            LoadImage(obj_material.metallic_texname, material_ref->metallicity_map);
            LoadImage(obj_material.emissive_texname, material_ref->emissivity_map);
            materials[i] = material_ref;    // append the new material
        }
        for(size_t i = 0; i < obj_reader.GetShapes().size(); ++i)
        {
            uint32_t first_index = 0;
            tinyobj::shape_t const &obj_shape = obj_reader.GetShapes()[i];
            if(obj_shape.mesh.indices.empty()) continue;    // only support meshes for now
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

GfxMetadata gfxSceneGetCameraMetadata(GfxScene scene, uint64_t camera_handle)
{
    GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxCamera>(camera_handle);
}

GfxConstRef<GfxCamera> gfxSceneGetCameraHandle(GfxScene scene, uint32_t camera_index)
{
    GfxConstRef<GfxCamera> const camera_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return camera_ref;   // invalid parameter
    return gfx_scene->getObjectHandle<GfxCamera>(scene, camera_index);
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

GfxMetadata gfxSceneGetImageMetadata(GfxScene scene, uint64_t image_handle)
{
    GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxImage>(image_handle);
}

GfxConstRef<GfxImage> gfxSceneGetImageHandle(GfxScene scene, uint32_t image_index)
{
    GfxConstRef<GfxImage> const image_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return image_ref;    // invalid parameter
    return gfx_scene->getObjectHandle<GfxImage>(scene, image_index);
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

GfxMetadata gfxSceneGetMaterialMetadata(GfxScene scene, uint64_t material_handle)
{
    GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxMaterial>(material_handle);
}

GfxConstRef<GfxMaterial> gfxSceneGetMaterialHandle(GfxScene scene, uint32_t material_index)
{
    GfxConstRef<GfxMaterial> const material_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return material_ref; // invalid parameter
    return gfx_scene->getObjectHandle<GfxMaterial>(scene, material_index);
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

GfxMetadata gfxSceneGetMeshMetadata(GfxScene scene, uint64_t mesh_handle)
{
    GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxMesh>(mesh_handle);
}

GfxConstRef<GfxMesh> gfxSceneGetMeshHandle(GfxScene scene, uint32_t mesh_index)
{
    GfxConstRef<GfxMesh> const mesh_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return mesh_ref; // invalid parameter
    return gfx_scene->getObjectHandle<GfxMesh>(scene, mesh_index);
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

GfxMetadata gfxSceneGetInstanceMetadata(GfxScene scene, uint64_t instance_handle)
{
    GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxInstance>(instance_handle);
}

GfxConstRef<GfxInstance> gfxSceneGetInstanceHandle(GfxScene scene, uint32_t instance_index)
{
    GfxConstRef<GfxInstance> const instance_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return instance_ref; // invalid parameter
    return gfx_scene->getObjectHandle<GfxInstance>(scene, instance_index);
}

#endif //! GFX_IMPLEMENTATION_DEFINE
