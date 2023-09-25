
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/00-hellotriangle)
	
if(GFX_ENABLE_GUI AND GFX_ENABLE_SCENE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/01-rtao)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/02-pbr)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/03-mesh)
