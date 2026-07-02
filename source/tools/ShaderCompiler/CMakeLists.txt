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

target_compile_definitions(ShaderCompiler PRIVATE
    SLANGC_PATH="${SOLUTION_DIR}/bin/slang/slangc.exe"
)

set_target_properties(ShaderCompiler PROPERTIES FOLDER "Tools")
