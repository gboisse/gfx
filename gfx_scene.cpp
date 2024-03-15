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

#include "gfx_scene.h"

#include <map>
#include <set>
#include <functional>
#include <ios>
#include <fstream>
#define CGLTF_IMPLEMENTATION
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable:4789)   // buffer will be overrun
#endif
#include <cgltf.h>              // glTF loader
#ifdef _MSC_VER
#   pragma warning(pop)
#endif
#include <tiny_obj_loader.h>   // obj loader
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <tinyexr.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <stb_image_write.h>
#ifdef __clang__
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
#ifdef GFX_ENABLE_SCENE_KTX
#include <ktx.h>
#include <vulkan/vulkan.h>
#endif
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

class GfxSceneInternal
{
    GFX_NON_COPYABLE(GfxSceneInternal);

    struct GltfNode
    {
        glm::dmat4 default_local_transform_;
        glm::dmat4 world_transform_;

        uint64_t parent_ = 0;
        GfxRef<GfxSkin> skin_;
        GfxRef<GfxLight> light_;
        GfxRef<GfxCamera> camera_;
        std::vector<uint64_t> children_;
        std::vector<GfxRef<GfxInstance>> instances_;
        std::vector<float> default_weights_;
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
        kGltfAnimationChannelType_Weights,

        kGltfAnimationChannelType_Count
    };

    struct GltfAnimationChannel
    {
        uint64_t node_;
        std::vector<float> keyframes_;
        std::vector<float> values_;
        GltfAnimationChannelMode mode_;
        GltfAnimationChannelType type_;
    };

    struct GltfAnimation
    {
        std::vector<uint64_t> animated_root_nodes_;
        std::vector<GfxRef<GfxSkin>> dependent_skins_;
        std::vector<GltfAnimationChannel> channels_;
    };

    struct GltfSkin
    {
        std::vector<glm::mat4> inverse_bind_matrices_;
        std::vector<uint64_t> joints_;
    };

    std::vector<uint64_t> scene_gltf_nodes_;
    GfxArray<GltfNode> gltf_nodes_;
    GfxArray<GltfAnimatedNode> gltf_animated_nodes_;
    GfxHandles gltf_node_handles_;
    GfxArray<GltfAnimation> gltf_animations_;
    GfxArray<GltfSkin> gltf_skins_;

    GfxArray<GfxAnimation> animations_;
    GfxArray<uint64_t> animation_refs_;
    GfxArray<GfxMetadata> animation_metadata_;
    GfxHandles animation_handles_;

    GfxArray<GfxSkin> skins_;
    GfxArray<uint64_t> skin_refs_;
    GfxArray<GfxMetadata> skin_metadata_;
    GfxHandles skin_handles_;

    GfxArray<GfxCamera> cameras_;
    GfxArray<uint64_t> camera_refs_;
    GfxArray<GfxMetadata> camera_metadata_;
    GfxRef<GfxCamera> active_camera_;
    GfxHandles camera_handles_;

    GfxArray<GfxLight> lights_;
    GfxArray<uint64_t> light_refs_;
    GfxArray<GfxMetadata> light_metadata_;
    GfxHandles light_handles_;

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

    template<> inline GfxArray<GfxSkin> &objects_<GfxSkin>() { return skins_; }
    template<> inline GfxArray<uint64_t> &object_refs_<GfxSkin>() { return skin_refs_; }
    template<> inline GfxArray<GfxMetadata> &object_metadata_<GfxSkin>() { return skin_metadata_; }
    template<> inline GfxHandles &object_handles_<GfxSkin>() { return skin_handles_; }

    template<> inline GfxArray<GfxCamera> &objects_<GfxCamera>() { return cameras_; }
    template<> inline GfxArray<uint64_t> &object_refs_<GfxCamera>() { return camera_refs_; }
    template<> inline GfxArray<GfxMetadata> &object_metadata_<GfxCamera>() { return camera_metadata_; }
    template<> inline GfxHandles &object_handles_<GfxCamera>() { return camera_handles_; }

    template<> inline GfxArray<GfxLight>& objects_<GfxLight>() { return lights_; }
    template<> inline GfxArray<uint64_t>& object_refs_<GfxLight>() { return light_refs_; }
    template<> inline GfxArray<GfxMetadata>& object_metadata_<GfxLight>() { return light_metadata_; }
    template<> inline GfxHandles& object_handles_<GfxLight>() { return light_handles_; }

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
    GfxSceneInternal(GfxScene &scene) : gltf_node_handles_("gltf_node"), animation_handles_("animation"), skin_handles_("skin"), camera_handles_("camera")
                                      , image_handles_("image"), material_handles_("material"), mesh_handles_("mesh"), instance_handles_("instance")
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
        else if(CaseInsensitiveCompare(asset_extension, ".dds"))
            GFX_TRY(importDds(scene, asset_file));
#ifdef GFX_ENABLE_SCENE_KTX
        else if(CaseInsensitiveCompare(asset_extension, ".ktx2") ||
            CaseInsensitiveCompare(asset_extension, ".ktx"))
            GFX_TRY(importKtx(scene, asset_file));
#endif
        else if(CaseInsensitiveCompare(asset_extension, ".exr"))
            GFX_TRY(importExr(scene, asset_file));
        else if(CaseInsensitiveCompare(asset_extension, ".bmp") ||
                CaseInsensitiveCompare(asset_extension, ".png") ||
                CaseInsensitiveCompare(asset_extension, ".tga") ||
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
        clearObjects<GfxSkin>();
        clearObjects<GfxCamera>();
        clearObjects<GfxLight>();
        clearObjects<GfxImage>();
        clearObjects<GfxMaterial>();
        clearObjects<GfxMesh>();
        clearObjects<GfxInstance>();
        clearNodes();

        return kGfxResult_NoError;
    }

