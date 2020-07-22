set( UGI_FILES )

function( UGI_STATIC_LIBRARY NAME )
    project ( ${NAME} )
    set ( file_list )
    foreach( file ${ARGN} )
        set( file_list ${file_list} ${CMAKE_CURRENT_SOURCE_DIR}/${file} )
    endforeach()
    # message( ${file_list} )
    add_library( ${NAME} STATIC ${file_list} )
endfunction()

include_directories(
    ${SOLUTION_DIR}/thirdpart/include
    ${SOLUTION_DIR}/thirdpart/CMCore/inc
    ${SOLUTION_DIR}/thirdpart/CMPlatform/inc
    ${SOLUTION_DIR}/thirdpart/CMUtil/inc
    ${SOLUTION_DIR}/thirdpart/CMAssetsManage/inc
    ${SOLUTION_DIR}/source/utility
    ${SOLUTION_DIR}/source/thirdpart
    ${SOLUTION_DIR}/source/thirdpart/CMCore/inc
    ${SOLUTION_DIR}/source/thirdpart/CMUtil/inc
    ${SOLUTION_DIR}/source/thirdpart/CMPlatform/inc
)

if( MSVC )
    add_definitions( -DWIN32_LEAN_AND_MEAN )
endif()


include_directories( ${CMAKE_SOURCE_DIR}/source/thirdpart )

UGI_STATIC_LIBRARY( UGI
    VulkanFunctionDefine.cpp
    VulkanDebugConfigurator.cpp
    Device.cpp
    CommandQueue.cpp
    CommandBuffer.cpp
    ./commandBuffer/RenderCommandEncoder.cpp
    ./commandBuffer/ResourceCommandEncoder.cpp
    ArgumentGroupLayout.cpp
    ./resourcePool/ArgumentGroupLayoutPool.cpp
    Texture.cpp
    Buffer.cpp
    RenderPass.cpp
    Swapchain.cpp
    Semaphore.cpp
    ResourceManager.cpp
    Descriptor.cpp
    Argument.cpp
    Pipeline.cpp
    Drawable.cpp
    UniformBuffer.cpp
)