add_executable(03-mesh ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(03-mesh PUBLIC common)

set_target_properties(03-mesh PROPERTIES FOLDER "examples")

set_target_properties(03-mesh PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.task
    ${CMAKE_CURRENT_SOURCE_DIR}/*.mesh
    ${CMAKE_CURRENT_SOURCE_DIR}/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/*.vert)

target_sources(03-mesh PRIVATE ${SHADER_FILES})

source_group("Media Files\\Shaders" FILES ${SHADER_FILES})

set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

add_custom_command(
    TARGET 03-mesh
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
    TARGET 03-mesh
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
    TARGET 03-mesh
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
