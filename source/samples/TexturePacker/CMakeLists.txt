project( SDFTest )

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

add_executable( SDFTest WIN32 )

target_sources(SDFTest
PRIVATE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/TPTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SDFTextureTileManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SDFFontRenderer.cpp
)

target_include_directories(SDFTest
PRIVATE
    ../../common
    ../../../thirdpart/tweeny/include
    ../../../bin/shaders/SDF2D
)

add_custom_command(TARGET SDFTest PRE_BUILD
    COMMAND $<TARGET_FILE:ShaderCompiler> "${SOLUTION_DIR}/bin/shaders/SDF2D"
    COMMENT "ShaderCompiler: SDFTest"
)

target_compile_definitions(SDFTest PRIVATE NOMINMAX)

target_link_libraries(SDFTest
PRIVATE
    UGI
    LightWeightCommon
)