    GfxResult applyAnimation(uint64_t animation_handle, float time_in_seconds)
    {
        if(!animation_handles_.has_handle(animation_handle))
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Cannot apply animation of an invalid object");
        GltfAnimation const *gltf_animation = gltf_animations_.at(GetObjectIndex(animation_handle));
        if(gltf_animation == nullptr)
            return kGfxResult_NoError;
        applyAnimation(*gltf_animation, time_in_seconds);
        updateTransforms(*gltf_animation);
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
                glm::dmat4 const transform = parent_transform * node.default_local_transform_;
                for(size_t i = 0; i < node.children_.size(); ++i)
                    VisitNode(node.children_[i], transform);
                for(size_t i = 0; i < node.instances_.size(); ++i)
                    if(node.instances_[i])
                        node.instances_[i]->transform = glm::mat4(transform);
                if(node.camera_)
                    TransformGltfCamera(*node.camera_, transform);
                if(node.light_)
                    TransformGltfLight(*node.light_, transform);
            };
            for(size_t i = 0; i < gltf_animation->animated_root_nodes_.size(); ++i)
                VisitNode(gltf_animation->animated_root_nodes_[i], glm::dmat4(1.0));
            for(size_t i = 0; i < gltf_animation->channels_.size(); ++i)
            {
                GltfAnimationChannel const &channel = gltf_animation->channels_[i];
                if(!gltf_node_handles_.has_handle(channel.node_) || (channel.type_ != kGltfAnimationChannelType_Weights)) continue;
                GltfNode const &node = gltf_nodes_[GetObjectIndex(channel.node_)];
                for(size_t j = 0; j < node.instances_.size(); ++j)
                {
                    if(!node.instances_[j]) continue;
                    if(!node.default_weights_.empty())
                        node.instances_[j]->weights = node.default_weights_;
                    else if(!node.instances_[j]->mesh->default_weights.empty())
                        node.instances_[j]->weights = node.instances_[j]->mesh->default_weights;
                    else
                        std::fill(node.instances_[j]->weights.begin(), node.instances_[j]->weights.end(), 0.0f);
                }
            }
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
        float animation_length = 0.0f;
        GltfAnimation const *gltf_animation = gltf_animations_.at(GetObjectIndex(animation_handle));
        if(gltf_animation != nullptr)
            for(size_t i = 0; i < gltf_animation->channels_.size(); ++i)
                if(!gltf_animation->channels_[i].keyframes_.empty())
                    animation_length = GFX_MAX(animation_length, gltf_animation->channels_[i].keyframes_.back());
        return animation_length;
    }

    GfxResult setActiveCamera(GfxScene const &scene, uint64_t camera_handle)
    {
        active_camera_.handle = camera_handle;
        active_camera_.scene = scene;

        return kGfxResult_NoError;
    }

    GfxRef<GfxCamera> getActiveCamera()
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
            gltf_animations_.erase(GetObjectIndex(object_handle));
        return kGfxResult_NoError;
    }

    template<>
    GfxResult destroyObjectCallback<GfxSkin>(uint64_t object_handle)
    {
        GFX_ASSERT(skin_handles_.has_handle(object_handle));
        GltfSkin const *gltf_skin = gltf_skins_.at(GetObjectIndex(object_handle));
        if(gltf_skin != nullptr)
            gltf_skins_.erase(GetObjectIndex(object_handle));
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

    GfxResult clearNodes()
    {
        std::function<void(uint64_t)> VisitNode;
        VisitNode = [&](uint64_t node_handle)
        {
            if(!gltf_node_handles_.has_handle(node_handle)) return;
            GltfNode const &node = gltf_nodes_[GetObjectIndex(node_handle)];
            for(uint64_t child_handle : node.children_)
                VisitNode(child_handle);   // release child nodes
            gltf_node_handles_.free_handle(node_handle);
            gltf_nodes_.erase(GetObjectIndex(node_handle));
            if(gltf_animated_nodes_.has(GetObjectIndex(node_handle)))
                gltf_animated_nodes_.erase(GetObjectIndex(node_handle));
        };
        for(uint64_t node_handle : scene_gltf_nodes_)
            VisitNode(node_handle);
        scene_gltf_nodes_.clear();
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

    template<typename TYPE>
    bool setObjectMetadata(uint64_t object_handle, GfxMetadata const &metadata)
    {
        if(!object_handles_<TYPE>().has_handle(object_handle))
            return false;   // invalid object handle
        GfxMetadata &object_metadata = object_metadata_<TYPE>()[GetObjectIndex(object_handle)];
        object_metadata = metadata; // commit metadata
        object_metadata.is_valid = true;
        return true;
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

    // The cgltf_node_transform_local function adaptation
    static inline glm::dmat4 CalculateNodeTransform(glm::dvec3 trans, glm::dquat rot, glm::dvec3 scale)
    {
        glm::dmat4 transform;

        transform[0].x = (1 - 2.0 * rot.y * rot.y - 2.0 * rot.z * rot.z) * scale.x;
        transform[0].y = (2.0 * rot.x * rot.y + 2.0 * rot.z * rot.w) * scale.x;
        transform[0].z = (2.0 * rot.x * rot.z - 2.0 * rot.y * rot.w) * scale.x;
        transform[0].w = 0.0;

        transform[1].x = (2.0 * rot.x * rot.y - 2.0 * rot.z * rot.w) * scale.x;
        transform[1].y = (1.0 - 2.0 * rot.x * rot.x - 2.0 * rot.z * rot.z) * scale.x;
        transform[1].z = (2.0 * rot.y * rot.z + 2 * rot.x * rot.w) * scale.x;
        transform[1].w = 0.0;

        transform[2].x = (2.0 * rot.x * rot.z + 2.0 * rot.y * rot.w) * scale.x;
        transform[2].y = (2.0 * rot.y * rot.z - 2.0 * rot.x * rot.w) * scale.x;
        transform[2].z = (1.0 - 2.0 * rot.x * rot.x - 2.0 * rot.y * rot.y) * scale.x;
        transform[2].w = 0.0;

        transform[3].x = trans.x;
        transform[3].y = trans.y;
        transform[3].z = trans.z;
        transform[3].w = 1.0;

        return transform;
    }

    static inline void TransformGltfCamera(GfxCamera &camera, glm::dmat4 const &transform)
    {
        glm::dvec4 eye(0.0, 0.0, 0.0, 1.0);
        glm::dvec4 center(0.0, 0.0, -1.0, 1.0);
        glm::dvec4 up(0.0, 1.0, 0.0, 0.0);

        eye    = transform * eye;
        center = transform * center;
        up     = transform * up;

        camera.eye    = glm::vec3(eye / eye.w);
        camera.center = glm::vec3(center / center.w);
        camera.up     = glm::vec3(glm::normalize(up));
    }

    static inline void TransformGltfLight(GfxLight& light, glm::dmat4 const& transform)
    {
        glm::dvec4 position(0.0, 0.0, 0.0, 1.0);
        glm::dvec4 direction(0.0, 0.0, 1.0, 0.0);//Direction oriented toward the light

        position = transform * position;
        direction = transform * direction;

        light.position = glm::vec3(position / position.w);
        light.direction = glm::vec3(direction);
    }

    static inline DXGI_FORMAT GetImageFormat(GfxImage const& image)
    {
        if(image.format != DXGI_FORMAT_UNKNOWN) return image.format;
        if(image.bytes_per_channel != 1 && image.bytes_per_channel != 2 && image.bytes_per_channel != 4) return DXGI_FORMAT_UNKNOWN;
        uint32_t const bits = (image.bytes_per_channel << 3);
        switch(image.channel_count) {
        case 1:
            return (bits == 8 ? DXGI_FORMAT_R8_UNORM : bits == 16 ? DXGI_FORMAT_R16_UNORM : DXGI_FORMAT_R32_FLOAT);
        case 2:
            return (bits == 8 ? DXGI_FORMAT_R8G8_UNORM : bits == 16 ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R32G32_FLOAT);
        case 4:
            return (bits == 8 ? DXGI_FORMAT_R8G8B8A8_UNORM : bits == 16 ? DXGI_FORMAT_R16G16B16A16_UNORM : DXGI_FORMAT_R32G32B32A32_FLOAT);
        default:
            break;
        }
        return DXGI_FORMAT_UNKNOWN;
    }

    static inline DXGI_FORMAT ConvertImageFormatSRGB(const DXGI_FORMAT format)
    {
        switch(format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        default:
            break;
        }
        return format;
    }

    static inline DXGI_FORMAT ConvertImageFormatLinear(const DXGI_FORMAT format)
    {
        switch(format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM;
        case DXGI_FORMAT_BC2_UNORM_SRGB: return DXGI_FORMAT_BC2_UNORM;
        case DXGI_FORMAT_BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB: return DXGI_FORMAT_BC7_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_TYPELESS;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8X8_TYPELESS;
        default:
            break;
        }
        return format;
    }

    inline static uint32_t GetBitsPerPixel(DXGI_FORMAT format)
    {
        switch(format)
        {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            return 64;
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
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_YUY2:
            return 32;
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            return 24;
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
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_NV11:
            return 12;
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
            return 8;
        case DXGI_FORMAT_R1_UNORM:
            return 1;
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;
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
            return 8;
        default:
            return 0;
        }
    }

    inline static uint32_t GetNumChannels(DXGI_FORMAT format)
    {
        switch(format)
        {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC7_TYPELESS: // BC7 could also be 3 channel, there is no way of knowing
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 4;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_YUY2:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
        case DXGI_FORMAT_NV11:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_P208:
        case DXGI_FORMAT_V208:
        case DXGI_FORMAT_V408:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
            return 3;
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
            return 2;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_R1_UNORM:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        return 1;
        default: return 0;
        }
    }

    template<typename T>
    static inline bool UnpackAccessor(cgltf_accessor const *accessor, std::vector<T> &buffer)
    {
        if(accessor == nullptr) return true;
        GFX_ASSERT(sizeof(T) == cgltf_num_components(accessor->type) * sizeof(float));
        buffer.resize(accessor->count);
        cgltf_size floats_size = cgltf_num_components(accessor->type) * accessor->count;
        if(cgltf_accessor_unpack_floats(accessor, (float*)&buffer[0], floats_size) < floats_size)
        {
            GFX_PRINT_ERROR(kGfxResult_InternalError, "Failed to unpack sparse accessor");
            return false;
        }
        return true;
    };

    void applyAnimation(GltfAnimation const &gltf_animation, float time_in_seconds)
    {
        for(size_t i = 0; i < gltf_animation.channels_.size(); ++i)
        {
            GltfAnimationChannel const &animation_channel = gltf_animation.channels_[i];
            if(!gltf_node_handles_.has_handle(animation_channel.node_)) continue;   // invalid target node
            GltfAnimatedNode *animated_node = gltf_animated_nodes_.at(GetObjectIndex(animation_channel.node_));
            if(animation_channel.keyframes_.empty() || ((animation_channel.type_ != kGltfAnimationChannelType_Weights) &&
                (animated_node == nullptr))) { GFX_ASSERT(0); continue; }
            if(animation_channel.keyframes_.empty()) { GFX_ASSERT(0); continue; }
            intptr_t const keyframe = std::lower_bound(animation_channel.keyframes_.begin(),
                animation_channel.keyframes_.end(), time_in_seconds) - animation_channel.keyframes_.begin();
            size_t const previous_index = std::max(keyframe - 1, 0ll);
            size_t const next_index = std::min(keyframe, (intptr_t)animation_channel.keyframes_.size() - 1);
            double interpolate = 0.0;
            if((animation_channel.mode_ == kGltfAnimationChannelMode_Linear) && (previous_index != next_index))
            {
                interpolate = ((double)time_in_seconds - (double)animation_channel.keyframes_[keyframe - 1]) /
                    ((double)animation_channel.keyframes_[keyframe] - (double)animation_channel.keyframes_[keyframe - 1]);
            }
            if(animation_channel.type_ == kGltfAnimationChannelType_Translate)
            {
                for(uint32_t j = 0; j < 3; ++j)
                {
                    animated_node->translate_[j] = glm::mix(animation_channel.values_[3 * previous_index + j],
                        animation_channel.values_[3 * next_index + j], interpolate);
                }
            }
            else if(animation_channel.type_ == kGltfAnimationChannelType_Rotate)
            {
                glm::dquat const previous = glm::dquat(animation_channel.values_[4 * previous_index + 3], animation_channel.values_[4 * previous_index],
                    animation_channel.values_[4 * previous_index + 1], animation_channel.values_[4 * previous_index + 2]);
                glm::dquat const next = glm::dquat(animation_channel.values_[4 * next_index + 3], animation_channel.values_[4 * next_index],
                    animation_channel.values_[4 * next_index + 1], animation_channel.values_[4 * next_index + 2]);
                animated_node->rotate_ = glm::slerp(previous, next, interpolate);
            }
            else if(animation_channel.type_ == kGltfAnimationChannelType_Scale)
            {
                for(uint32_t j = 0; j < 3; ++j)
                {
                    animated_node->scale_[j] = glm::mix(animation_channel.values_[3 * previous_index + j],
                        animation_channel.values_[3 * next_index + j], interpolate);
                }
            }
            else
            {
                size_t const weights_count = animation_channel.values_.size() / animation_channel.keyframes_.size();
                std::vector<float> weights;
                weights.resize(weights_count);
                for(size_t j = 0; j < weights_count; ++j)
                {
                    weights[j] = glm::mix(animation_channel.values_[previous_index * weights_count + j],
                        animation_channel.values_[next_index * weights_count + j], interpolate);
                }
                GltfNode const &node = gltf_nodes_[GetObjectIndex(animation_channel.node_)];
                for(size_t j = 0; j < node.instances_.size(); ++j)
                {
                    if(!node.instances_[j]) continue;
                    node.instances_[j]->weights = weights;
                }
            }
        }
    }

    void updateTransforms(GltfAnimation const &gltf_animation)
    {
        std::function<void(uint64_t, glm::dmat4 const &)> VisitNode;
        VisitNode = [&](uint64_t node_handle, glm::dmat4 const &parent_transform)
        {
            GFX_ASSERT(gltf_node_handles_.has_handle(node_handle));
            GltfNode &node = gltf_nodes_[GetObjectIndex(node_handle)];
            GltfAnimatedNode *animated_node = gltf_animated_nodes_.at(GetObjectIndex(node_handle));
            if(animated_node == nullptr)
                node.world_transform_ = parent_transform * node.default_local_transform_;
            else
                node.world_transform_ = parent_transform * CalculateNodeTransform(
                    animated_node->translate_, animated_node->rotate_, animated_node->scale_);
            for(size_t i = 0; i < node.children_.size(); ++i)
                VisitNode(node.children_[i], node.world_transform_);
            for(size_t i = 0; i < node.instances_.size(); ++i)
                if(node.instances_[i])
                    node.instances_[i]->transform = glm::mat4(node.world_transform_);
            if(node.camera_)
                TransformGltfCamera(*node.camera_, node.world_transform_);
            if(node.light_)
                TransformGltfLight(*node.light_, node.world_transform_);
        };
        for(uint64_t node_handle : gltf_animation.animated_root_nodes_)
        {
            GFX_ASSERT(gltf_node_handles_.has_handle(node_handle));
            GltfNode &node = gltf_nodes_[GetObjectIndex(node_handle)];
            if(!node.parent_ || !gltf_node_handles_.has_handle(node.parent_))
            {
                VisitNode(node_handle, glm::dmat4(1.0));
                continue;
            }
            VisitNode(node_handle, gltf_nodes_[GetObjectIndex(node.parent_)].world_transform_);
        }
        for(auto const skin : gltf_animation.dependent_skins_)
        {
            GltfSkin const *gltf_skin = gltf_skins_.at(GetObjectIndex(skin));
            for(size_t i = 0; i < skin->joint_matrices.size(); ++i)
            {
                GFX_ASSERT(gltf_node_handles_.has_handle(gltf_skin->joints_[i]));
                GltfNode const &joint_node = gltf_nodes_[GetObjectIndex(gltf_skin->joints_[i])];
                skin->joint_matrices[i] = joint_node.world_transform_ * glm::dmat4(gltf_skin->inverse_bind_matrices_[i]);
            }
        }
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
            std::string texture_file = texture_path + texname;
            FILE *fd = fopen(texture_file.c_str(), "rb"); if(fd) fclose(fd);
            if(fd == nullptr) texture_file = texname;   // is this not a relative path?
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
            material_ref->ior = obj_material.ior;
            material_ref->specular = glm::vec4(obj_material.specular[0], obj_material.specular[1], obj_material.specular[2], 1.0f);
            LoadImage(obj_material.diffuse_texname, material_ref->albedo_map);
            if(material_ref->albedo_map)
            {
                if(material_ref->albedo_map->bytes_per_channel <= 1)
                    gfxSceneGetObject<GfxImage>(scene, material_ref->albedo_map)->format = ConvertImageFormatSRGB(material_ref->albedo_map->format);
                material_ref->albedo = glm::vec4(glm::vec3(1.0f), material_ref->albedo.w);
            }
            LoadImage(obj_material.roughness_texname, material_ref->roughness_map);
            if(material_ref->roughness_map) material_ref->roughness = 1.0f;
            LoadImage(obj_material.metallic_texname, material_ref->metallicity_map);
            if(material_ref->metallicity_map) material_ref->metallicity = 1.0f;
            LoadImage(obj_material.emissive_texname, material_ref->emissivity_map);
            if(material_ref->emissivity_map)
            {
                if(material_ref->emissivity_map->bytes_per_channel <= 1)
                    gfxSceneGetObject<GfxImage>(scene, material_ref->emissivity_map)->format = ConvertImageFormatSRGB(material_ref->emissivity_map->format);
                material_ref->emissivity = glm::vec3(1.0f);
            }
            LoadImage(obj_material.specular_texname, material_ref->specular_map);
            if(material_ref->specular_map)
            {
                if(material_ref->specular_map->bytes_per_channel <= 1)
                    gfxSceneGetObject<GfxImage>(scene, material_ref->specular_map)->format = ConvertImageFormatSRGB(material_ref->specular_map->format);
                material_ref->specular = glm::vec4(1.0f);
            }
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
                            vertex.uv.y = 1.0f - obj_reader.GetAttrib().texcoords[2 * std::get<2>(key) + 1];
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
                if((*it).first < materials.size())
                    instance_ref->material = materials[(*it).first];
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

    GfxResult importGltf(GfxScene const &scene, char const *asset_file)
    {
        GFX_ASSERT(asset_file != nullptr);
        cgltf_options options;
        memset(&options, 0, sizeof(cgltf_options));
        cgltf_data *gltf_model = nullptr;
        cgltf_result result = cgltf_parse_file(&options, asset_file, &gltf_model);
        if(result != cgltf_result_success) {
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Failed to parse gltf file `%s'", asset_file);
        }
        result = cgltf_load_buffers(&options, gltf_model, asset_file);
        if(result != cgltf_result_success)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Failed to load gltf file `%s'", asset_file);
#ifndef NDEBUG
        if(cgltf_validate(gltf_model) != cgltf_result_success)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Invalid gltf file `%s'", asset_file);
#endif //! NDEBUG
        if(gltf_model->scenes_count == 0)
            return kGfxResult_NoError;  // nothing needs loading
        std::map<cgltf_skin const *, GfxConstRef<GfxSkin>> skins;
        for(size_t i = 0; i < gltf_model->skins_count; ++i)
        {
            cgltf_skin const &gltf_skin = gltf_model->skins[i];
            GfxRef<GfxSkin> skin_ref = gfxSceneCreateSkin(scene);
            skin_ref->joint_matrices.resize(gltf_skin.joints_count);
            GfxMetadata &skin_metadata = skin_metadata_[skin_ref];
            skin_metadata.asset_file = asset_file;    // set up metadata
            skin_metadata.object_name = (gltf_skin.name != nullptr) ? gltf_skin.name : "Skin" + std::to_string(i);
            skins[&gltf_skin] = skin_ref;
        }
        std::map<cgltf_camera const *, GfxConstRef<GfxCamera>> cameras;
        for(size_t i = 0; i < gltf_model->cameras_count; ++i)
        {
            cgltf_camera const &gltf_camera = gltf_model->cameras[i];
            if(gltf_camera.type != cgltf_camera_type_perspective) continue; // unsupported camera type
            cgltf_camera_perspective const &gltf_perspective_camera = gltf_camera.data.perspective;
            GfxRef<GfxCamera> camera_ref = gfxSceneCreateCamera(scene);
            GfxCamera &camera = *camera_ref;
            camera.type = kGfxCameraType_Perspective;
            TransformGltfCamera(camera, glm::dmat4(1.0));
            camera.aspect = gltf_perspective_camera.has_aspect_ratio ? gltf_perspective_camera.aspect_ratio : 1.0f;
            camera.fovY   = gltf_perspective_camera.yfov;
            camera.nearZ  = gltf_perspective_camera.znear;
            camera.farZ   = gltf_perspective_camera.zfar;
            GfxMetadata &camera_metadata = camera_metadata_[camera_ref];
            camera_metadata.asset_file = asset_file;    // set up metadata
            camera_metadata.object_name = (gltf_camera.name != nullptr) ? gltf_camera.name : "Camera" + std::to_string(i);
            cameras[&gltf_camera] = camera_ref;
        }
        std::map<cgltf_light const *, GfxConstRef<GfxLight>> lights;
        for(size_t i = 0; i < gltf_model->lights_count; ++i)
        {
            cgltf_light const &gltf_light = gltf_model->lights[i];
            GfxRef<GfxLight> light_ref = gfxSceneCreateLight(scene);
            GfxLight &light = *light_ref;
            light.color = glm::vec3(gltf_light.color[0], gltf_light.color[1], gltf_light.color[2]);
            float const lumens_to_watts = 683.f;
            light.intensity             = gltf_light.intensity / lumens_to_watts;
            if(gltf_light.type == cgltf_light_type_point || gltf_light.type == cgltf_light_type_spot)
                light.intensity *= 4.0f * 3.1415926535897932384626433832795f;
            light.range = gltf_light.range > 0.0 ? gltf_light.range : FLT_MAX;
            light.type = gltf_light.type == cgltf_light_type_point ? kGfxLightType_Point :
                (gltf_light.type == cgltf_light_type_spot ? kGfxLightType_Spot : kGfxLightType_Directional);
            if(light.type == kGfxLightType_Spot)
            {
                light.inner_cone_angle = gltf_light.spot_inner_cone_angle;
                light.outer_cone_angle = gltf_light.spot_outer_cone_angle;
            }
            TransformGltfLight(light, glm::dmat4(1.0));
            GfxMetadata &light_metadata = light_metadata_[light_ref];
            light_metadata.asset_file = asset_file;    // set up metadata
            light_metadata.object_name = (gltf_light.name != nullptr) ? gltf_light.name : "Light" + std::to_string(i);
            lights[&gltf_light] = light_ref;
        }
        std::map<cgltf_image const *, GfxConstRef<GfxImage>> images;
        for(size_t i = 0; i < gltf_model->textures_count; ++i)
        {
            cgltf_texture const &gltf_texture = gltf_model->textures[i];
            uint32_t texture_dds_index = std::numeric_limits<uint32_t>::max();
            for(uint64_t e = 0; e < gltf_texture.extensions_count; ++e)
            {
                if(strcmp(gltf_texture.extensions[e].name, "MSFT_texture_dds") == 0)
                {
                    std::string json = gltf_texture.extensions[e].data;
                    json.erase(remove_if(json.begin(), json.end(),
                                   [](char c) { return (c == '\r' || c == '\t' || c == ' ' || c == '\n'); }),
                        json.end());
                    constexpr std::string_view source_tag = "\"source\":";
                    if(auto pos = json.find(source_tag); pos != std::string::npos)
                    {
                        pos += source_tag.length();
                        std::string clipped = json.substr(pos, json.find_first_not_of("0123456789", pos + 1) - pos);
                        texture_dds_index = stoi(clipped);
                    }
                }
            }
            if(gltf_texture.image == nullptr && gltf_texture.basisu_image == nullptr
                && texture_dds_index != std::numeric_limits<uint32_t>::max())
                continue;
            cgltf_image const *gltf_image = gltf_texture.has_basisu ? gltf_texture.basisu_image : gltf_texture.image;
            if(texture_dds_index != std::numeric_limits<uint32_t>::max())
            {
                gltf_image = &(gltf_model->images[texture_dds_index]);
            }
            GfxRef<GfxImage> image_ref;
            if(gltf_image->uri != nullptr)
            {
                char const *filename = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '\\'));
                std::string image_folder =
                    (filename != nullptr ? std::string(asset_file, 0, filename - asset_file)
                                         : std::string(""));
                if(!image_folder.empty() && image_folder.back() != '/' && image_folder.back() != '\\')
                    image_folder += '/';
                std::string image_file = image_folder + gltf_image->uri;
                if(gfxSceneImport(scene, image_file.c_str()) != kGfxResult_NoError)
                    continue; // unable to load image file
                image_ref          = gfxSceneFindObjectByAssetFile<GfxImage>(scene, image_file.c_str());
                images[gltf_texture.image] = image_ref;
            }
            else if(gltf_image->buffer_view != nullptr)
            {
                const std::string mime(gltf_image->mime_type);
                if(mime != "image/jpeg" && mime != "image/png")
                {
                    GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Unsupported embedded texture type '%s'", mime.c_str());
                    continue;
                }
                void *ptr = (uint8_t*)gltf_image->buffer_view->buffer->data + gltf_image->buffer_view->offset;
                if(importImage(scene, gltf_image->name, ptr, gltf_image->buffer_view->size) != kGfxResult_NoError)
                    continue; // unable to load image file
                image_ref          = gfxSceneFindObjectByAssetFile<GfxImage>(scene, gltf_image->name);
                images[gltf_texture.image] = image_ref;
            }
        }
        std::map<cgltf_material const *, GfxConstRef<GfxMaterial>> materials;
        std::map<cgltf_image const *, std::pair<GfxConstRef<GfxImage>, GfxConstRef<GfxImage>>> maps;
        for(size_t i = 0; i < gltf_model->materials_count; ++i)
        {
            std::map<cgltf_image const *, GfxConstRef<GfxImage>>::const_iterator it;
            cgltf_material const &gltf_material = gltf_model->materials[i];
            if(!gltf_material.has_pbr_metallic_roughness)
                continue; // unsupported material type
            GfxRef<GfxMaterial> material_ref = gfxSceneCreateMaterial(scene);
            GfxMetadata &material_metadata = material_metadata_[material_ref];
            material_metadata.asset_file = asset_file;  // set up metadata
            material_metadata.object_name = (gltf_material.name != nullptr) ? gltf_material.name : "Material" + std::to_string(i);
            GfxMaterial &material = *material_ref;
            cgltf_pbr_metallic_roughness const &gltf_material_pbr = gltf_material.pbr_metallic_roughness;
            material.albedo = glm::vec4(gltf_material_pbr.base_color_factor[0], gltf_material_pbr.base_color_factor[1],
                gltf_material_pbr.base_color_factor[2], gltf_material_pbr.base_color_factor[3]);
            material.roughness = gltf_material_pbr.roughness_factor;
            material.metallicity = gltf_material_pbr.metallic_factor;
            float const emissiveFactor = gltf_material.emissive_strength.emissive_strength;
            material.emissivity = glm::vec3(gltf_material.emissive_factor[0], gltf_material.emissive_factor[1],
                gltf_material.emissive_factor[2]);
            if(emissiveFactor > 0.0f)
                material.emissivity *= emissiveFactor;
            if(gltf_material.has_ior)
                material.ior = gltf_material.ior.ior;
            if(gltf_material.has_specular)
                material.specular = glm::vec4(gltf_material.specular.specular_color_factor[0], gltf_material.specular.specular_color_factor[1], gltf_material.specular.specular_color_factor[2], gltf_material.specular.specular_factor);
            if(gltf_material.has_transmission)
                material.transmission = gltf_material.transmission.transmission_factor;
            if(gltf_material.has_sheen)
                material.sheen = glm::vec4(gltf_material.sheen.sheen_color_factor[0], gltf_material.sheen.sheen_color_factor[1], gltf_material.sheen.sheen_color_factor[2], gltf_material.sheen.sheen_roughness_factor);
            if(gltf_material.has_clearcoat)
            {
                material.clearcoat = gltf_material.clearcoat.clearcoat_factor;
                material.clearcoat_roughness = gltf_material.clearcoat.clearcoat_roughness_factor;
            }
            if(gltf_material.double_sided) material.flags |= kGfxMaterialFlag_DoubleSided;
            cgltf_texture const *albedo_map_text = gltf_material_pbr.base_color_texture.texture;
            it = (albedo_map_text != nullptr ? images.find(albedo_map_text->image) : images.end());
            if(it != images.end())
            {
                GfxImage *temp = gfxSceneGetObject<GfxImage>(scene, (*it).second);
                if(temp->bytes_per_channel <= 1)
                    temp->format = ConvertImageFormatSRGB(temp->format);
                material.albedo_map = (*it).second;
            }
            cgltf_texture const *metallicity_roughness_map_text = gltf_material_pbr.metallic_roughness_texture.texture;
            it = (metallicity_roughness_map_text != nullptr ? images.find(metallicity_roughness_map_text->image) : images.end());
            if(it != images.end())
            {
                std::map<cgltf_image const *, std::pair<GfxConstRef<GfxImage>, GfxConstRef<GfxImage>>>::const_iterator const it2 =
                    maps.find(it->first);
                if(it2 != maps.end())
                {
                    material_ref->roughness_map = (*it2).second.first;
                    material_ref->metallicity_map = (*it2).second.second;
                }
                else
                {
                    size_t const asset_file_ext = image_metadata_[(*it).second].asset_file.rfind('.');
                    std::string  metallicity_map_file;
                    std::string  roughness_map_file;
                    if(asset_file_ext != std::string::npos)
                    {
                        std::string const asset_file_name =
                            image_metadata_[(*it).second].asset_file.substr(0, asset_file_ext);
                        std::string const asset_file_extension =
                            image_metadata_[(*it).second].asset_file.substr(asset_file_ext);
                        metallicity_map_file =
                            asset_file_name + ".metallicity" + asset_file_extension;
                        roughness_map_file =
                            asset_file_name + ".roughness" + asset_file_extension;
                    }
                    else
                    {
                        // Embedded texture
                        metallicity_map_file = image_metadata_[(*it).second].asset_file + ".metallicity";
                        metallicity_map_file = image_metadata_[(*it).second].asset_file + ".roughness";
                    }
                    GfxRef<GfxImage> metallicity_map_ref = gfxSceneFindObjectByAssetFile<GfxImage>(scene, metallicity_map_file.c_str());
                    GfxRef<GfxImage> roughness_map_ref = gfxSceneFindObjectByAssetFile<GfxImage>(scene, roughness_map_file.c_str());
                    if(!metallicity_map_ref)
                    {
                        std::ifstream f(metallicity_map_file.c_str(), std::ios_base::in);
                        if(f.good() && gfxSceneImport(scene, metallicity_map_file.c_str()) == kGfxResult_NoError)
                        {
                            metallicity_map_ref = gfxSceneFindObjectByAssetFile<GfxImage>(scene, metallicity_map_file.c_str());
                            metallicity_map_ref->format = ConvertImageFormatLinear(metallicity_map_ref->format);
                        }
                    }
                    if(!roughness_map_ref)
                    {
                        std::ifstream f(roughness_map_file.c_str(), std::ios_base::in);
                        if(f.good() && gfxSceneImport(scene, roughness_map_file.c_str()) == kGfxResult_NoError)
                        {
                            roughness_map_ref = gfxSceneFindObjectByAssetFile<GfxImage>(scene, roughness_map_file.c_str());
                            roughness_map_ref->format = ConvertImageFormatLinear(roughness_map_ref->format);
                        }
                    }
                    if(!metallicity_map_ref && !roughness_map_ref)
                    {
                        if(gfxImageIsFormatCompressed(*gfxSceneGetObject<GfxImage>(scene, (*it).second)))
                        {
                            GFX_PRINT_ERROR(kGfxResult_InvalidOperation, "Compressed textures require separate metal/roughness textures '%s'",
                                image_metadata_[(*it).second].asset_file.c_str());
                            continue;
                        }
                        metallicity_map_ref = gfxSceneCreateImage(scene);
                        roughness_map_ref = gfxSceneCreateImage(scene);
                        GfxMetadata &metallicity_map_metadata = image_metadata_[metallicity_map_ref];
                        metallicity_map_metadata = image_metadata_[(*it).second];   // set up metadata
                        metallicity_map_metadata.asset_file = metallicity_map_file;
                        metallicity_map_metadata.object_name = metallicity_map_file;
                        GfxMetadata &roughness_map_metadata = image_metadata_[roughness_map_ref];
                        roughness_map_metadata = image_metadata_[(*it).second];
                        roughness_map_metadata.asset_file = roughness_map_file;
                        roughness_map_metadata.object_name += roughness_map_file;
                        GfxImage &metallicity_map = *metallicity_map_ref;
                        GfxImage &roughness_map = *roughness_map_ref;
                        GfxImage const &image = *(*it).second;
                        metallicity_map.width = image.width;
                        metallicity_map.height = image.height;
                        metallicity_map.channel_count = 1;
                        metallicity_map.bytes_per_channel = image.bytes_per_channel;
                        metallicity_map.format = GetImageFormat(metallicity_map);
                        metallicity_map.flags = 0;
                        metallicity_map.data.resize((size_t)metallicity_map.width * metallicity_map.height *
                            metallicity_map.bytes_per_channel);
                        roughness_map.width = image.width;
                        roughness_map.height = image.height;
                        roughness_map.channel_count = 1;
                        roughness_map.bytes_per_channel = image.bytes_per_channel;
                        roughness_map.format = GetImageFormat(roughness_map);
                        roughness_map.flags = 0;
                        roughness_map.data.resize((size_t)roughness_map.width * roughness_map.height *
                            roughness_map.bytes_per_channel);
                        uint32_t const texel_count = image.width * image.height * image.bytes_per_channel;
                        uint32_t const byte_stride = image.channel_count * image.bytes_per_channel;
                        for(uint32_t j = 0; j < texel_count; ++j)
                        {
                            uint32_t index = j * byte_stride + image.bytes_per_channel;
                            if(index + image.bytes_per_channel <= (uint32_t)image.data.size())
                                for(uint32_t k = 0; k < image.bytes_per_channel; ++k)
                                    roughness_map.data[(size_t)j * image.bytes_per_channel + k] = image.data[index++];
                            if(index + image.bytes_per_channel <= (uint32_t)image.data.size())
                                for(uint32_t k = 0; k < image.bytes_per_channel; ++k)
                                    metallicity_map.data[(size_t)j * image.bytes_per_channel + k] = image.data[index++];
                        }
                    }
                    material.roughness_map = roughness_map_ref;
                    material.metallicity_map = metallicity_map_ref;
                    maps[it->first] =
                        std::pair<GfxConstRef<GfxImage>, GfxConstRef<GfxImage>>(roughness_map_ref, metallicity_map_ref);
                }
            }
            cgltf_texture const *emissivity_map_text = gltf_material.emissive_texture.texture;
            it = (emissivity_map_text != nullptr ? images.find(emissivity_map_text->image) : images.end());
            if(it != images.end())
            {
                GfxImage *temp = gfxSceneGetObject<GfxImage>(scene, (*it).second);
                if(temp->bytes_per_channel <= 1)
                    temp->format = ConvertImageFormatSRGB(temp->format);
                material.emissivity_map = (*it).second;
            }
            if(gltf_material.has_specular)
            {
                cgltf_texture const *specular_map_text = gltf_material.specular.specular_color_texture.texture;
                it = (specular_map_text != nullptr ? images.find(specular_map_text->image) : images.end());
                if(it != images.end())
                {
                    GfxImage *temp = gfxSceneGetObject<GfxImage>(scene, (*it).second);
                    if(temp->bytes_per_channel <= 1)
                        temp->format = ConvertImageFormatSRGB(temp->format);
                    material.specular_map = (*it).second;
                }
                if(gltf_material.specular.specular_texture.texture != nullptr &&
                   gltf_material.specular.specular_texture.texture != specular_map_text)
                    GFX_PRINT_ERROR(kGfxResult_InvalidOperation,
                        "Specular factor texture should be stored in Specular color texture alpha channel");
            }
            cgltf_texture const *normal_map_text = gltf_material.normal_texture.texture;
            it = (normal_map_text != nullptr ? images.find(normal_map_text->image) : images.end());
            if(it != images.end())
            {
                GfxImage *temp = gfxSceneGetObject<GfxImage>(scene, (*it).second);
                temp->format = ConvertImageFormatLinear(temp->format);
                material.normal_map = (*it).second;
            }
            if(gltf_material.has_transmission)
            {
                cgltf_texture const *transmission_map_text = gltf_material.transmission.transmission_texture.texture;
                it = (transmission_map_text != nullptr ? images.find(transmission_map_text->image) : images.end());
                if(it != images.end())
                {
                    GfxImage *temp = gfxSceneGetObject<GfxImage>(scene, (*it).second);
                    temp->format = ConvertImageFormatLinear(temp->format);
                    material.transmission_map = (*it).second;
                }
            }
            if(gltf_material.has_sheen)
            {
                cgltf_texture const *sheen_map_text = gltf_material.sheen.sheen_color_texture.texture;
                it = (sheen_map_text != nullptr ? images.find(sheen_map_text->image) : images.end());
                if(it != images.end())
                {
                    GfxImage *temp = gfxSceneGetObject<GfxImage>(scene, (*it).second);
                    if(temp->bytes_per_channel <= 1)
                        temp->format = ConvertImageFormatSRGB(temp->format);
                    material.sheen_map = (*it).second;
                }
                if(gltf_material.sheen.sheen_roughness_texture.texture != nullptr &&
                   gltf_material.sheen.sheen_roughness_texture.texture != sheen_map_text)
                    GFX_PRINT_ERROR(kGfxResult_InvalidOperation,
                        "Sheen roughness texture should be stored in Sheen color texture alpha channel");
            }
            if(gltf_material.has_clearcoat)
            {
                cgltf_texture const *clearcoat_map_text = gltf_material.clearcoat.clearcoat_texture.texture;
                it = (clearcoat_map_text != nullptr ? images.find(clearcoat_map_text->image) : images.end());
                if(it != images.end())
                {
                    GfxImage *temp = gfxSceneGetObject<GfxImage>(scene, (*it).second);
                    temp->format = ConvertImageFormatLinear(temp->format);
                    material.clearcoat_map = (*it).second;
                }
                cgltf_texture const *clearcoat_rough_map_text = gltf_material.clearcoat.clearcoat_roughness_texture.texture;
                it = (clearcoat_rough_map_text != nullptr ? images.find(clearcoat_rough_map_text->image) : images.end());
                if(it != images.end())
                {
                    GfxImage *temp = gfxSceneGetObject<GfxImage>(scene, (*it).second);
                    temp->format = ConvertImageFormatLinear(temp->format);
                    material.clearcoat_roughness_map = (*it).second;
                }
            }
            cgltf_texture const *ao_map_text = gltf_material.occlusion_texture.texture;
            it = (ao_map_text != nullptr ? images.find(ao_map_text->image) : images.end());
            if(it != images.end()) material.ao_map = (*it).second;
            materials[&gltf_material] = material_ref;
        }
        typedef std::pair<GfxConstRef<GfxMesh>, GfxConstRef<GfxMaterial>> instance_pair;
        std::map<cgltf_mesh const *, std::vector<instance_pair>> meshes;
        struct MeshAccessors
        {
            cgltf_accessor const *indices;
            cgltf_accessor const *positions;
            cgltf_accessor const *normals;
            cgltf_accessor const *uvs;
            cgltf_accessor const *joints;
            cgltf_accessor const *weights;
            struct TargetAccessors
            {
                cgltf_accessor const* positions;
                cgltf_accessor const* normals;
                cgltf_accessor const* uvs;
            };
            std::vector<TargetAccessors> targets;

            bool operator<(MeshAccessors const &v) const
            {
                if(indices != v.indices) return indices < v.indices;
                if(positions != v.positions) return positions < v.positions;
                if(normals != v.normals) return normals < v.normals;
                if(uvs != v.uvs) return uvs < v.uvs;
                if(joints != v.joints) return joints < v.joints;
                if(weights != v.weights) return weights < v.weights;
                if(targets.size() != v.targets.size()) return targets.size() < v.targets.size();
                for(size_t i = 0; i < targets.size(); ++i)
                {
                    if(targets[i].positions != v.targets[i].positions) return targets[i].positions < v.targets[i].positions;
                    if(targets[i].normals != v.targets[i].normals) return targets[i].normals < v.targets[i].normals;
                    if(targets[i].uvs != v.targets[i].uvs) return targets[i].uvs < v.targets[i].uvs;
                }
                return false;
            }
        };
        struct MeshData
        {
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> uvs;
            std::vector<glm::vec4> joints;
            std::vector<glm::vec4> weights;
            struct TargetAccessors
            {
                std::vector<glm::vec3> positions;
                std::vector<glm::vec3> normals;
                std::vector<glm::vec2> uvs;
            };
            std::vector<TargetAccessors> targets;
        };
        std::map<MeshAccessors, GfxConstRef<GfxMesh>> meshInstances;
        for(size_t i = 0; i < gltf_model->meshes_count; ++i)
        {
            cgltf_mesh const &gltf_mesh = gltf_model->meshes[i];
            std::vector<instance_pair> &mesh_list = meshes[&gltf_mesh];
            for(size_t j = 0; j < gltf_mesh.primitives_count; ++j)
            {
                cgltf_primitive const &gltf_primitive = gltf_mesh.primitives[j];
                if(gltf_primitive.type != cgltf_primitive_type_triangles) continue;    // only support triangle meshes
                GfxConstRef<GfxMesh> current_mesh;
                MeshAccessors accessors;
                accessors.indices = gltf_primitive.indices;
                cgltf_attribute *attributes_end = gltf_primitive.attributes + gltf_primitive.attributes_count;
                cgltf_attribute const *it = std::find_if(gltf_primitive.attributes, attributes_end,
                    [&](const cgltf_attribute& x) { return x.type == cgltf_attribute_type_position; });  // locate position stream
                accessors.positions = it != attributes_end ? it->data : nullptr;
                if(accessors.positions == nullptr) continue; // invalid mesh primitive
                it = std::find_if(gltf_primitive.attributes, attributes_end,
                    [&](const cgltf_attribute& x) { return x.type == cgltf_attribute_type_normal; });  // locate normal stream
                accessors.normals = it != attributes_end ? it->data : nullptr;
                it = std::find_if(gltf_primitive.attributes, attributes_end,
                    [&](const cgltf_attribute& x) { return x.type == cgltf_attribute_type_texcoord; });  // locate uv stream
                accessors.uvs = it != attributes_end ? it->data : nullptr;
                it = std::find_if(gltf_primitive.attributes, attributes_end,
                    [&](const cgltf_attribute& x) { return x.type == cgltf_attribute_type_joints; });  // joints stream
                accessors.joints = it != attributes_end ? it->data : nullptr;
                it = std::find_if(gltf_primitive.attributes, attributes_end,
                    [&](const cgltf_attribute& x) { return x.type == cgltf_attribute_type_weights; });  // weights stream
                accessors.weights = it != attributes_end ? it->data : nullptr;
                accessors.targets.resize(gltf_primitive.targets_count);
                for(cgltf_size k = 0; k < gltf_primitive.targets_count; ++k)
                {
                    cgltf_attribute *target_attributes_end = gltf_primitive.targets[k].attributes + gltf_primitive.targets[k].attributes_count;
                    it = std::find_if(gltf_primitive.targets[k].attributes, target_attributes_end,
                        [&](const cgltf_attribute& x) { return x.type == cgltf_attribute_type_position; });
                    accessors.targets[k].positions = it != target_attributes_end ? it->data : nullptr;
                    it = std::find_if(gltf_primitive.targets[k].attributes, target_attributes_end,
                        [&](const cgltf_attribute& x) { return x.type == cgltf_attribute_type_normal; });
                    accessors.targets[k].normals = it != target_attributes_end ? it->data : nullptr;
                    it = std::find_if(gltf_primitive.targets[k].attributes, target_attributes_end,
                        [&](const cgltf_attribute& x) { return x.type == cgltf_attribute_type_texcoord; });
                    accessors.targets[k].uvs = it != target_attributes_end ? it->data : nullptr;
                }
                auto mesh_it = meshInstances.find(accessors);
                if(mesh_it == meshInstances.end())
                {
                    MeshData mesh_data;
                    bool unpacked;
                    unpacked = UnpackAccessor(accessors.positions, mesh_data.positions); GFX_ASSERT(unpacked);
                    unpacked = UnpackAccessor(accessors.normals, mesh_data.normals); GFX_ASSERT(unpacked);
                    unpacked = UnpackAccessor(accessors.uvs, mesh_data.uvs); GFX_ASSERT(unpacked);
                    unpacked = UnpackAccessor(accessors.joints, mesh_data.joints); GFX_ASSERT(unpacked);
                    unpacked = UnpackAccessor(accessors.weights, mesh_data.weights); GFX_ASSERT(unpacked);
                    mesh_data.targets.resize(accessors.targets.size());
                    for(size_t k = 0; k < accessors.targets.size(); ++k)
                    {
                        unpacked = UnpackAccessor(accessors.targets[k].positions, mesh_data.targets[k].positions); GFX_ASSERT(unpacked);
                        unpacked = UnpackAccessor(accessors.targets[k].normals, mesh_data.targets[k].normals); GFX_ASSERT(unpacked);
                        unpacked = UnpackAccessor(accessors.targets[k].uvs, mesh_data.targets[k].uvs); GFX_ASSERT(unpacked);
                    }
                    bool skinned_mesh = accessors.joints != nullptr;
                    GfxRef<GfxMesh> mesh_ref = gfxSceneCreateMesh(scene);
                    GfxMesh &mesh = *mesh_ref;
                    mesh.default_weights = std::vector<float>(gltf_mesh.weights, gltf_mesh.weights + gltf_mesh.weights_count);
                    auto unpack_vertex = [&](size_t const gltf_index) {
                        GfxVertex vertex = {};
                        vertex.position = mesh_data.positions[gltf_index];
                        if(!mesh_data.normals.empty())
                            vertex.normal = mesh_data.normals[gltf_index];
                        if(!mesh_data.uvs.empty())
                            vertex.uv = mesh_data.uvs[gltf_index];
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
                        for(size_t k = 0; k < mesh_data.targets.size(); ++k)
                        {
                            GfxVertex target_vertex = {};
                            if(!mesh_data.targets[k].positions.empty())
                                target_vertex.position = mesh_data.targets[k].positions[gltf_index];
                            if(!mesh_data.targets[k].normals.empty())
                                target_vertex.normal = mesh_data.targets[k].normals[gltf_index];
                            if(!mesh_data.targets[k].uvs.empty())
                                target_vertex.uv = mesh_data.targets[k].uvs[gltf_index];
                            mesh.morph_targets.push_back(target_vertex);
                        }
                        if(skinned_mesh)
                        {
                            GfxJoint joint = {};
                            if(!mesh_data.joints.empty())
                                joint.joints = mesh_data.joints[gltf_index];
                            if(!mesh_data.weights.empty())
                                joint.weights = mesh_data.weights[gltf_index];
                            mesh.joints.push_back(joint);
                        }
                    };
                    if(accessors.indices != nullptr)
                    {
                        std::map<cgltf_uint, uint32_t> indices;
                        for(size_t k = 0; k < accessors.indices->count; ++k)
                        {
                            cgltf_uint gltf_index = 0;
                            cgltf_bool read = cgltf_accessor_read_uint(accessors.indices, k, &gltf_index, sizeof(cgltf_uint));
                            GFX_ASSERT(read); (void)read;
                            std::map<cgltf_uint, uint32_t>::const_iterator const it2 = indices.find(gltf_index);
                            if(it2 != indices.end())
                                mesh.indices.push_back((*it2).second);
                            else
                            {
                                unpack_vertex(gltf_index);
                                indices[gltf_index] = mesh.indices.back();
                            }
                        }
                    }
                    else
                    {
                        for(size_t k = 0; k < mesh_data.positions.size(); ++k)
                        {
                            unpack_vertex(k);
                        }
                    }
                    GfxMetadata &mesh_metadata = mesh_metadata_[mesh_ref];
                    mesh_metadata.asset_file = asset_file;  // set up metadata
                    mesh_metadata.object_name = (gltf_mesh.name != nullptr) ? gltf_mesh.name : "Mesh" + std::to_string(i);
                    if(j > 0)
                    {
                        mesh_metadata.object_name += ".";
                        mesh_metadata.object_name += std::to_string(j);
                    }
                    current_mesh             = mesh_ref;
                    meshInstances[accessors] = mesh_ref;
                }
                else
                {
                    current_mesh = mesh_it->second;
                }
                GfxConstRef<GfxMaterial> material;
                if(gltf_primitive.material != nullptr)
                {
                    std::map<cgltf_material const *, GfxConstRef<GfxMaterial>>::const_iterator const it2 =
                        materials.find(gltf_primitive.material);
                    if(it2 != materials.end()) material = (*it2).second;
                }
                mesh_list.push_back(std::make_pair(current_mesh, material));
            }
        }
        std::map<cgltf_node const *, std::set<GfxConstRef<GfxAnimation>>> node_animations;
        std::map<cgltf_node const *, uint64_t /*gfx node handle*/>        node_handles;
        std::map<uint64_t /*gltf ID*/, GfxConstRef<GfxAnimation>>         animations;
        for(size_t i = 0; i < gltf_model->animations_count; ++i)
        {
            GfxRef<GfxAnimation> animation_ref;
            GltfAnimation *animation_object = nullptr;
            cgltf_animation const &gltf_animation = gltf_model->animations[i];
            for(size_t j = 0; j < gltf_animation.channels_count; ++j)
            {
                uint64_t animated_node_handle;
                cgltf_animation_channel const &gltf_animation_channel = gltf_animation.channels[j];
                if(gltf_animation_channel.target_node == nullptr) continue;
                GltfAnimationChannelType type = kGltfAnimationChannelType_Count;
                     if(gltf_animation_channel.target_path == cgltf_animation_path_type_translation)
                         type = kGltfAnimationChannelType_Translate;
                else if(gltf_animation_channel.target_path == cgltf_animation_path_type_rotation)
                         type = kGltfAnimationChannelType_Rotate;
                else if(gltf_animation_channel.target_path == cgltf_animation_path_type_scale)
                         type = kGltfAnimationChannelType_Scale;
                else if(gltf_animation_channel.target_path == cgltf_animation_path_type_weights)
                         type = kGltfAnimationChannelType_Weights;
                if(type == kGltfAnimationChannelType_Count) continue;   // unsupported animation channel type
                if(gltf_animation_channel.sampler == nullptr) continue;
                cgltf_animation_sampler const &gltf_animation_sampler = *gltf_animation_channel.sampler;
                GltfAnimationChannelMode mode = kGltfAnimationChannelMode_Count;
                     if(gltf_animation_sampler.interpolation == cgltf_interpolation_type_linear)
                         mode = kGltfAnimationChannelMode_Linear;
                else if(gltf_animation_sampler.interpolation == cgltf_interpolation_type_step)
                         mode = kGltfAnimationChannelMode_Step;
                else if(gltf_animation_sampler.interpolation == cgltf_interpolation_type_cubic_spline)
                         mode = kGltfAnimationChannelMode_Count;  // not supported yet
                if(mode == kGltfAnimationChannelMode_Count) continue;   // unsupported animation channel mode
                cgltf_accessor const *input_buffer  = gltf_animation_sampler.input;
                cgltf_accessor const *output_buffer = gltf_animation_sampler.output;
                if(input_buffer == nullptr || output_buffer == nullptr || input_buffer->count == 0) continue;
                if(input_buffer->is_sparse || output_buffer->is_sparse) continue;
                std::map<cgltf_node const *, uint64_t>::const_iterator const it =
                    node_handles.find(gltf_animation_channel.target_node);
                if(it != node_handles.end())
                    animated_node_handle = (*it).second;
                else
                {
                    animated_node_handle = gltf_node_handles_.allocate_handle();
                    node_handles[gltf_animation_channel.target_node] = animated_node_handle;
                    gltf_animated_nodes_.insert(GetObjectIndex(animated_node_handle));
                }
                if(!animation_ref)
                {
                    animation_ref = gfxSceneCreateAnimation(scene);
                    animations[i] = animation_ref;  // insert into map
                    animation_object = &gltf_animations_.insert(GetObjectIndex(animation_ref));
                    GfxMetadata &animation_metadata = animation_metadata_[animation_ref];
                    animation_metadata.asset_file = asset_file; // set up metadata
                    animation_metadata.object_name = (gltf_animation.name != nullptr) ? gltf_animation.name : "Animation" + std::to_string(i);
                }
                if(type != kGltfAnimationChannelType_Weights)
                    node_animations[gltf_animation_channel.target_node].insert(animation_ref);
                GFX_ASSERT(animation_object != nullptr);
                animation_object->channels_.emplace_back();
                GltfAnimationChannel &animation_channel = animation_object->channels_.back();
                animation_channel.keyframes_.resize(input_buffer->count);
                cgltf_accessor_unpack_floats(input_buffer, &animation_channel.keyframes_[0], input_buffer->count);
                cgltf_size const num_components = cgltf_num_components(output_buffer->type);
                GFX_ASSERT(((type == kGltfAnimationChannelType_Translate) && (num_components == 3)) ||
                    ((type == kGltfAnimationChannelType_Rotate) && (num_components == 4)) ||
                    ((type == kGltfAnimationChannelType_Scale) && (num_components == 3)) ||
                    ((type == kGltfAnimationChannelType_Weights) && (num_components == 1)));
                animation_channel.values_.resize(num_components * output_buffer->count);
                for(uint32_t k = 0; k < output_buffer->count; ++k)
                    cgltf_accessor_read_float(output_buffer, k, (float*)&animation_channel.values_[num_components * k], num_components * sizeof(float));
                animation_channel.node_ = animated_node_handle;
                animation_channel.mode_ = mode;
                animation_channel.type_ = type;
            }
        }
        std::function<uint64_t (cgltf_node const *gltf_node, glm::mat4 const &parent_transform,
            std::vector<GfxConstRef<GfxAnimation>> const &parent_animations, uint64_t parent_handle)> VisitNode;
        VisitNode = [&](cgltf_node const *gltf_node, glm::mat4 const &parent_transform,
            std::vector<GfxConstRef<GfxAnimation>> const &parent_animations, uint64_t parent_handle) -> uint64_t
        {
            glm::mat4 local_transform(1.0); // default to identity
            cgltf_node_transform_local(gltf_node, (float*)&local_transform);
            std::vector<GfxRef<GfxInstance>> instances;
            glm::mat4 const transform = parent_transform * local_transform;
            GfxRef<GfxSkin> skin;
            if(gltf_node->skin != nullptr)
            {
                std::map<cgltf_skin const *, GfxConstRef<GfxSkin>>::const_iterator const it = skins.find(gltf_node->skin);
                if(it != skins.end())
                {
                    skin = (*it).second;
                }
            }
            if(gltf_node->mesh != nullptr)
            {
                std::map<cgltf_mesh const *, std::vector<instance_pair>>::const_iterator const it = meshes.find(gltf_node->mesh);
                if(it != meshes.end())
                {
                    instances.reserve((*it).second.size());
                    for(size_t i = 0; i < (*it).second.size(); ++i)
                    {
                        GfxRef<GfxInstance> instance_ref = gfxSceneCreateInstance(scene);
                        instances.push_back(instance_ref);
                        GfxConstRef<GfxMesh> mesh = it->second[i].first;
                        instance_ref->mesh      = mesh;
                        instance_ref->material  = it->second[i].second;
                        instance_ref->skin      = skin;
                        instance_ref->transform = glm::mat4(transform);
                        if(!mesh->morph_targets.empty())
                        {
                            if(gltf_node->weights != nullptr)
                                instance_ref->weights = std::vector<float>(gltf_node->weights, gltf_node->weights + gltf_node->weights_count);
                            else if(!mesh->default_weights.empty())
                                instance_ref->weights = mesh->default_weights;
                            else
                                instance_ref->weights = std::vector<float>(mesh->morph_targets.size() / mesh->vertices.size(), 0.0f);
                        }
                        GfxMetadata &instance_metadata = instance_metadata_[instance_ref];
                        GfxMetadata const *mesh_metadata = mesh_metadata_.at(it->second[i].first);
                        if(mesh_metadata != nullptr)
                            instance_metadata = *mesh_metadata; // set up metadata
                        else
                            instance_metadata.asset_file = asset_file;
                    }
                }
            }
            GfxRef<GfxCamera> camera;
            if(gltf_node->camera != nullptr)
            {
                std::map<cgltf_camera const *, GfxConstRef<GfxCamera>>::const_iterator const it = cameras.find(gltf_node->camera);
                if(it != cameras.end())
                {
                    camera = (*it).second;
                    TransformGltfCamera(*camera, transform);
                }
            }
            GfxRef<GfxLight> light;
            if(gltf_node->light != nullptr)
            {
                std::map<cgltf_light const *, GfxConstRef<GfxLight>>::const_iterator const it = lights.find(gltf_node->light);
                if(it != lights.end())
                {
                    light = (*it).second;
                    TransformGltfLight(*light, transform);
                }
            }
            std::map<cgltf_node const *, uint64_t>::const_iterator const node_it = node_handles.find(gltf_node);
            uint64_t node_handle;
            if(node_it != node_handles.end())
            {
                node_handle = (*node_it).second;
                GltfAnimatedNode *animated_node = gltf_animated_nodes_.at(GetObjectIndex(node_handle));
                GFX_ASSERT(animated_node != nullptr);
                animated_node->translate_ = local_transform[3];
                for(int32_t i = 0; i < 3; i++)
                    animated_node->scale_[i] = glm::length(glm::dvec3(local_transform[i]));
                const glm::dmat3 rotMtx(glm::dvec3(local_transform[0]) / animated_node->scale_[0],
                    glm::dvec3(local_transform[1]) / animated_node->scale_[1],
                    glm::dvec3(local_transform[2]) / animated_node->scale_[2]);
                animated_node->rotate_ = glm::quat_cast(rotMtx);
            }
            else
            {
                GFX_ASSERT(node_animations.find(gltf_node) == node_animations.end());
                node_handle = gltf_node_handles_.allocate_handle();
                node_handles[gltf_node] = node_handle;
            }
            std::vector<GfxConstRef<GfxAnimation>> propagate_parent_animations;
            auto const node_animations_it = node_animations.find(gltf_node);
            if(node_animations_it != node_animations.end())
            {
                std::vector<GfxConstRef<GfxAnimation>> new_animations;
                new_animations.reserve(node_animations_it->second.size());
                std::set_difference(node_animations_it->second.begin(), node_animations_it->second.end(),
                    parent_animations.begin(), parent_animations.end(), std::back_inserter(new_animations));
                for(GfxConstRef<GfxAnimation> animation_ref : new_animations)
                {
                    GltfAnimation &animation_object = *gltf_animations_.at(GetObjectIndex(animation_ref));
                    animation_object.animated_root_nodes_.push_back(node_handle);
                }
                propagate_parent_animations.reserve(parent_animations.size() + new_animations.size());
                std::merge(parent_animations.begin(), parent_animations.end(),
                    new_animations.begin(), new_animations.end(),
                    std::back_inserter(propagate_parent_animations)); // "new_animations" are sorted according to the STL documentation
            }
            else
            {
                propagate_parent_animations = parent_animations;
            }
            std::vector<uint64_t> children;
            children.reserve(gltf_node->children_count);
            for(size_t i = 0; i < gltf_node->children_count; ++i)
            {
                if(gltf_node->children[i] == nullptr) continue;
                uint64_t child_handle = VisitNode(gltf_node->children[i], transform, propagate_parent_animations, node_handle);
                children.push_back(child_handle);
            }
            GltfNode &node = gltf_nodes_.insert(GetObjectIndex(node_handle));
            node.default_local_transform_ = local_transform;
            node.world_transform_ = transform;
            node.parent_ = parent_handle;
            std::swap(node.children_, children);
            std::swap(node.instances_, instances);
            node.skin_ = skin;
            node.camera_ = camera;
            node.light_ = light;
            node.default_weights_ = std::vector<float>(gltf_node->weights, gltf_node->weights + gltf_node->weights_count);
            return node_handle;
        };
        cgltf_scene const &gltf_scene = gltf_model->scene != nullptr ? *gltf_model->scene : gltf_model->scenes[0];
        scene_gltf_nodes_.reserve(gltf_scene.nodes_count);
        for(size_t i = 0; i < gltf_scene.nodes_count; ++i)
        {
            if(gltf_scene.nodes[i] == nullptr) continue;
            uint64_t node_handle = VisitNode(gltf_scene.nodes[i], glm::mat4(1.0), {}, 0);
            scene_gltf_nodes_.push_back(node_handle);
        }
        for(cgltf_size i = 0; i < gltf_model->skins_count; ++i)
        {
            cgltf_skin const &gltf_skin = gltf_model->skins[i];
            std::map<cgltf_skin const *, GfxConstRef<GfxSkin>>::const_iterator const it = skins.find(&gltf_skin);
            if(it == skins.end()) continue;
            GltfSkin &skin = gltf_skins_.insert(GetObjectIndex((*it).second));
            skin.joints_.resize(gltf_skin.joints_count);
            for(cgltf_size j = 0; j < gltf_skin.joints_count; ++j)
            {
                skin.joints_[j] = node_handles.at(gltf_skin.joints[j]);
            }
            skin.inverse_bind_matrices_.resize(gltf_skin.inverse_bind_matrices->count);
            assert(gltf_skin.inverse_bind_matrices->count * sizeof(glm::mat4)
                   == gltf_skin.inverse_bind_matrices->buffer_view->size);
            memcpy(skin.inverse_bind_matrices_.data(),
                (uint8_t *)gltf_skin.inverse_bind_matrices->buffer_view->buffer->data
                    + gltf_skin.inverse_bind_matrices->buffer_view->offset,
                gltf_skin.inverse_bind_matrices->buffer_view->size);
        }
        for(auto const &animation : animations)
        {
            GfxRef<GfxAnimation> animation_ref = animation.second;
            GltfAnimation &animation_object = *gltf_animations_.at(GetObjectIndex(animation_ref));
            std::set<uint64_t> animated_nodes;
            for(auto const &channel : animation_object.channels_)
                animated_nodes.insert(channel.node_);
            std::set<uint64_t> dependent_skinned_nodes;
            animation_object.dependent_skins_.reserve(gltf_model->skins_count);
            for(cgltf_size i = 0; i < gltf_model->skins_count; ++i)
            {
                GltfSkin &skin = *gltf_skins_.at((std::uint32_t)i);
                cgltf_skin const &gltf_skin = gltf_model->skins[i];
                std::map<cgltf_skin const *, GfxConstRef<GfxSkin>>::const_iterator const it = skins.find(&gltf_skin);
                if(it == skins.end()) continue;
                for(cgltf_size j = 0; j < gltf_skin.joints_count; ++j)
                {
                    if(animated_nodes.find(skin.joints_[j]) != animated_nodes.end())
                    {
                        animation_object.dependent_skins_.push_back(it->second);
                        break;
                    }
                }
            }
        }
        for(auto const it : node_handles)
        {
            uint64_t const node_handle = it.second;
            uint32_t const node_index = GetObjectIndex(node_handle);
            if(gltf_nodes_.has(node_index)) continue;
            if(gltf_animated_nodes_.has(node_index))
                gltf_animated_nodes_.erase(node_index);
            gltf_node_handles_.free_handle(node_handle);
        }
        cgltf_free(gltf_model);
        return kGfxResult_NoError;
    }

    GfxResult importHdr(GfxScene const &scene, char const *asset_file)
    {
        GFX_ASSERT(asset_file != nullptr);
        int32_t image_width, image_height, channel_count;
        if(gfxSceneFindObjectByAssetFile<GfxImage>(scene, asset_file))
            return kGfxResult_NoError;  // image was already imported
        float *image_data = stbi_loadf(asset_file, &image_width, &image_height, &channel_count, 0);
        if(image_data == nullptr)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Unable to load image `%s': %s", asset_file, stbi_failure_reason());
        uint32_t const resolved_channel_count = (uint32_t)(channel_count != 3 ? channel_count : 4);
        char const *file = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '\\'));
        file = (file == nullptr ? asset_file : file + 1);   // retrieve file name
        size_t const image_data_size = (size_t)image_width * image_height * resolved_channel_count * sizeof(float);
        GfxRef<GfxImage> image_ref = gfxSceneCreateImage(scene);
        image_ref->data.resize(image_data_size);
        image_ref->width = (uint32_t)image_width;
        image_ref->height = (uint32_t)image_height;
        image_ref->channel_count = resolved_channel_count;
        image_ref->bytes_per_channel = (uint32_t)4;
        image_ref->format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        float *data = (float *)image_ref->data.data();
        bool alpha_check = false;   // check alpha
        for(int32_t y = 0; y < image_height; ++y)
            for(int32_t x = 0; x < image_width; ++x)
                for(int32_t k = 0; k < (int32_t)resolved_channel_count; ++k)
                {
                    int32_t const dst_index = (int32_t)resolved_channel_count * (x + (image_height - y - 1) * image_width) + k;
                    int32_t const src_index = channel_count * (x + y * image_width) + k;
                    float source = (k < channel_count ? image_data[src_index] : 1.0f);
                    if(k == 3)
                    {
                        source = glm::clamp(source, 0.0f, 1.0f);
                        alpha_check |= source < 1.0f;
                    }
                    else
                    {
                        source /= (1.0f + source);  // fix NaNs
                        source  = glm::clamp(source, 0.0f, 1.0f);
                        source /= GFX_MAX(1.0f - source, 1e-3f);
                    }
                    data[dst_index] = source;
                }
        image_ref->flags = (!alpha_check ? 0 : kGfxImageFlag_HasAlphaChannel);
        GfxMetadata &image_metadata = image_metadata_[image_ref];
        image_metadata.asset_file = asset_file; // set up metadata
        image_metadata.object_name = file;
        stbi_image_free(image_data);
        return kGfxResult_NoError;
    }

    GfxResult importDds(GfxScene const &scene, char const *asset_file)
    {
        GFX_ASSERT(asset_file != nullptr);
        if(gfxSceneFindObjectByAssetFile<GfxImage>(scene, asset_file))
            return kGfxResult_NoError;  // image was already imported

        struct DDSHeader
        {
            uint32_t size;
            enum class Flags : uint32_t
            {
                Caps        = 0x1,
                Height      = 0x2,
                Width       = 0x4,
                Pitch       = 0x8,
                PixelFormat = 0x1000,
                Texture     = Caps | Height | Width | PixelFormat,
                Mipmap      = 0x20000,
                LinearSize  = 0x80000,
                Depth      = 0x800000,
            } flags;
            uint32_t height;
            uint32_t width;
            uint32_t pitchOrLinearSize;
            uint32_t depth;
            uint32_t mipMapCount;
            uint32_t reserved[11];
            struct PixelFormat
            {
                uint32_t size;
                enum Flags : uint32_t
                {
                    AlphaPixels   = 0x1,
                    Alpha         = 0x2,
                    FourCC        = 0x4,
                    PAL8          = 0x20,
                    RGB           = 0x40,
                    RGBA          = RGB | AlphaPixels,
                    YUV           = 0x200,
                    Luminance     = 0x20000,
                    LuminanceA    = Luminance | AlphaPixels,
                    BumpLuminance = 0x00040000,
                    BumpDuDv      = 0x00080000
                } flags;
                uint32_t fourCC;
                uint32_t bitCount;
                uint32_t rBitMask;
                uint32_t gBitMask;
                uint32_t bBitMask;
                uint32_t aBitMask;
            } pixelFormat;
            uint32_t caps1;
            enum class Caps2Flags : uint32_t
            {
                CubemapPositiveX = 0x00000600,
                CubemapNegativeX = 0x00000a00,
                CubemapPositiveY = 0x00001200,
                CubemapNegativeY = 0x00002200,
                CubemapPositiveZ = 0x00004200,
                CubemapNegativeZ = 0x00008200,
                CubemapAllFaces  = CubemapPositiveX | CubemapNegativeX | CubemapPositiveY | CubemapNegativeY
                                | CubemapPositiveZ | CubemapNegativeZ,
                Volume = 0x00200000,
            } caps2;
            uint32_t caps3;
            uint32_t caps4;
            uint32_t reserved2;
        };

        struct DDSHeaderDX10
        {
            uint32_t DXGIFormat;
            enum class TextureDimension : uint32_t
            {
                Unknown   = 0,
                Texture1D = 2,
                Texture2D = 3,
                Texture3D = 4
            } resourceDimension;
            enum class MiscFlags : uint32_t
            {
                Unknown     = 0,
                TextureCube = 0x4
            } miscFlag;
            uint32_t arraySize;
            uint32_t reserved;
        };

#define MAKE_FOURCC(char1, char2, char3, char4)                 \
    (static_cast<uint32_t>(char1) | (static_cast<uint32_t>(char2) << 8) \
    | (static_cast<uint32_t>(char3) << 16) | (static_cast<uint32_t>(char4) << 24))

        // Open file
        std::FILE *file = std::fopen(asset_file, "rb");
        if(!file)
        {
            return GFX_SET_ERROR(
                kGfxResult_InvalidOperation, "Unable to load image `%s' : File not found", asset_file);
        }
        // Check DDS magic number identifier
        uint32_t magic;
        if((std::fread(&magic, sizeof(uint32_t), 1, file) != sizeof(uint32_t))
            || (magic != MAKE_FOURCC('D', 'D', 'S', ' ')))
        {
            std::fclose(file);
            return GFX_SET_ERROR(
                kGfxResult_InvalidOperation, "Unable to load image `%s' : Invalid dds file", asset_file);
        }

        // Read and validate header
        DDSHeader header;
        if(std::fread(&header, sizeof(DDSHeader), 1, file) != sizeof(DDSHeader)
            || header.size != sizeof(DDSHeader) || header.pixelFormat.size != sizeof(DDSHeader::PixelFormat))
        {
            std::fclose(file);
            return GFX_SET_ERROR(
                kGfxResult_InvalidOperation, "Unable to load image `%s' : Invalid dds file", asset_file);
        }

        uint32_t width  = header.width;
        uint32_t height = header.height;
        uint32_t depth  = 1;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        uint32_t numDimensions = 0;
        uint32_t mipCount  = header.mipMapCount;
        uint32_t arraySize = 1;
        bool     cubeMap   = false;

        // Check for DX10 header
        if((header.pixelFormat.flags & DDSHeader::PixelFormat::FourCC)
            && (header.pixelFormat.fourCC == MAKE_FOURCC('D', 'X', '1', '0')))
        {
            DDSHeaderDX10 header10;
            if(std::fread(&header10, sizeof(DDSHeaderDX10), 1, file) != sizeof(DDSHeaderDX10))
            {
                std::fclose(file);
                return GFX_SET_ERROR(
                    kGfxResult_InvalidOperation, "Unable to load image `%s' : Invalid dds10 file", asset_file);
            }
            format = (DXGI_FORMAT)header10.DXGIFormat;
            if(header10.arraySize != 0)
            {
                arraySize = header10.arraySize;
            }
            switch(header10.resourceDimension)
            {
            case DDSHeaderDX10::TextureDimension::Texture1D:
                if((static_cast<uint32_t>(header.flags) & static_cast<uint32_t>(DDSHeader::Flags::Height) && (height != 1)))
                {
                    std::fclose(file);
                    return GFX_SET_ERROR(kGfxResult_InvalidOperation,
                        "Unable to load image `%s' : Invalid dds header", asset_file);
                }
                height = depth = 1;
                numDimensions  = 1;
                break;
            case DDSHeaderDX10::TextureDimension::Texture2D:
                if(static_cast<uint32_t>(header10.miscFlag) & static_cast<uint32_t>(DDSHeaderDX10::MiscFlags::TextureCube))
                {
                    arraySize *= 6;
                    cubeMap = true;
                }
                depth         = 1;
                numDimensions = 2;
                break;
            case DDSHeaderDX10::TextureDimension::Texture3D:
                if(!(static_cast<uint32_t>(header.flags) & static_cast<uint32_t>(DDSHeader::Flags::Depth)))
                {
                    std::fclose(file);
                    return GFX_SET_ERROR(kGfxResult_InvalidOperation,
                        "Unable to load image `%s' : Invalid dds header", asset_file);
                }
                numDimensions = 3;
                break;
            }

        }

        if(format == DXGI_FORMAT_UNKNOWN || format > DXGI_FORMAT_B4G4R4A4_UNORM)
        {
            if(header.pixelFormat.flags & DDSHeader::PixelFormat::Flags::RGB)
            {
                if(header.pixelFormat.bitCount == 32)
                {
                    if(header.pixelFormat.rBitMask == 0x000000ff && header.pixelFormat.gBitMask == 0x0000ff00
                        && header.pixelFormat.bBitMask == 0x00ff0000
                        && header.pixelFormat.aBitMask == 0xff000000)
                    {
                        format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    }
                    else if(header.pixelFormat.rBitMask == 0x00ff0000
                        && header.pixelFormat.gBitMask == 0x0000ff00
                        && header.pixelFormat.bBitMask == 0x000000ff
                        && header.pixelFormat.aBitMask == 0xff000000)
                    {
                        format = DXGI_FORMAT_B8G8R8A8_UNORM;
                    }
                    else if(header.pixelFormat.rBitMask == 0x00ff0000
                        && header.pixelFormat.gBitMask == 0x0000ff00
                        && header.pixelFormat.bBitMask == 0x000000ff
                        && header.pixelFormat.aBitMask == 0x00000000)
                    {
                        format = DXGI_FORMAT_B8G8R8X8_UNORM;
                    }
                    else if(header.pixelFormat.rBitMask == 0x000003ff
                        && header.pixelFormat.gBitMask == 0x000ffc00
                        && header.pixelFormat.bBitMask == 0x3ff00000
                        && header.pixelFormat.aBitMask == 0x00000000)
                    {
                        format = DXGI_FORMAT_R10G10B10A2_UNORM;
                    }
                    else if(header.pixelFormat.rBitMask == 0x0000ffff
                        && header.pixelFormat.gBitMask == 0xffff0000
                        && header.pixelFormat.bBitMask == 0x00000000
                        && header.pixelFormat.aBitMask == 0x00000000)
                    {
                        format = DXGI_FORMAT_R16G16_UNORM;
                    }
                    else if(header.pixelFormat.rBitMask == 0xffffffff
                        && header.pixelFormat.gBitMask == 0x00000000
                        && header.pixelFormat.bBitMask == 0x00000000
                        && header.pixelFormat.aBitMask == 0x00000000)
                    {
                        format = DXGI_FORMAT_R32_FLOAT;
                    }
                }
                else if(header.pixelFormat.bitCount == 16)
                {
                    if(header.pixelFormat.rBitMask == 0x7c00
                        && header.pixelFormat.gBitMask == 0x03e0
                        && header.pixelFormat.bBitMask == 0x001f
                        && header.pixelFormat.aBitMask == 0x8000)
                    {
                        format = DXGI_FORMAT_B5G5R5A1_UNORM;
                    }
                    else if(header.pixelFormat.rBitMask == 0xf800
                        && header.pixelFormat.gBitMask == 0x07e0
                        && header.pixelFormat.bBitMask == 0x001f
                        && header.pixelFormat.aBitMask == 0x0000)
                    {
                        format = DXGI_FORMAT_B5G6R5_UNORM;
                    }
                    else if(header.pixelFormat.rBitMask == 0x0f00
                        && header.pixelFormat.gBitMask == 0x00f0
                        && header.pixelFormat.bBitMask == 0x000f
                        && header.pixelFormat.aBitMask == 0xf000)
                    {
                        format = DXGI_FORMAT_B4G4R4A4_UNORM;
                    }
                }
            }
            else if(header.pixelFormat.flags & DDSHeader::PixelFormat::Flags::Luminance)
            {
                if(header.pixelFormat.bitCount == 8)
                {
                    if(header.pixelFormat.rBitMask == 0x000000ff
                        && header.pixelFormat.gBitMask == 0x00000000
                        && header.pixelFormat.bBitMask == 0x00000000
                        && header.pixelFormat.aBitMask == 0x00000000)
                    {
                        format = DXGI_FORMAT_R8_UNORM;
                    }
                }
                else if(header.pixelFormat.bitCount == 16)
                {
                    if(header.pixelFormat.rBitMask == 0x0000ffff
                        && header.pixelFormat.gBitMask == 0x00000000
                        && header.pixelFormat.bBitMask == 0x00000000
                        && header.pixelFormat.aBitMask == 0x00000000)
                    {
                        format = DXGI_FORMAT_R16_UNORM;
                    }
                    else if(header.pixelFormat.rBitMask == 0x000000ff
                        && header.pixelFormat.gBitMask == 0x0000ff00
                        && header.pixelFormat.bBitMask == 0x00000000
                        && header.pixelFormat.aBitMask == 0x00000000)
                    {
                        format = DXGI_FORMAT_R8G8_UNORM;
                    }
                }
            }
            else if(header.pixelFormat.flags & DDSHeader::PixelFormat::Flags::Alpha)
            {
                if(header.pixelFormat.bitCount == 8)
                {
                    format = DXGI_FORMAT_A8_UNORM;
                }
            }
            else if(header.pixelFormat.flags & DDSHeader::PixelFormat::Flags::BumpDuDv)
            {
                if(header.pixelFormat.bitCount == 16)
                {
                    if(header.pixelFormat.rBitMask == 0x00ff
                        && header.pixelFormat.gBitMask == 0xff00
                        && header.pixelFormat.bBitMask == 0x0000
                        && header.pixelFormat.aBitMask == 0x0000)
                    {
                        format = DXGI_FORMAT_R8G8_SNORM;
                    }
                }
                else if(header.pixelFormat.bitCount == 32)
                {
                    if(header.pixelFormat.rBitMask == 0x000000ff
                        && header.pixelFormat.gBitMask == 0x0000ff00
                        && header.pixelFormat.bBitMask == 0x00ff0000
                        && header.pixelFormat.aBitMask == 0xff000000)
                    {
                        format = DXGI_FORMAT_R8G8B8A8_SNORM;
                    }
                    else if(header.pixelFormat.rBitMask == 0x0000ffff
                        && header.pixelFormat.gBitMask == 0xffff0000
                        && header.pixelFormat.bBitMask == 0x00000000
                        && header.pixelFormat.aBitMask == 0x00000000)
                    {
                        format = DXGI_FORMAT_R16G16_SNORM;
                    }
                }
            }
            else if(header.pixelFormat.flags & DDSHeader::PixelFormat::Flags::BumpLuminance)
            {
                if(header.pixelFormat.bitCount == 8)
                {
                    format = DXGI_FORMAT_R8_SINT;
                }
                else if(header.pixelFormat.bitCount == 16)
                {
                    format = DXGI_FORMAT_R16_SINT;
                }
                else if(header.pixelFormat.bitCount == 32)
                {
                    format = DXGI_FORMAT_R32_SINT;
                }
            }
            else if(header.pixelFormat.flags & DDSHeader::PixelFormat::Flags::FourCC)
            {
                if(header.pixelFormat.fourCC == MAKE_FOURCC('D', 'X', 'T', '1'))
                {
                    format = DXGI_FORMAT_BC1_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('D', 'X', 'T', '3'))
                {
                    format = DXGI_FORMAT_BC2_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('D', 'X', 'T', '5'))
                {
                    format = DXGI_FORMAT_BC3_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('D', 'X', 'T', '4'))
                {
                    format = DXGI_FORMAT_BC2_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('D', 'X', 'T', '5'))
                {
                    format = DXGI_FORMAT_BC3_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('A', 'T', 'I', '1'))
                {
                    format = DXGI_FORMAT_BC4_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('B', 'C', '4', 'U'))
                {
                    format = DXGI_FORMAT_BC4_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('B', 'C', '4', 'S'))
                {
                    format = DXGI_FORMAT_BC4_SNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('A', 'T', 'I', '2'))
                {
                    format = DXGI_FORMAT_BC5_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('B', 'C', '5', 'U'))
                {
                    format = DXGI_FORMAT_BC5_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('B', 'C', '5', 'S'))
                {
                    format = DXGI_FORMAT_BC5_SNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('R', 'G', 'B', 'G'))
                {
                    format = DXGI_FORMAT_R8G8_B8G8_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('G', 'R', 'G', 'B'))
                {
                    format = DXGI_FORMAT_G8R8_G8B8_UNORM;
                }
                else if(header.pixelFormat.fourCC == MAKE_FOURCC('Y', 'U', 'Y', '2'))
                {
                    format = DXGI_FORMAT_YUY2;
                }
                else
                {
                    switch(header.pixelFormat.fourCC)
                    {
                    case 36: format = DXGI_FORMAT_R16G16B16A16_UNORM; break;
                    case 110: format = DXGI_FORMAT_R16G16B16A16_SNORM; break;
                    case 111: format = DXGI_FORMAT_R16_FLOAT; break;
                    case 112: format = DXGI_FORMAT_R16G16_FLOAT; break;
                    case 113: format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
                    case 114: format = DXGI_FORMAT_R32_FLOAT; break;
                    case 115: format = DXGI_FORMAT_R32G32_FLOAT; break;
                    case 116: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
                    }
                }
            }
        }

        if(format == DXGI_FORMAT_UNKNOWN)
        {
            std::fclose(file);
            return GFX_SET_ERROR(
                kGfxResult_InvalidOperation, "Unable to load image `%s' : Unknown or unsupported image format", asset_file);
        }

        if(numDimensions == 0)
        {
            if(static_cast<uint32_t>(header.flags) & static_cast<uint32_t>(DDSHeader::Flags::Depth))
            {
                numDimensions = 3;
            }
            else
            {
                auto caps2 = static_cast<uint32_t>(header.caps2)
                           & static_cast<uint32_t>(DDSHeader::Caps2Flags::CubemapAllFaces);
                if(caps2)
                {
                    if(caps2 != static_cast<uint32_t>(DDSHeader::Caps2Flags::CubemapAllFaces))
                    {
                        std::fclose(file);
                        return GFX_SET_ERROR(kGfxResult_InvalidOperation,
                            "Unable to load image `%s' : Partial cube maps are not supported", asset_file);
                    }
                    arraySize = 6;
                    cubeMap   = true;
                }
                depth  = 1;
                numDimensions = height > 1 ? 2 : 1;
            }
        }

        if(numDimensions == 3 && (cubeMap || arraySize > 1))
        {
            std::fclose(file);
            return GFX_SET_ERROR(kGfxResult_InvalidOperation,
                "Unable to load image `%s' : 3D textures cannot have arrays or cube maps", asset_file);
        }

        if(numDimensions != 2 || cubeMap || arraySize > 1)
        {
            std::fclose(file);
            return GFX_SET_ERROR(kGfxResult_InvalidOperation,
                "Unable to load image `%s' : Only 2D textures are supported", asset_file);
        }

        uint32_t bitsPerPixel = GetBitsPerPixel(format);
        auto     GetImageSize = [bitsPerPixel, format](uint32_t width, uint32_t height) {
            uint32_t numBytes;
            uint32_t rowBytes;
            if(format >= DXGI_FORMAT_BC1_TYPELESS && format <= DXGI_FORMAT_BC7_UNORM_SRGB)
            {
                uint32_t numBlocksWide = GFX_MAX(1U, (width + 3) / 4);
                uint32_t numBlocksHigh = GFX_MAX(1U, (height + 3) / 4);
                rowBytes               = numBlocksWide * bitsPerPixel * 2;
                numBytes               = rowBytes * numBlocksHigh;
            }
            else if(format == DXGI_FORMAT_R8G8_B8G8_UNORM || format == DXGI_FORMAT_G8R8_G8B8_UNORM
                || format == DXGI_FORMAT_YUY2 || format == DXGI_FORMAT_Y210
                || format == DXGI_FORMAT_Y216)
            {
                rowBytes = ((width + 1) >> 1) * (bitsPerPixel / 8);
                numBytes = rowBytes * height;
            }
            else if(format == DXGI_FORMAT_NV11)
            {
                rowBytes = ((width + 3) >> 2) * 4;
                numBytes = rowBytes + height * 2;
            }
            else if(format == DXGI_FORMAT_NV12 || format == DXGI_FORMAT_420_OPAQUE
                || format == DXGI_FORMAT_P010 || format == DXGI_FORMAT_P016)
            {
                rowBytes = ((width + 1) >> 1) * (bitsPerPixel / 6);
                numBytes = (rowBytes * height) + ((rowBytes * height + 1) >> 1);
            }
            else
            {
                rowBytes = (width * bitsPerPixel + 7) / 8;
                numBytes = rowBytes * height;
            }
            return numBytes;
        };
        GfxRef<GfxImage> image_ref = gfxSceneCreateImage(scene);
        size_t           dataSize  = GetImageSize(width, height) * arraySize;
        if(mipCount > 1)
        {
            dataSize = ((dataSize * 4) / 3) + 32;
        }
        image_ref->data.resize(dataSize);
        image_ref->width             = width;
        image_ref->height            = height;
        image_ref->channel_count     = GetNumChannels(format);
        image_ref->bytes_per_channel = GFX_MAX((bitsPerPixel / 8) / image_ref->channel_count, 1U);
        image_ref->format            = format;
        uint8_t *data                = image_ref->data.data();
        for(uint32_t j = 0; j < arraySize; j++)
        {
            uint32_t mipWidth = width;
            uint32_t mipHeight = height;
            uint32_t mipDepth = depth;
            for(uint32_t i = 0; i < mipCount; i++)
            {
                uint32_t numBytes = GetImageSize(mipWidth, mipHeight) * mipDepth;
                if(std::fread(data, numBytes, 1, file) != sizeof(numBytes))
                {
                    std::fclose(file);
                    return GFX_SET_ERROR(kGfxResult_InvalidOperation,
                        "Unable to load image `%s' : Corrupted image file", asset_file);
                }
                data += numBytes;
                mipWidth = GFX_MAX(1U, mipWidth / 2);
                mipHeight = GFX_MAX(1U, mipHeight / 2);
                mipDepth = GFX_MAX(1U, mipDepth / 2);
            }
        }
        image_ref->flags = (image_ref->channel_count != 4 ? 0 : kGfxImageFlag_HasAlphaChannel);
        image_ref->flags |= (mipCount > 1 ? kGfxImageFlag_HasMipLevels : 0);
        GfxMetadata &image_metadata = image_metadata_[image_ref];
        image_metadata.asset_file = asset_file; // set up metadata
        char const *file_name = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '\\'));
        file_name = (file_name == nullptr ? asset_file : file_name + 1); // retrieve file name
        image_metadata.object_name = file_name;
        return kGfxResult_NoError;
    }

    static inline KTX_error_code IterateKtxImage(int32_t, int32_t, int32_t, int32_t, int32_t,
        ktx_uint64_t faceLodSize, void *pixels, void *userdata)
    {
        GfxRef<GfxImage> *image_ref = (GfxRef<GfxImage> *)userdata;
        size_t const size = (*image_ref)->data.size();
        (*image_ref)->data.resize(size + faceLodSize);
        uint8_t *back = &((*image_ref)->data[size]);
        memcpy(back, pixels, faceLodSize);
        return KTX_SUCCESS;
    }

    GfxResult importKtx(GfxScene const& scene, char const *asset_file)
    {
        GFX_ASSERT(asset_file != nullptr);
        if(gfxSceneFindObjectByAssetFile<GfxImage>(scene, asset_file))
            return kGfxResult_NoError;  // image was already imported
        ktxTexture2 *ktx_texture;
        KTX_error_code result;
        result = ktxTexture2_CreateFromNamedFile(asset_file, KTX_TEXTURE_CREATE_NO_FLAGS, &ktx_texture);
        if(result != KTX_SUCCESS)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Unable to open image `%s': %s", asset_file, ktxErrorString(result));
        if(ktx_texture->numDimensions != 2)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Only 2D textures are supported `%s'", asset_file);
        char const *file = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '\\'));
        file = (file == nullptr ? asset_file : file + 1);   // retrieve file name
        GfxRef<GfxImage> image_ref = gfxSceneCreateImage(scene);
        auto vk_to_dxgi = [](const uint32_t vk_format) -> DXGI_FORMAT {
            switch(vk_format)
            {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_UINT: return DXGI_FORMAT_R8_UNORM;
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_UINT: return DXGI_FORMAT_R8G8_UNORM;
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_UINT: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case VK_FORMAT_R8G8B8A8_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case VK_FORMAT_R16_UNORM:
            case VK_FORMAT_R16_UINT: return DXGI_FORMAT_R16_UNORM;
            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_UINT: return DXGI_FORMAT_R16G16_UNORM;
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_UINT: return DXGI_FORMAT_R16G16B16A16_UNORM;
            case VK_FORMAT_R32_SFLOAT: return DXGI_FORMAT_R32_FLOAT;
            case VK_FORMAT_R32G32_SFLOAT: return DXGI_FORMAT_R32G32_FLOAT;
            case VK_FORMAT_R32G32B32_SFLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;
            case VK_FORMAT_R32G32B32A32_SFLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return DXGI_FORMAT_BC1_UNORM;
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return DXGI_FORMAT_BC1_UNORM;
            case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return DXGI_FORMAT_BC1_UNORM;
            case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return DXGI_FORMAT_BC1_UNORM_SRGB;
            case VK_FORMAT_BC2_UNORM_BLOCK: return DXGI_FORMAT_BC2_UNORM;
            case VK_FORMAT_BC2_SRGB_BLOCK: return DXGI_FORMAT_BC2_UNORM_SRGB;
            case VK_FORMAT_BC3_UNORM_BLOCK: return DXGI_FORMAT_BC3_UNORM;
            case VK_FORMAT_BC3_SRGB_BLOCK: return DXGI_FORMAT_BC3_UNORM_SRGB;
            case VK_FORMAT_BC4_UNORM_BLOCK: return DXGI_FORMAT_BC4_UNORM;
            case VK_FORMAT_BC5_UNORM_BLOCK: return DXGI_FORMAT_BC5_UNORM;
            case VK_FORMAT_BC6H_UFLOAT_BLOCK: return DXGI_FORMAT_BC6H_UF16;
            case VK_FORMAT_BC6H_SFLOAT_BLOCK: return DXGI_FORMAT_BC6H_SF16;
            case VK_FORMAT_BC7_UNORM_BLOCK: return DXGI_FORMAT_BC7_UNORM;
            case VK_FORMAT_BC7_SRGB_BLOCK: return DXGI_FORMAT_BC7_UNORM_SRGB;
            case VK_FORMAT_UNDEFINED:
            default: return DXGI_FORMAT_UNKNOWN;
            }
        };
        image_ref->data.resize(0);
        image_ref->data.reserve(ktxTexture_GetDataSizeUncompressed((ktxTexture*)ktx_texture));
        image_ref->channel_count = ktxTexture2_GetNumComponents(ktx_texture);
        image_ref->bytes_per_channel = 1; //basisu only support 8bit
        if(ktxTexture2_NeedsTranscoding(ktx_texture))
        {
            // Need to determine number of texture channels
            ktx_transcode_fmt_e tf;
            switch(image_ref->channel_count)
            {
            case 1: tf = KTX_TTF_BC4_R; break;
            case 2: tf = KTX_TTF_BC5_RG; break;
            case 3: tf = KTX_TTF_BC1_RGB; break; //TODO: Use BC7 once transcoding correctly handles setting alpha
            default: tf = KTX_TTF_BC3_RGBA; break;
            }
            result = ktxTexture2_TranscodeBasis(ktx_texture, tf, KTX_TF_HIGH_QUALITY);
            if(result != KTX_SUCCESS)
                return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Unable to transcode image `%s': %s", asset_file, ktxErrorString(result));
            image_ref->data.reserve(ktxTexture_GetDataSize((ktxTexture*)ktx_texture));
            result = ktxTexture_IterateLevelFaces((ktxTexture*)ktx_texture, &IterateKtxImage, &image_ref);
        }
        else
        {
            result = ktxTexture_IterateLoadLevelFaces((ktxTexture*)ktx_texture, &IterateKtxImage, &image_ref);
        }
        if(result != KTX_SUCCESS)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Unable to load image `%s': %s", asset_file, ktxErrorString(result));
        image_ref->format = vk_to_dxgi(ktx_texture->vkFormat);
        image_ref->width = ktx_texture->baseWidth;
        image_ref->height = ktx_texture->baseHeight;
        if(image_ref->format == DXGI_FORMAT_UNKNOWN)
            image_ref->format = GetImageFormat(*image_ref);
        image_ref->flags = (image_ref->channel_count != 4 ? 0 : kGfxImageFlag_HasAlphaChannel);
        image_ref->flags |= (ktx_texture->numLevels > 1 ? kGfxImageFlag_HasMipLevels : 0);

        GfxMetadata &image_metadata = image_metadata_[image_ref];
        image_metadata.asset_file = asset_file; // set up metadata
        image_metadata.object_name = file;
        ktxTexture_Destroy((ktxTexture*)ktx_texture);
        return kGfxResult_NoError;
    }

    GfxResult importExr(GfxScene const& scene, char const* asset_file)
    {
        GFX_ASSERT(asset_file != nullptr);
        if(gfxSceneFindObjectByAssetFile<GfxImage>(scene, asset_file))
            return kGfxResult_NoError; // image was already imported
        EXRVersion exr_version;
        int32_t    ret = ParseEXRVersionFromFile(&exr_version, asset_file);
        if(ret != 0)
        {
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Invalid EXR file `%s'", asset_file);
        }
        if(exr_version.multipart)
        {
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Multi-part EXR files are not supported `%s'", asset_file);
        }
        if(exr_version.tiled)
        {
            return GFX_SET_ERROR(
                kGfxResult_InvalidOperation, "Tiled EXR files are not supported `%s'", asset_file);
        }

        EXRHeader exr_header;
        InitEXRHeader(&exr_header);
        char const *err = nullptr;
        ret             = ParseEXRHeaderFromFile(&exr_header, &exr_version, asset_file, &err);
        if(ret != 0)
        {
            const auto retError = GFX_SET_ERROR(kGfxResult_InvalidOperation, "Unable to load image header `%s': %s", asset_file, err);
            FreeEXRErrorMessage(err); // free's buffer for an error message
            return retError;
        }

        // Force always loading data as float
        for(int32_t i = 0; i < exr_header.num_channels; ++i)
        {
            if(exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF
                || exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_UINT)
            {
                exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
            }
        }

        EXRImage exr_image;
        InitEXRImage(&exr_image);
        ret = LoadEXRImageFromFile(&exr_image, &exr_header, asset_file, &err);
        if(ret != 0)
        {
            auto const retError =
                GFX_SET_ERROR(kGfxResult_InvalidOperation, "Unable to load image `%s': %s", asset_file, err);
            FreeEXRHeader(&exr_header);
            FreeEXRErrorMessage(err); // free's buffer for an error message
            return retError;
        }
        uint32_t const resolved_channel_count =
            (uint32_t)(exr_image.num_channels != 3 ? exr_image.num_channels : 4);
        char const    *file                   = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '\\'));
        file = (file == nullptr ? asset_file : file + 1); // retrieve file name
        size_t const image_data_size =
            (size_t)exr_image.width * exr_image.height * resolved_channel_count * 4;
        GfxRef<GfxImage> image_ref = gfxSceneCreateImage(scene);
        image_ref->data.resize(image_data_size);
        image_ref->width             = (uint32_t)exr_image.width;
        image_ref->height            = (uint32_t)exr_image.height;
        image_ref->channel_count     = resolved_channel_count;
        image_ref->bytes_per_channel = 4;
        image_ref->format            = GetImageFormat(*image_ref);
        float *data                  = reinterpret_cast<float *>(image_ref->data.data());
        bool   alpha_check           = false; // check alpha
        for(int32_t y = 0; y < exr_image.height; ++y)
            for(int32_t x = 0; x < exr_image.width; ++x)
            {
                int32_t const src_index = x + y * exr_image.width;
                for(int32_t k = 0; k < (int32_t)resolved_channel_count; ++k)
                {
                    // EXR stores channels in alphabetical order ABGR
                    int32_t const k_mod     = exr_image.num_channels - 1 - k;
                    int32_t const dst_index = (int32_t)resolved_channel_count * (x + y * exr_image.width) + k;
                    float const   source    = (k < exr_image.num_channels
                                                   ? reinterpret_cast<float **>(exr_image.images)[k_mod][src_index]
                                                   : 1.0f);
                    if(k == 3) alpha_check |= source < 1.0f;
                    data[dst_index] = source;
                }
            }
        image_ref->flags            = (!alpha_check ? 0 : kGfxImageFlag_HasAlphaChannel);
        GfxMetadata &image_metadata = image_metadata_[image_ref];
        image_metadata.asset_file   = asset_file; // set up metadata
        image_metadata.object_name  = file;
        FreeEXRImage(&exr_image);
        FreeEXRHeader(&exr_header);
        return kGfxResult_NoError;
    }

    GfxResult importImage(GfxScene const &scene, char const *asset_file, void const *memory = nullptr, size_t size = 0)
    {
        GFX_ASSERT(asset_file != nullptr);
        int32_t image_width = 0, image_height = 0, channel_count = 0;
        if(gfxSceneFindObjectByAssetFile<GfxImage>(scene, asset_file))
            return kGfxResult_NoError;  // image was already imported
        stbi_uc *image_data = nullptr;
        uint32_t bytes_per_channel = 2;
        if(memory == nullptr)
        {
            if(stbi_is_16_bit(asset_file))
                image_data =
                    (stbi_uc *)stbi_load_16(asset_file, &image_width, &image_height, &channel_count, 0);
            if(image_data == nullptr)
            {
                image_data        = stbi_load(asset_file, &image_width, &image_height, &channel_count, 0);
                bytes_per_channel = 1;
            }
        }
        else
        {
            if(stbi_is_16_bit_from_memory((stbi_uc *)memory, (int32_t)size))
                image_data = (stbi_uc *)stbi_load_16_from_memory(
                    (stbi_uc *)memory, (int32_t)size, &image_width, &image_height, &channel_count, 0);
            if(image_data == nullptr)
            {
                image_data = stbi_load_from_memory(
                    (stbi_uc *)memory, (int32_t)size, &image_width, &image_height, &channel_count, 0);
                bytes_per_channel = 1;
            }
        }
        if(image_data == nullptr)
            return GFX_SET_ERROR(kGfxResult_InvalidOperation, "Unable to load image `%s': %s", asset_file, stbi_failure_reason());
        uint32_t const resolved_channel_count = (uint32_t)(channel_count != 3 ? channel_count : 4);
        char const *file = GFX_MAX(strrchr(asset_file, '/'), strrchr(asset_file, '\\'));
        file = (file == nullptr ? asset_file : file + 1);   // retrieve file name
        size_t const image_data_size = (size_t)image_width * image_height * resolved_channel_count * bytes_per_channel;
        GfxRef<GfxImage> image_ref = gfxSceneCreateImage(scene);
        image_ref->data.resize(image_data_size);
        image_ref->width = (uint32_t)image_width;
        image_ref->height = (uint32_t)image_height;
        image_ref->channel_count = resolved_channel_count;
        image_ref->bytes_per_channel = bytes_per_channel;
        image_ref->format = GetImageFormat(*image_ref);
        if(bytes_per_channel == 1)
        {
            uint8_t *data = image_ref->data.data();
            uint8_t alpha_check = 255;  // check alpha
            for(int32_t y = 0; y < image_height; ++y)
                for(int32_t x = 0; x < image_width; ++x)
                    for(int32_t k = 0; k < (int32_t)resolved_channel_count; ++k)
                    {
                        int32_t const dst_index = (int32_t)resolved_channel_count * (x + y * image_width) + k;
                        int32_t const src_index = channel_count * (x + y * image_width) + k;
                        uint8_t const source = (k < channel_count ? image_data[src_index] : (uint8_t)255);
                        if(k == 3) alpha_check &= source;
                        data[dst_index] = source;
                    }
            image_ref->flags = (alpha_check == 255 ? 0 : kGfxImageFlag_HasAlphaChannel);
        }
        else
        {
            uint16_t *data = (uint16_t*)image_ref->data.data();
            uint16_t alpha_check = 65535;   // check alpha
            for(int32_t y = 0; y < image_height; ++y)
                for(int32_t x = 0; x < image_width; ++x)
                    for(int32_t k = 0; k < (int32_t)resolved_channel_count; ++k)
                    {
                        int32_t const dst_index = (int32_t)resolved_channel_count * (x + y * image_width) + k;
                        int32_t const src_index = channel_count * (x + y * image_width) + k;
                        uint16_t const source = (k < channel_count ? image_data[src_index] : (uint16_t)65535);
                        if(k == 3) alpha_check &= source;
                        data[dst_index] = source;
                    }
            image_ref->flags = (alpha_check == 65535 ? 0 : kGfxImageFlag_HasAlphaChannel);
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

GfxResult gfxSceneDestroyAllAnimations(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->clearObjects<GfxAnimation>();
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

bool gfxSceneSetAnimationMetadata(GfxScene scene, uint64_t animation_handle, GfxMetadata const &metadata)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return false;    // invalid parameter
    return gfx_scene->setObjectMetadata<GfxAnimation>(animation_handle, metadata);
}

GfxRef<GfxSkin> gfxSceneCreateSkin(GfxScene scene)
{
    GfxRef<GfxSkin> const skin_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return skin_ref;    // invalid parameter
    return gfx_scene->createObject<GfxSkin>(scene);
}

GfxResult gfxSceneDestroySkin(GfxScene scene, uint64_t skin_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->destroyObject<GfxSkin>(skin_handle);
}

GfxResult gfxSceneDestroyAllSkins(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->clearObjects<GfxSkin>();
}

uint32_t gfxSceneGetSkinCount(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0;    // invalid parameter
    return gfx_scene->getObjectCount<GfxSkin>();
}

GfxSkin const *gfxSceneGetSkins(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObjects<GfxSkin>();
}

GfxSkin *gfxSceneGetSkin(GfxScene scene, uint64_t skin_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObject<GfxSkin>(skin_handle);
}

GfxRef<GfxSkin> gfxSceneGetSkinHandle(GfxScene scene, uint32_t skin_index)
{
    GfxRef<GfxSkin> const skin_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return skin_ref;    // invalid parameter
    return gfx_scene->getObjectHandle<GfxSkin>(scene, skin_index);
}

GfxMetadata const &gfxSceneGetSkinMetadata(GfxScene scene, uint64_t skin_handle)
{
    static GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxSkin>(skin_handle);
}

bool gfxSceneSetSkinMetadata(GfxScene scene, uint64_t skin_handle, GfxMetadata const &metadata)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return false;    // invalid parameter
    return gfx_scene->setObjectMetadata<GfxSkin>(skin_handle, metadata);
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

GfxResult gfxSceneDestroyAllCameras(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->clearObjects<GfxCamera>();
}

GfxResult gfxSceneSetActiveCamera(GfxScene scene, uint64_t camera_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->setActiveCamera(scene, camera_handle);
}

GfxRef<GfxCamera> gfxSceneGetActiveCamera(GfxScene scene)
{
    GfxRef<GfxCamera> const camera_ref = {};
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

bool gfxSceneSetCameraMetadata(GfxScene scene, uint64_t camera_handle, GfxMetadata const &metadata)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return false;    // invalid parameter
    return gfx_scene->setObjectMetadata<GfxCamera>(camera_handle, metadata);
}

GfxRef<GfxLight> gfxSceneCreateLight(GfxScene scene)
{
    GfxRef<GfxLight> const light_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return light_ref;    // invalid parameter
    return gfx_scene->createObject<GfxLight>(scene);
}

GfxResult gfxSceneDestroyLight(GfxScene scene, uint64_t light_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->destroyObject<GfxLight>(light_handle);
}

GfxResult gfxSceneDestroyAllLights(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->clearObjects<GfxLight>();
}

uint32_t gfxSceneGetLightCount(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return 0;    // invalid parameter
    return gfx_scene->getObjectCount<GfxLight>();
}

GfxLight const *gfxSceneGetLights(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObjects<GfxLight>();
}

GfxLight *gfxSceneGetLight(GfxScene scene, uint64_t light_handle)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return nullptr;  // invalid parameter
    return gfx_scene->getObject<GfxLight>(light_handle);
}

GfxRef<GfxLight> gfxSceneGetLightHandle(GfxScene scene, uint32_t light_index)
{
    GfxRef<GfxLight> const light_ref = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return light_ref;    // invalid parameter
    return gfx_scene->getObjectHandle<GfxLight>(scene, light_index);
}

GfxMetadata const &gfxSceneGetLightMetadata(GfxScene scene, uint64_t light_handle)
{
    static GfxMetadata const metadata = {};
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return metadata; // invalid parameter
    return gfx_scene->getObjectMetadata<GfxLight>(light_handle);
}

bool gfxSceneSetLightMetadata(GfxScene scene, uint64_t light_handle, GfxMetadata const &metadata)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return false;    // invalid parameter
    return gfx_scene->setObjectMetadata<GfxLight>(light_handle, metadata);
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

GfxResult gfxSceneDestroyAllImages(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->clearObjects<GfxImage>();
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

bool gfxSceneSetImageMetadata(GfxScene scene, uint64_t image_handle, GfxMetadata const &metadata)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return false;    // invalid parameter
    return gfx_scene->setObjectMetadata<GfxImage>(image_handle, metadata);
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

GfxResult gfxSceneDestroyAllMaterials(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->clearObjects<GfxMaterial>();
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

bool gfxSceneSetMaterialMetadata(GfxScene scene, uint64_t material_handle, GfxMetadata const &metadata)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return false;    // invalid parameter
    return gfx_scene->setObjectMetadata<GfxMaterial>(material_handle, metadata);
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

GfxResult gfxSceneDestroyAllMeshes(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->clearObjects<GfxMesh>();
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

bool gfxSceneSetMeshMetadata(GfxScene scene, uint64_t mesh_handle, GfxMetadata const &metadata)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return false;    // invalid parameter
    return gfx_scene->setObjectMetadata<GfxMesh>(mesh_handle, metadata);
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

GfxResult gfxSceneDestroyAllInstances(GfxScene scene)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return kGfxResult_InvalidParameter;
    return gfx_scene->clearObjects<GfxInstance>();
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

bool gfxSceneSetInstanceMetadata(GfxScene scene, uint64_t instance_handle, GfxMetadata const &metadata)
{
    GfxSceneInternal *gfx_scene = GfxSceneInternal::GetGfxScene(scene);
    if(!gfx_scene) return false;    // invalid parameter
    return gfx_scene->setObjectMetadata<GfxInstance>(instance_handle, metadata);
}
