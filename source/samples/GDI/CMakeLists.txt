project( GDITest )

include_directories( ${CMAKE_SOURCE_DIR}/source )

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/GDI.cpp
)

set ( PROJECT_HEADER 
    ${CMAKE_CURRENT_SOURCE_DIR}/GDI.h
)

add_executable( GDITest WIN32
    ${PROJECT_SOURCE}
    ${PROJECT_HEADER}
)

target_link_libraries(
    GDITest
    CMCore
    CMAssetsManage
    CMPlatform
    GDI
    UGI
    MathGeoLib
)