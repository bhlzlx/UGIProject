project(ShaderCompiler)

add_executable(ShaderCompiler
    ${CMAKE_CURRENT_SOURCE_DIR}/shaderCompilerMain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shaderCompilerUtility.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UGITypes.h
)

target_include_directories(ShaderCompiler
PRIVATE
    ${SOLUTION_DIR}/thirdpart/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ShaderCompiler
PRIVATE
    spirv-reflect-static
)

if(MSVC)
    target_compile_options(ShaderCompiler PRIVATE /utf-8)
endif()

set_target_properties(ShaderCompiler PROPERTIES FOLDER "Tools")
