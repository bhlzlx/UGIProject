project( UGI )

add_library( UGI STATIC )

target_include_directories( UGI
PRIVATE
    ${SOLUTION_DIR}/thirdpart/include
    ${SOLUTION_DIR}/source
    ${SOLUTION_DIR}/source/utility
    ${SOLUTION_DIR}/source/thirdpart
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources( UGI 
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanFunctionDefine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VulkanDebugConfigurator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CommandQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ArgumentGroupLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TextureUtility.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenderPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Swapchain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Semaphore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ResourceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Descriptor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Argument.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Drawable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UniformBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CommandBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resourcePool/ArgumentGroupLayoutPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffer/RenderCommandEncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffer/ResourceCommandEncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffer/ComputeCommandEncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_components/pipeline_material.cpp
)

if( MSVC )
    add_definitions( -DWIN32_LEAN_AND_MEAN )
endif()