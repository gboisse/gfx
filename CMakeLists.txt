cmake_minimum_required(VERSION 3.10.0)

set(KTX_FEATURE_TESTS OFF)
set(KTX_FEATURE_VULKAN OFF)
set(KTX_FEATURE_GL_UPLOAD OFF)
set(KTX_FEATURE_TOOLS OFF)
set(KTX_FEATURE_DOC OFF)
set(KTX_FEATURE_STATIC_LIBRARY ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/KTX-Software)

option(GFX_BUILD_EXAMPLES "Build gfx examples" ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(gfx STATIC gfx.cpp)

set(GFX_DXC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc_2022_07_18)
set(GFX_PIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/WinPixEventRuntime-1.0.210209001)

set(GFX_DXC_PATH ${GFX_DXC_PATH} PARENT_SCOPE)
set(GFX_PIX_PATH ${GFX_PIX_PATH} PARENT_SCOPE)

target_sources(gfx PRIVATE gfx.h gfx_core.h gfx_imgui.h gfx_scene.h gfx_window.h)

target_compile_options(gfx PRIVATE /bigobj)

target_include_directories(gfx PUBLIC ${GFX_DXC_PATH})
target_include_directories(gfx PUBLIC ${GFX_PIX_PATH}/include)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cgltf)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/include)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/KTX-Software/include)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/include)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/src)

target_compile_options(ktx      PRIVATE "/wd4005")  # 'KTX_API': macro redefinition
target_compile_options(ktx      PRIVATE "/wd4244")  # '=': conversion from '__int64' to 'ktx_uint32_t', possible loss of data
target_compile_options(ktx      PRIVATE "/wd4267")  # 'initializing': conversion from 'size_t' to 'ktx_uint32_t', possible loss of data
target_compile_options(ktx_read PRIVATE "/wd4005")

target_compile_features(gfx PUBLIC cxx_std_14)
target_compile_options(gfx PRIVATE
    -DUSE_PIX
    /W3 /WX
    -D_HAS_EXCEPTIONS=0
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

target_link_libraries(gfx PUBLIC d3d12.lib dxgi.lib glm tinyobjloader ktx dxcompiler dxil WinPixEventRuntime)

if(GFX_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()
