add_executable(gui_test WIN32)

target_sources(gui_test
PRIVATE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui_test.cpp
)

target_link_libraries(gui_test
PRIVATE
    UGI
    LightWeightCommon
)

target_compile_options(gui_test
PRIVATE
    $<$<
)