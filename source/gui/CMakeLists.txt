cmake_minimum_required(VERSION 3.20)
project(gui)
# add_subdirectory(thirdpart/lwc)

add_library(gui STATIC)

target_compile_features(gui 
PRIVATE
    cxx_std_20
)

target_sources(gui 
PRIVATE 
    core/data_types/relation.cpp
    core/data_types/transition.cpp
    core/data_types/interpolatable_path.cpp
    core/data_types/tweener.cpp
    core/data_types/tween_types.cpp
    core/ease/ease.cpp
    core/events/event_dispatcher.cpp
    core/events/input_event.cpp
    core/input_handler.cpp
    core/package.cpp
    core/package_item.cpp
    core/gui_context.cpp
    core/hit_test.cpp
    utils/byte_buffer.cpp
    utils/toolset.cpp
    #
    core/display_objects/display_object.cpp
    core/display_objects/container.cpp
    # ui
    core/ui/component.cpp
    core/controller.cpp
    core/ui/object.cpp
    core/ui/object_factory.cpp
    core/ui/image.cpp
    render/ui_image_render.cpp
)

target_link_libraries(gui
PRIVATE
    UGI
    LightWeightCommon
    EnTT
)

target_include_directories(gui
PRIVATE
    .
    ${CMAKE_SOURCE_DIR}/thirdpart/glm
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(gui
PUBLIC
    $<$<STREQUAL:${CMAKE_CXX_BYTE_ORDER},BIG_ENDIAN>: BIG_ENDIAN=1>
)