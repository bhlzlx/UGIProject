project( UGI )

add_library( UGI STATIC )

target_include_directories( UGI
    PRIVATE ${SOLUTION_DIR}/thirdpart/include
    PRIVATE ${SOLUTION_DIR}/thirdpart/CMCore/inc
    PRIVATE ${SOLUTION_DIR}/thirdpart/CMPlatform/inc
    PRIVATE ${SOLUTION_DIR}/thirdpart/CMUtil/inc
    PRIVATE ${SOLUTION_DIR}/thirdpart/CMAssetsManage/inc
    PRIVATE ${SOLUTION_DIR}/source
    PRIVATE ${SOLUTION_DIR}/source/utility
    PRIVATE ${SOLUTION_DIR}/source/thirdpart
    PRIVATE ${SOLUTION_DIR}/source/thirdpart/CMCore/inc
    PRIVATE ${SOLUTION_DIR}/source/thirdpart/CMUtil/inc
    PRIVATE ${SOLUTION_DIR}/source/thirdpart/CMPlatform/inc
)

target_sources( UGI 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/VulkanFunctionDefine.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/VulkanDebugConfigurator.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CommandQueue.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ArgumentGroupLayout.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Texture.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/TextureUtility.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Buffer.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/RenderPass.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Swapchain.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Semaphore.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ResourceManager.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Descriptor.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Argument.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Drawable.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/UniformBuffer.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CommandBuffer.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Sampler.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/resourcePool/ArgumentGroupLayoutPool.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffer/RenderCommandEncoder.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffer/ResourceCommandEncoder.cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffer/ComputeCommandEncoder.cpp
)

if( MSVC )
    add_definitions( -DWIN32_LEAN_AND_MEAN )
endif()