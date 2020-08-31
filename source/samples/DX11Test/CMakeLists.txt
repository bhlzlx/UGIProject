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

add_executable( DX11Test WIN32
    ${PROJECT_SOURCE}
    ${PROJECT_HEADER}
)

target_link_libraries(
    DX11Test
    CMCore
    CMAssetsManage
    CMPlatform
    UGI
    dxgi
    d3d11
)