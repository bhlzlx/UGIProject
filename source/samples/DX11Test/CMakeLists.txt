project( DX11Test )

include_directories( ${CMAKE_SOURCE_DIR}/source )

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/DX11Test.cpp
)

set ( PROJECT_HEADER 
    ${CMAKE_CURRENT_SOURCE_DIR}/DX11Test.h
)

add_executable(DX11Test WIN32)

target_sources(DX11Test
PRIVATE
    ${PROJECT_SOURCE}
    ${CMAKE_SOURCE_DIR}/thirdpart/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/thirdpart/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/thirdpart/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/thirdpart/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/thirdpart/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/thirdpart/imgui/backends/imgui_impl_dx11.cpp
    ${CMAKE_SOURCE_DIR}/thirdpart/imgui/backends/imgui_impl_win32.cpp
)

target_link_libraries(
    DX11Test
    UGI
    dxgi
    d3d11
)

target_compile_definitions(DX11Test
PRIVATE
    USE_IMGUI
)

target_include_directories(DX11Test
PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdpart/imgui
)