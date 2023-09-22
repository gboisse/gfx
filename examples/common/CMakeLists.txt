
add_library(common STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/fly_camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gpu_scene.cpp
)

target_sources(common PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fly_camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gpu_scene.h
)

file(GLOB SHADER_FILES CONFIGURE_DEPENDS${CMAKE_CURRENT_SOURCE_DIR}/*.hlsli)

target_sources(common PRIVATE ${SHADER_FILES})

source_group("Media Files\\Shaders" FILES ${SHADER_FILES})

set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

target_link_libraries(common PUBLIC gfx)

set_target_properties(common PROPERTIES FOLDER "examples")

target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
