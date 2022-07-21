add_executable(02-pbr
    ${CMAKE_CURRENT_SOURCE_DIR}/ibl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_sources(02-pbr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ibl.h)

target_link_libraries(02-pbr PUBLIC common)

set_target_properties(02-pbr PROPERTIES FOLDER "examples")

set_target_properties(02-pbr PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/*.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/*.vert)

target_sources(02-pbr PRIVATE ${SHADER_FILES})

source_group("Media Files\\Shaders" FILES ${SHADER_FILES})

set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

add_custom_command(
    TARGET 02-pbr
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/dxil.dll)

add_custom_command(
    TARGET 02-pbr
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/dxcompiler.dll)

add_custom_command(
    TARGET 02-pbr
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/WinPixEventRuntime.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/WinPixEventRuntime.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/WinPixEventRuntime.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/WinPixEventRuntime.dll)
