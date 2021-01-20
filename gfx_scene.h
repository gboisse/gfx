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
        if(gfxSceneGetObjectMetadata(scene, object_handle).asset_file == asset_file)
            return object_handle;   // found
    }
    return object_ref;  // not found
}

//!
//! Texture map object.
//!

struct GfxTextureMap
{
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> data;
};

GfxRef<GfxTextureMap> gfxSceneCreateTextureMap(GfxScene scene);
GfxResult gfxSceneDestroyTextureMap(GfxScene scene, uint64_t texture_map_handle);

uint32_t gfxSceneGetTextureMapCount(GfxScene scene);
GfxTextureMap const *gfxSceneGetTextureMaps(GfxScene scene);
GfxTextureMap *gfxSceneGetTextureMap(GfxScene scene, uint64_t texture_map_handle);
GfxMetadata gfxSceneGetTextureMapMetadata(GfxScene scene, uint64_t texture_map_handle);
GfxConstRef<GfxTextureMap> gfxSceneGetTextureMapHandle(GfxScene scene, uint32_t texture_map_index);

//!
//! Material object.
//!

struct GfxMaterial
{
    glm::vec3 albedo;
    float roughness;
    float metallicity;
    glm::vec3 emissivity;

    GfxConstRef<GfxTextureMap> albedo_map;
    GfxConstRef<GfxTextureMap> roughness_map;
    GfxConstRef<GfxTextureMap> metallicity_map;
    GfxConstRef<GfxTextureMap> emissivity_map;
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
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec2 uv;
};

struct GfxMesh
{
    GfxConstRef<GfxMaterial> material;
    std::vector<GfxVertex> vertices;
    std::vector<uint32_t> indices;
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
    glm::mat4 transform;
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

template<> inline uint32_t gfxSceneGetObjectCount<GfxTextureMap>(GfxScene scene) { return gfxSceneGetTextureMapCount(scene); }
template<> inline GfxTextureMap const *gfxSceneGetObjects<GfxTextureMap>(GfxScene scene) { return gfxSceneGetTextureMaps(scene); }
template<> inline GfxTextureMap *gfxSceneGetObject<GfxTextureMap>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetTextureMap(scene, object_handle); }
template<> inline GfxMetadata gfxSceneGetObjectMetadata<GfxTextureMap>(GfxScene scene, uint64_t object_handle) { return gfxSceneGetTextureMapMetadata(scene, object_handle); }
template<> inline GfxConstRef<GfxTextureMap> gfxSceneGetObjectHandle<GfxTextureMap>(GfxScene scene, uint32_t object_index) { return gfxSceneGetTextureMapHandle(scene, object_index); }

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
#include "tiny_gltf.h"          // glTF loader
#include "tiny_obj_loader.cc"   // obj loader

class GfxSceneInternal
{
    GFX_NON_COPYABLE(GfxSceneInternal);

    GfxArray<GfxTextureMap> texture_maps_;
    GfxArray<uint64_t> texture_map_refs_;
    GfxArray<GfxMetadata> texture_map_metadata_;
    GfxHandles texture_map_handles_;

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

    template<> inline GfxArray<GfxTextureMap> &objects_<GfxTextureMap>() { return texture_maps_; }
    template<> inline GfxArray<uint64_t> &object_refs_<GfxTextureMap>() { return texture_map_refs_; }
    template<> inline GfxArray<GfxMetadata> &object_metadata_<GfxTextureMap>() { return texture_map_metadata_; }
    template<> inline GfxHandles &object_handles_<GfxTextureMap>() { return texture_map_handles_; }

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
    GfxSceneInternal(GfxScene &scene) : texture_map_handles_("texture map"), material_handles_("material"), mesh_handles_("mesh"), instance_handles_("instance")
                                      { scene.handle = reinterpret_cast<uint64_t>(this); }
    ~GfxSceneInternal() { terminate(); }

    GfxResult initialize()
    {
        return kGfxResult_NoError;
    }

    GfxResult terminate()
    {
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
        else
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot import unsupported asset type `%s'", asset_extension);
        return kGfxResult_NoError;
    }

    GfxResult clear()
    {
        clearObjects<GfxTextureMap>();
        clearObjects<GfxMaterial>();
        clearObjects<GfxMesh>();
        clearObjects<GfxInstance>();
        return kGfxResult_NoError;
    }

    template<typename TYPE>
    GfxRef<TYPE> createObject(GfxScene const &scene)
    {
        GfxRef<TYPE> object_ref = {};
        object_ref.handle = object_handles_<TYPE>().allocate_handle();
        object_refs_<TYPE>().insert(GetObjectIndex(object_ref.handle), object_ref.handle);
        object_metadata_<TYPE>().insert(GetObjectIndex(object_ref.handle)) = {};
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
            GFX_TRY(destroyObject<TYPE>(object_refs_<TYPE>()[0]));
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
            // TODO: load the texture maps (gboisse)
            materials[i] = material_ref;
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
                            vertex.position.w = 1.0f;
                        }
                        if(std::get<1>(key) >= 0)
                        {
                            vertex.normal.x = obj_reader.GetAttrib().normals[3 * std::get<1>(key) + 0];
                            vertex.normal.y = obj_reader.GetAttrib().normals[3 * std::get<1>(key) + 1];
                            vertex.normal.z = obj_reader.GetAttrib().normals[3 * std::get<1>(key) + 2];
                            vertex.normal.w = 0.0f;
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
                for(VertexMap::const_iterator it2 = vertices.begin(); it2 != vertices.end(); ++it2)
                    mesh_ref->vertices[(*it2).second.first] = (*it2).second.second;
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

GfxRef<GfxTextureMap> gfxSceneCreateTextureMap(GfxScene scene)
{
    GfxRef<GfxTextureMap> const texture_map_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return texture_map_ref;  // invalid parameter
    return gfx_scene->createObject<GfxTextureMap>(scene);
}

GfxResult gfxSceneDestroyTextureMap(GfxScene scene, uint64_t texture_map_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->destroyObject<GfxTextureMap>(texture_map_handle);
}

uint32_t gfxSceneGetTextureMapCount(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0;    // invalid parameter
    return gfx_scene->getObjectCount<GfxTextureMap>();
}

GfxTextureMap const *gfxSceneGetTextureMaps(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObjects<GfxTextureMap>();
}

GfxTextureMap *gfxSceneGetTextureMap(GfxScene scene, uint64_t texture_map_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObject<GfxTextureMap>(texture_map_handle);
}

GfxMetadata gfxSceneGetTextureMapMetadata(GfxScene scene, uint64_t texture_map_handle)
{
    GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxTextureMap>(texture_map_handle);
}

GfxConstRef<GfxTextureMap> gfxSceneGetTextureMapHandle(GfxScene scene, uint32_t texture_map_index)
{
    GfxConstRef<GfxTextureMap> const texture_map_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return texture_map_ref;  // invalid parameter
    return gfx_scene->getObjectHandle<GfxTextureMap>(scene, texture_map_index);
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
