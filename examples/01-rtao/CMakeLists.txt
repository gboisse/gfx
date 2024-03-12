add_executable(01-rtao ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(01-rtao PUBLIC gfx)

set_target_properties(01-rtao PROPERTIES FOLDER "examples")

set_target_properties(01-rtao PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/*.vert)

target_sources(01-rtao PRIVATE ${SHADER_FILES})

source_group("Media Files\\Shaders" FILES ${SHADER_FILES})

set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

add_custom_command(TARGET 01-rtao POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:01-rtao> $<TARGET_FILE_DIR:01-rtao>
    COMMAND_EXPAND_LISTS
)
