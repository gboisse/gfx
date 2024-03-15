add_executable(00-hellotriangle ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(00-hellotriangle PUBLIC gfx)

set_target_properties(00-hellotriangle PROPERTIES FOLDER "examples")

set_target_properties(00-hellotriangle PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/*.vert)

target_sources(00-hellotriangle PRIVATE ${SHADER_FILES})

source_group("Media Files\\Shaders" FILES ${SHADER_FILES})

set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

add_custom_command(TARGET 00-hellotriangle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:00-hellotriangle> $<TARGET_FILE_DIR:00-hellotriangle>
    COMMAND_EXPAND_LISTS
)
