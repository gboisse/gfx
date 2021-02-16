cmake_minimum_required(VERSION 3.10.0)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm-0.9.9.8)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader-2.0-rc1)

add_library(gfx STATIC gfx.cpp)

target_sources(gfx PRIVATE gfx.h gfx_core.h gfx_imgui.h gfx_scene.h gfx_window.h)

target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui-1.79)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinygltf-2.5.0)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc_2020_10-22/inc)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator-1.0.0/src)
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/WinPixEventRuntime-1.0.200127001/include)

target_link_libraries(gfx PUBLIC d3d12.lib dxgi.lib glm tinyobjloader
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxc_2020_10-22/lib/x64/dxcompiler.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/WinPixEventRuntime-1.0.200127001/bin/x64/WinPixEventRuntime.lib)
