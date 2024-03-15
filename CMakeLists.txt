cmake_minimum_required(VERSION 3.24.0)

project(gfx)

option(GFX_BUILD_EXAMPLES "Build gfx examples" ON)
option(GFX_ENABLE_GUI "Build gfx with imgui support" OFF)
option(GFX_ENABLE_SCENE "Build gfx with scene loading support" OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(gfx STATIC gfx.cpp gfx_window.cpp)

set(GFX_DXC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc)
set(GFX_PIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/WinPixEventRuntime)
set(GFX_AGILITY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/agility_sdk)

if(NOT PROJECT_IS_TOP_LEVEL)
    set(GFX_DXC_PATH ${GFX_DXC_PATH} PARENT_SCOPE)
    set(GFX_PIX_PATH ${GFX_PIX_PATH} PARENT_SCOPE)
    set(GFX_AGILITY_PATH ${GFX_AGILITY_PATH} PARENT_SCOPE)
endif()

target_sources(gfx PRIVATE gfx.h gfx_core.h gfx_window.h)
if(GFX_ENABLE_GUI)
    target_sources(gfx PRIVATE gfx_imgui.h gfx_imgui.cpp)
endif()
if(GFX_ENABLE_SCENE)
    target_sources(gfx PRIVATE gfx_scene.h gfx_scene.cpp)
endif()

target_compile_options(gfx PRIVATE /bigobj)

target_include_directories(gfx PRIVATE ${GFX_DXC_PATH})
target_include_directories(gfx PRIVATE ${GFX_PIX_PATH}/include)
target_include_directories(gfx PUBLIC ${GFX_AGILITY_PATH}/include)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gfx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/include)
target_include_directories(gfx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/src)

target_compile_features(gfx PUBLIC cxx_std_20)
target_compile_definitions(gfx PRIVATE USE_PIX)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -pedantic -Werror>)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX /experimental:external /external:anglebrackets /external:W0 /analyze:external->)
    target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/wd4324>) # structure was padded due to alignment specifier
    target_compile_definitions(gfx PRIVATE _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
        target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX>)
        target_compile_definitions(gfx PRIVATE _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -pedantic -Werror>)
    endif()
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(gfx PRIVATE -march=x86-64-v3)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(gfx PRIVATE /arch:AVX2)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        if("${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
            target_compile_options(gfx PRIVATE /arch:AVX2)
        else()
            target_compile_options(gfx PRIVATE -march=x86-64-v3)
        endif()
    endif()
endif()

add_library(D3D12Core SHARED IMPORTED)
set_target_properties(D3D12Core PROPERTIES
    IMPORTED_LOCATION ${GFX_AGILITY_PATH}/lib/D3D12Core.dll
    IMPORTED_IMPLIB ${GFX_AGILITY_PATH}/lib/dummy.lib
)
add_library(d3d12SDKLayers SHARED IMPORTED)
set_target_properties(d3d12SDKLayers PROPERTIES
    IMPORTED_LOCATION ${GFX_AGILITY_PATH}/lib/d3d12SDKLayers.dll
    IMPORTED_IMPLIB ${GFX_AGILITY_PATH}/lib/dummy.lib
)

add_library(dxcompiler SHARED IMPORTED)
set_target_properties(dxcompiler PROPERTIES
    IMPORTED_LOCATION ${GFX_DXC_PATH}/bin/x64/dxcompiler.dll
    IMPORTED_IMPLIB ${GFX_DXC_PATH}/lib/x64/dxcompiler.lib
)
add_library(dxil SHARED IMPORTED)
set_target_properties(dxil PROPERTIES
    IMPORTED_LOCATION ${GFX_DXC_PATH}/bin/x64/dxil.dll
    IMPORTED_IMPLIB ${GFX_DXC_PATH}/lib/x64/dxcompiler.lib
)

add_library(WinPixEventRuntime SHARED IMPORTED)
set_target_properties(WinPixEventRuntime PROPERTIES
    IMPORTED_LOCATION ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll
    IMPORTED_IMPLIB ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.lib
)

target_link_libraries(gfx PRIVATE d3d12 D3D12Core d3d12SDKLayers dxgi dxcompiler dxil WinPixEventRuntime)

if(GFX_ENABLE_GUI)
    include(FetchContent)

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.90.4
        SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/
    )
    FetchContent_MakeAvailable(imgui)

    target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
    target_compile_definitions(gfx PUBLIC GFX_ENABLE_GUI)
