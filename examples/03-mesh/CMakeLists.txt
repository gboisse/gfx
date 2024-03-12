add_executable(03-mesh ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(03-mesh PUBLIC gfx)

set_target_properties(03-mesh PROPERTIES FOLDER "examples")

set_target_properties(03-mesh PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.task
    ${CMAKE_CURRENT_SOURCE_DIR}/*.mesh
    ${CMAKE_CURRENT_SOURCE_DIR}/*.frag)

target_sources(03-mesh PRIVATE ${SHADER_FILES})

source_group("Media Files\\Shaders" FILES ${SHADER_FILES})

set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

add_custom_command(TARGET 03-mesh POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:03-mesh> $<TARGET_FILE_DIR:03-mesh>
    COMMAND_EXPAND_LISTS
)
