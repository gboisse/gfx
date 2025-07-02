cmake_minimum_required(VERSION 3.24.0)

project(gfx)

option(GFX_BUILD_EXAMPLES "Build gfx examples" ON)
option(GFX_ENABLE_GUI "Build gfx with imgui support" OFF)
option(GFX_ENABLE_SCENE "Build gfx with scene loading support" OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(gfx STATIC)

target_sources(gfx PRIVATE gfx.h gfx_core.h gfx_internal_types.h gfx_window.h gfx_core.cpp gfx_internal_types.cpp gfx.cpp gfx_window.cpp)
if(GFX_ENABLE_GUI)
    target_sources(gfx PRIVATE gfx_imgui.h gfx_imgui.cpp)
endif()
if(GFX_ENABLE_SCENE)
    target_sources(gfx PRIVATE gfx_scene.h gfx_scene.cpp)
endif()

target_compile_features(gfx PUBLIC cxx_std_20)
target_compile_definitions(gfx PRIVATE USE_PIX)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -pedantic -Werror>)
    target_compile_options(gfx PRIVATE -Wa,-mbig-obj)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX /experimental:external /external:anglebrackets /external:W0 /analyze:external->)
    target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/wd4324>) # structure was padded due to alignment specifier
    target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/wd6387>) # param could be 0
    target_compile_definitions(gfx PRIVATE _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)
    target_compile_options(gfx PRIVATE /bigobj)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
        target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX>)
        target_compile_options(gfx PRIVATE /bigobj)
    else()
        target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -pedantic -Werror>)
        target_compile_options(gfx PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-language-extension-token>) # anonymous types declared in an anonymous union
        target_compile_options(gfx PRIVATE -Wa,-mbig-obj)
    endif()
    include(CheckCXXSymbolExists)
    check_cxx_symbol_exists(_MSVC_STL_VERSION version MSVCSTL)
    if(MSVCSTL)
        target_compile_definitions(gfx PRIVATE _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)
    endif()
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(gfx PRIVATE -march=x86-64-v3)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(gfx PRIVATE /arch:AVX2)
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        if("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
            target_compile_options(gfx PRIVATE /arch:AVX2)
        else()
            target_compile_options(gfx PRIVATE -march=x86-64-v3)
        endif()
    endif()
endif()

include(FetchContent)
if(PROJECT_IS_TOP_LEVEL)
    set(GFX_TP_FOLDER "third_party")
else()
    set(GFX_TP_FOLDER "third_party/gfx_deps")
endif()

target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Find the latest windows SDK needed for d3d12 libs
if(NOT CMAKE_WINDOWS_KITS_10_DIR)
    set(CMAKE_WINDOWS_KITS_10_DIR "C:/Program Files (x86)/Windows Kits/10")
endif()
if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    STRING(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+" WINDOWS_KIT_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION})
else()
    set(WINDOWS_KIT_VERSION "10.0.0.0")
    file(GLOB _KITS RELATIVE ${CMAKE_WINDOWS_KITS_10_DIR}/Lib ${CMAKE_WINDOWS_KITS_10_DIR}/Lib/*)
    foreach(_KIT ${_KITS})
        if(${_KIT} VERSION_GREATER_EQUAL ${WINDOWS_KIT_VERSION})
            set(WINDOWS_KIT_VERSION ${_KIT})
        endif()
    endforeach()
endif()
set(WINDOWS_KIT_LIBRARY_DIR ${CMAKE_WINDOWS_KITS_10_DIR}/Lib/${WINDOWS_KIT_VERSION}/um/x64)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${WINDOWS_KIT_LIBRARY_DIR})
find_library(D3D12_LIB NAMES d3d12 REQUIRED)
find_library(DXGI_LIB NAMES dxgi REQUIRED)
target_link_libraries(gfx PRIVATE ${D3D12_LIB} ${DXGI_LIB})

set(GFX_D3D12MA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/")
FetchContent_Declare(
    D3D12MemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git
    GIT_TAG        v3.0.1
    SOURCE_DIR     "${GFX_D3D12MA_PATH}"
    FIND_PACKAGE_ARGS NAMES D3D12MemoryAllocator
)
FetchContent_MakeAvailable(D3D12MemoryAllocator)
if(NOT D3D12MemoryAllocator_FOUND)
    target_include_directories(gfx PRIVATE "${GFX_D3D12MA_PATH}/include")
    target_link_libraries(gfx PRIVATE D3D12MemoryAllocator)
    set_target_properties(D3D12MemoryAllocator PROPERTIES FOLDER "${GFX_TP_FOLDER}")
else()
    target_link_libraries(gfx PRIVATE GPUOpen::D3D12MemoryAllocator)
endif()

set(GFX_AGILITY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/directx12-agility")
set(GFX_AGILITY_VERSION 616)
set(GFX_AGILITY_MICRO_VERSION 0)
target_compile_definitions(gfx PRIVATE GFX_AGILITY_VERSION=${GFX_AGILITY_VERSION})
FetchContent_Declare(
    DirectX12-Agility
    URL            "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.${GFX_AGILITY_VERSION}.${GFX_AGILITY_MICRO_VERSION}"
    SOURCE_DIR     "${GFX_AGILITY_PATH}"
    FIND_PACKAGE_ARGS NAMES DirectX12-Agility
)
FetchContent_MakeAvailable(DirectX12-Agility)
if(NOT DirectX12-Agility_FOUND)
    add_library(D3D12Core SHARED IMPORTED)
    set_target_properties(D3D12Core PROPERTIES
        IMPORTED_LOCATION "${GFX_AGILITY_PATH}/build/native/bin/x64/D3D12Core.dll"
        IMPORTED_IMPLIB ${D3D12_LIB}
        INTERFACE_INCLUDE_DIRECTORIES "${GFX_AGILITY_PATH}/build/native/include"
    )
    add_library(d3d12SDKLayers SHARED IMPORTED)
    set_target_properties(d3d12SDKLayers PROPERTIES
        IMPORTED_LOCATION "${GFX_AGILITY_PATH}/build/native/bin/x64/d3d12SDKLayers.dll"
        IMPORTED_IMPLIB ${D3D12_LIB}
    )
    target_link_libraries(gfx PRIVATE D3D12Core d3d12SDKLayers)
else()
    find_package(directx-headers CONFIG REQUIRED)
    target_link_libraries(gfx PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX12-Agility)
endif()

set(GFX_DXC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/directx-dxc")
FetchContent_Declare(
    directx-dxc
    URL            "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/dxc_2024_07_31.zip"
    SOURCE_DIR     "${GFX_DXC_PATH}"
    FIND_PACKAGE_ARGS NAMES directx-dxc
)
FetchContent_MakeAvailable(directx-dxc)
if(NOT directx-dxc_FOUND)
    add_library(dxcompiler SHARED IMPORTED)
    set_target_properties(dxcompiler PROPERTIES
        IMPORTED_LOCATION "${GFX_DXC_PATH}/bin/x64/dxcompiler.dll"
        IMPORTED_IMPLIB "${GFX_DXC_PATH}/lib/x64/dxcompiler.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${GFX_DXC_PATH}/inc"
    )
    add_library(dxil SHARED IMPORTED)
    set_target_properties(dxil PROPERTIES
        IMPORTED_LOCATION "${GFX_DXC_PATH}/bin/x64/dxil.dll"
        IMPORTED_IMPLIB "${GFX_DXC_PATH}/lib/x64/dxcompiler.lib"
    )
    target_link_libraries(gfx PRIVATE dxcompiler dxil)
else()
    target_link_libraries(gfx PRIVATE Microsoft::DirectXShaderCompiler)
endif()

set(GFX_PIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/WinPixEventRuntime")
set(GFX_PIX_AMD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/AmdDxExt")
FetchContent_Declare(
    winpixevent
    URL            "https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.240308001"
    SOURCE_DIR     "${GFX_PIX_PATH}"
    FIND_PACKAGE_ARGS NAMES winpixevent
)
FetchContent_MakeAvailable(winpixevent)
if(NOT winpixevent_FOUND)
    file(READ "${GFX_PIX_PATH}/include/WinPixEventRuntime/PIXEvents.h" PIXE_FILE_CONTENTS)
    string(REPLACE "#define PIX_CONTEXT_EMIT_CPU_EVENTS" "# define PIX_CONTEXT_EMIT_CPU_EVENTS\n# include \"AmdPix3.h\"\n# define PIX_AMD_EXT\n" PIXE_FILE_CONTENTS "${PIXE_FILE_CONTENTS}")
    string(REPLACE " PIXBeginEventOnContextCpu(destination" " RgpPIXBeginEventOnContextCpu(destination" PIXE_FILE_CONTENTS "${PIXE_FILE_CONTENTS}")
    string(REPLACE " PIXSetMarkerOnContextCpu(destination" " RgpPIXSetMarkerOnContextCpu(destination" PIXE_FILE_CONTENTS "${PIXE_FILE_CONTENTS}")
    string(REPLACE "destination = PIXEndEventOnContextCpu(" "RgpPIXEndEventOnContextCpu(destination, " PIXE_FILE_CONTENTS "${PIXE_FILE_CONTENTS}")
    file(WRITE "${GFX_PIX_PATH}/include/WinPixEventRuntime/PIXEvents.h" "${PIXE_FILE_CONTENTS}")
    add_library(WinPixEventRuntime SHARED IMPORTED)
    set_target_properties(WinPixEventRuntime PROPERTIES
        IMPORTED_LOCATION "${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll"
        IMPORTED_IMPLIB "${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${GFX_PIX_PATH}/include/WinPixEventRuntime;${GFX_PIX_AMD_PATH}"
    )
    target_link_libraries(gfx PRIVATE WinPixEventRuntime)
else()
    target_include_directories(gfx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/AmdDxExt")
    target_link_libraries(gfx PRIVATE Microsoft::WinPixEventRuntime)
endif()

if(GFX_ENABLE_GUI)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.9
        SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/"
        FIND_PACKAGE_ARGS NAMES imgui
    )
    FetchContent_MakeAvailable(imgui)

    if(imgui_FOUND)
        target_link_libraries(gfx PUBLIC imgui::imgui)
    else()
        target_include_directories(gfx PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
        target_compile_definitions(gfx PRIVATE GFX_IMGUI_SOURCE)
    endif()
    target_compile_definitions(gfx PUBLIC GFX_ENABLE_GUI)
endif()
if(GFX_ENABLE_SCENE)
    target_compile_definitions(gfx PUBLIC GFX_ENABLE_SCENE)

    option(GFX_ENABLE_SCENE_KTX "Build gfx with ktx image loading support" ON)

    include(FetchContent)

    find_package(Stb QUIET)
    if(NOT Stb_FOUND)
        FetchContent_Declare(
            Stb
            GIT_REPOSITORY https://github.com/nothings/stb.git
            GIT_TAG        5c205738c191bcb0abc65c4febfa9bd25ff35234
            SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/"
        )
        FetchContent_MakeAvailable(Stb)
        target_include_directories(gfx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb")
    else()
        target_include_directories(gfx PRIVATE "${Stb_INCLUDE_DIR}")
    endif()

    find_path(CGLTF_INCLUDE_DIRS "cgltf.h")
    if("${CGLTF_INCLUDE_DIRS}" STREQUAL "CGLTF_INCLUDE_DIRS-NOTFOUND")
        FetchContent_Declare(
            cgltf
            GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
            GIT_TAG        v1.14
            SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/third_party/cgltf/"
        )
        FetchContent_MakeAvailable(cgltf)
        target_include_directories(gfx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/cgltf")
    else()
        target_include_directories(gfx PRIVATE "${CGLTF_INCLUDE_DIRS}")
    endif()

    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm/"
        FIND_PACKAGE_ARGS NAMES glm
    )
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
        set(GLM_ENABLE_SIMD_AVX2 ON CACHE BOOL "")
    endif()
    set(GLM_ENABLE_CXX_17 ON CACHE BOOL "")
    set(GLM_QUIET ON CACHE BOOL "")
    FetchContent_MakeAvailable(glm)
    target_compile_definitions(gfx PRIVATE GLM_FORCE_XYZW_ONLY)
    target_link_libraries(gfx PUBLIC glm::glm)
    if(NOT glm_FOUND)
        set_target_properties(glm PROPERTIES FOLDER "${GFX_TP_FOLDER}")
    endif()

    FetchContent_Declare(
        tinyobjloader
        GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
        GIT_TAG        v2.0.0rc13
        SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader/"
        FIND_PACKAGE_ARGS NAMES tinyobjloader
    )
    FetchContent_MakeAvailable(tinyobjloader)
    if(NOT tinyobjloader_FOUND)
        target_link_libraries(gfx PRIVATE tinyobjloader)
        set_target_properties(tinyobjloader PROPERTIES FOLDER "${GFX_TP_FOLDER}")
    else()
        target_link_libraries(gfx PRIVATE tinyobjloader::tinyobjloader)
    endif()

    FetchContent_Declare(
        tinyexr
        GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
        GIT_TAG        v1.0.10
        SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyexr/"
        FIND_PACKAGE_ARGS NAMES tinyexr
    )
    set(TINYEXR_BUILD_SAMPLE OFF CACHE BOOL "")
    set(TINYEXR_USE_MINIZ OFF CACHE BOOL "")
    FetchContent_MakeAvailable(tinyexr)
    if(NOT tinyexr_FOUND)
        #tinyexr cmake is currently broken when not using miniz
        target_compile_definitions(tinyexr PRIVATE TINYEXR_USE_MINIZ=0 TINYEXR_USE_STB_ZLIB=1)
        # Workaround compilation failures with current version
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND NOT "${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
            target_compile_options(tinyexr PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-sign-conversion -Wno-nonportable-system-include-path -Wno-tautological-type-limit-compare -Wno-old-style-cast>)
        endif()
        target_include_directories(gfx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyexr")
        target_link_libraries(gfx PRIVATE tinyexr)
        set_target_properties(tinyexr PROPERTIES FOLDER "${GFX_TP_FOLDER}")
    else()
        target_link_libraries(gfx PRIVATE unofficial::tinyexr::tinyexr)
    endif()

    if(GFX_ENABLE_SCENE_KTX)
        FetchContent_Declare(
            ktx
            GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
            GIT_TAG        v4.4.0
            SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/third_party/KTX-Software/"
            FIND_PACKAGE_ARGS NAMES Ktx
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
            GIT_TAG        v1.4.307
            SOURCE_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/"
            FIND_PACKAGE_ARGS NAMES VulkanHeaders
        )
        FetchContent_MakeAvailable(ktx vulkan-headers)
        target_link_libraries(gfx PRIVATE Vulkan::Headers)
        if(NOT ktx_FOUND)
            target_link_libraries(gfx PRIVATE ktx)
            set_target_properties(ktx PROPERTIES FOLDER "${GFX_TP_FOLDER}")
            set_target_properties(astcenc-avx2-static PROPERTIES FOLDER "${GFX_TP_FOLDER}/ktx_deps")
            set_target_properties(ktx_version PROPERTIES FOLDER "${GFX_TP_FOLDER}/ktx_deps")
        else()
            target_link_libraries(gfx PRIVATE KTX::ktx)
        endif()
        target_compile_definitions(gfx PRIVATE GFX_ENABLE_SCENE_KTX)
    endif()

endif()

if(GFX_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()
