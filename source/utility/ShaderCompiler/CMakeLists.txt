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
    CMUtil
    CMCore
    CMPlatform
    CMAssetsManage
    spirv-cross-cored
    glslangd
    OSDependentd
    OGLCompilerd
    MachineIndependentd
    GenericCodeGend
    SPIRVd
)
else()
target_link_libraries( ShaderCompiler 
    CMUtil
    CMCore
    CMPlatform
    CMAssetsManage
    glslang
    OSDependent
    OGLCompiler
    SPIRV
    spirv-cross-core
)
endif()

