project( UGI )

add_library( UGI STATIC )

target_include_directories( UGI
PUBLIC
    ${SOLUTION_DIR}/thirdpart/include
    ${SOLUTION_DIR}/thirdpart
    ${SOLUTION_DIR}/source
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
    # ${CMAKE_CURRENT_SOURCE_DIR}/Drawable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UniformBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CommandBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resourcePool/ArgumentGroupLayoutPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffer/RenderCommandEncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffer/ResourceCommandEncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commandBuffer/ComputeCommandEncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_components/PipelineMaterial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_components/MeshPrimitive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_components/Renderable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/async_load/AsyncLoadManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/async_load/GPUAsyncLoadItem.cpp
)

target_link_libraries(UGI
PUBLIC
    LightWeightCommon
)

if( MSVC )
    add_definitions( -DWIN32_LEAN_AND_MEAN )
endif()