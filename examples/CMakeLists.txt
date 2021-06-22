add_definitions(-DUSE_PIX
                -DGLM_FORCE_CTOR_INIT
                -DGLM_FORCE_XYZW_ONLY
                -DGLM_FORCE_DEPTH_ZERO_TO_ONE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/00-hellotriangle)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/01-rtao)