endif()
if(GFX_ENABLE_SCENE)
    option(GFX_ENABLE_SCENE_KTX "Build gfx with ktx image loading support" ON)

    include(FetchContent)

    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        ae721c50eaf761660b4f90cc590453cdb0c2acd0
        SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/
    )

    FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG        v1.13
        SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cgltf/
    )

    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.0
        SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm/
    )
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
        set(GLM_ENABLE_SIMD_AVX2 ON CACHE BOOL "")
    endif()
    set(GLM_ENABLE_CXX_17 ON CACHE BOOL "")
    set(GLM_QUIET ON CACHE BOOL "")

    FetchContent_Declare(
        tinyobjloader
        GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
        GIT_TAG        v2.0.0rc13
        SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader/
    )

    FetchContent_Declare(
        tinyexr
        GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
        GIT_TAG        v1.0.8
        SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyexr/
    )
    set(TINYEXR_BUILD_SAMPLE OFF CACHE BOOL "")
    set(TINYEXR_USE_MINIZ OFF CACHE BOOL "")

    if(GFX_ENABLE_SCENE_KTX)
        FetchContent_Declare(
            ktx
            GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
            GIT_TAG        v4.3.1
            SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/KTX-Software/
        )
        set(KTX_FEATURE_TESTS OFF CACHE BOOL "")
        set(KTX_FEATURE_VULKAN OFF CACHE BOOL "")
        set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "")
        set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "")
        set(KTX_FEATURE_TOOLS OFF CACHE BOOL "")
        set(KTX_FEATURE_DOC OFF CACHE BOOL "")
        set(KTX_FEATURE_STATIC_LIBRARY ON CACHE BOOL "")

        FetchContent_Declare(
            vulkan-headers
            GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
            GIT_TAG        v1.3.278
            SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/
        )
    endif()

    FetchContent_MakeAvailable(stb cgltf glm tinyobjloader tinyexr)
    if(GFX_ENABLE_SCENE_KTX)
        FetchContent_MakeAvailable(ktx vulkan-headers)
    endif()

    target_include_directories(gfx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
    target_include_directories(gfx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cgltf)
    target_include_directories(gfx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/include)
    target_include_directories(gfx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyexr)
    target_link_libraries(gfx PUBLIC glm::glm)
    target_link_libraries(gfx PRIVATE tinyobjloader tinyexr)
    if(GFX_ENABLE_SCENE_KTX)
        target_link_libraries(gfx PRIVATE ktx)
        target_compile_definitions(gfx PUBLIC GFX_ENABLE_SCENE_KTX)
    endif()

    target_compile_definitions(gfx PUBLIC GFX_ENABLE_SCENE)
    target_compile_definitions(gfx PRIVATE GLM_FORCE_XYZW_ONLY)

    #tinyexr cmake is currently broken when not using miniz
    target_compile_definitions(tinyexr PRIVATE TINYEXR_USE_MINIZ=0 TINYEXR_USE_STB_ZLIB=1)

    # Organize third party projects
    set_target_properties(glm PROPERTIES FOLDER "third_party")
    set_target_properties(tinyobjloader PROPERTIES FOLDER "third_party")
    set_target_properties(tinyexr PROPERTIES FOLDER "third_party")
    if(GFX_ENABLE_SCENE_KTX)
        set_target_properties(ktx PROPERTIES FOLDER "third_party")
        set_target_properties(astcenc-avx2-static PROPERTIES FOLDER "third_party/ktx_deps")
        set_target_properties(ktx_version PROPERTIES FOLDER "third_party/ktx_deps")
    endif()
endif()

if(GFX_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()
