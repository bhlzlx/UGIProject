project( UGI )

add_library( UGI STATIC )

target_include_directories( UGI
PUBLIC
    ${SOLUTION_DIR}/thirdpart/include
    ${SOLUTION_DIR}/thirdpart
    ${SOLUTION_DIR}/thirdpart/glm
    ${SOLUTION_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources( UGI 
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_function_declare.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_debug_configurator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/descriptor_binder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_pass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/swapchain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/semaphore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/descriptor_binder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/texture_util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/material_layout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource_pool/material_layout_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command_encoder/compute_cmd_encoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command_encoder/resource_cmd_encoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command_encoder/render_cmd_encoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asyncload/gpu_asyncload_item.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asyncload/gpu_asyncload_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helper/helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helper/pipeline_helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/flight_cycle_invoker.cpp
    #  allocators
    ${CMAKE_CURRENT_SOURCE_DIR}/descriptor_set_allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uniform_buffer_allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mesh_buffer_allocator.cpp
    # render components
    ${CMAKE_CURRENT_SOURCE_DIR}/render_components/pipeline_material.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_components/mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/render_components/renderable.cpp
)

target_link_libraries(UGI
PUBLIC
    LightWeightCommon
)

if( MSVC )
    add_definitions( -DWIN32_LEAN_AND_MEAN )
endif()