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
    ${CMAKE_CURRENT_SOURCE_DIR}/*.vert)

target_sources(02-pbr PRIVATE ${SHADER_FILES})

source_group("Media Files\\Shaders" FILES ${SHADER_FILES})

set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

add_custom_command(TARGET 02-pbr POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:02-pbr> $<TARGET_FILE_DIR:02-pbr>
    COMMAND_EXPAND_LISTS
)
