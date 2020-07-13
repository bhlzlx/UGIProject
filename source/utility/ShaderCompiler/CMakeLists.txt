project( ShaderCompiler )

include_directories( 
    ${CMAKE_SOURCE_DIR}/thirdpart/CMAssetsManage/inc
    ${CMAKE_SOURCE_DIR}/thirdpart/CMCore/inc
    ${CMAKE_SOURCE_DIR}/thirdpart/include
    ${CMAKE_SOURCE_DIR}/source
)

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

add_executable( ShaderCompiler 
    ${CMAKE_CURRENT_SOURCE_DIR}/shaderCompilerMain.cpp
)

if(MSVC)
target_link_libraries( ShaderCompiler 
    #
    general CMUtil
    general CMCore
    general CMPlatform
    general CMAssetsManage
    #
    optimized spirv-cross-core
    optimized glslang
    optimized OSDependent
    optimized OGLCompiler
    optimized MachineIndependent
    optimized GenericCodeGen
    optimized SPIRV
    #
    debug spirv-cross-cored
    debug glslangd
    debug OSDependentd
    debug OGLCompilerd
    debug MachineIndependentd
    debug GenericCodeGend
    debug SPIRVd
)
else()
target_link_libraries( ShaderCompiler 
    CMUtil
    CMAssetsManage
    CMCore
    CMPlatform
    glslang
    OSDependent
    OGLCompiler
    SPIRV
    spirv-cross-core
)
endif()

