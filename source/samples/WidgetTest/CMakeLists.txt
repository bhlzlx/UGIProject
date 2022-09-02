add_executable(WidgetTest WIN32
)

target_sources(WidgetTest
PRIVATE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgetTest.cpp
)

target_include_directories( WidgetTest
PRIVATE
    ${CMAKE_SOURCE_DIR}/source 
    ${CMAKE_SOURCE_DIR}/source/gdi 
    ${CMAKE_SOURCE_DIR}/thirdpart/tweeny/include 
)

target_compile_options(WidgetTest
PRIVATE
    $<$<CXX_COMPILER_ID:GNU>: -municode>
)

target_link_libraries( WidgetTest
PRIVATE
    GDI
    UGI
    CMAssetsManage
    CMCore
    CMPlatform
    MathGeoLib
    tweeny
)