add_executable(00-hellotriangle ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(00-hellotriangle PUBLIC common)

set_target_properties(00-hellotriangle PROPERTIES FOLDER "examples")

set_target_properties(00-hellotriangle PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/*.vert)

target_sources(00-hellotriangle PRIVATE ${SHADER_FILES})

source_group("Media Files\\Shaders" FILES ${SHADER_FILES})

set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

add_custom_command(
    TARGET 00-hellotriangle
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dxc_2021_07_01/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dxc_2021_07_01/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dxc_2021_07_01/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dxc_2021_07_01/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/dxil.dll)

add_custom_command(
    TARGET 00-hellotriangle
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dxc_2021_07_01/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dxc_2021_07_01/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dxc_2021_07_01/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/dxc_2021_07_01/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/dxcompiler.dll)

add_custom_command(
    TARGET 00-hellotriangle
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/WinPixEventRuntime-1.0.210209001/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/WinPixEventRuntime.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/WinPixEventRuntime-1.0.210209001/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/WinPixEventRuntime.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/WinPixEventRuntime-1.0.210209001/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/WinPixEventRuntime.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/WinPixEventRuntime-1.0.210209001/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/WinPixEventRuntime.dll)
