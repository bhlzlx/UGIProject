SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

if(Linux)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

if( MINGW )
    add_definitions(-DWIN32)
endif()

include_directories( 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory( tweeny )
add_subdirectory( glm )
# add_subdirectory( tracy )
add_subdirectory( entt )
add_subdirectory( LightWeightCommon )

# SPIRV-Reflect for ShaderCompiler
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory( SPIRV-Reflect )