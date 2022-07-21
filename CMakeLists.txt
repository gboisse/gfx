cmake_minimum_required(VERSION 3.10.0)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm-0.9.9.8)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader-2.0-rc1)

option(GFX_BUILD_EXAMPLES "Build gfx examples" ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(gfx STATIC gfx.cpp)

set(GFX_DXC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc_2022_07_18)
set(GFX_PIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/WinPixEventRuntime-1.0.210209001)

target_sources(gfx PRIVATE gfx.h gfx_core.h gfx_imgui.h gfx_scene.h gfx_window.h)

target_include_directories(gfx PUBLIC ${GFX_DXC_PATH})
target_include_directories(gfx PUBLIC ${GFX_PIX_PATH}/include)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui-1.79)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf-2.5.0)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator-1.0.0/src)

target_link_libraries(gfx PUBLIC d3d12.lib dxgi.lib glm tinyobjloader
    ${GFX_DXC_PATH}/lib/x64/dxcompiler.lib
    ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.lib)

if(GFX_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()
