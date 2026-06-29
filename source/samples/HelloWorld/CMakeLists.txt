if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

add_executable(HelloWorld WIN32)

target_sources(HelloWorld
PRIVATE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld.cpp
)

target_link_libraries( HelloWorld
PRIVATE
    UGI
    LightWeightCommon
)

target_include_directories(HelloWorld
PRIVATE
    ../../common
    ../../../bin/shaders/triangle
)

add_custom_command(TARGET HelloWorld PRE_BUILD
    COMMAND ${SOLUTION_DIR}/bin/ShaderCompiler.exe "${SOLUTION_DIR}/bin/shaders/triangle"
    COMMENT "ShaderCompiler: HelloWorld"
)