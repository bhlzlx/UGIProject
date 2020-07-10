project( Sphere )

include_directories( ${CMAKE_SOURCE_DIR}/source )

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
)

set ( PROJECT_HEADER 
    ${CMAKE_CURRENT_SOURCE_DIR}/app.h
)

add_executable( Sphere WIN32
    ${PROJECT_SOURCE}
    ${PROJECT_HEADER}
)

target_link_libraries(
    Sphere
    UGI
    CMAssetsManage
    CMCore
    CMPlatform
    MathGeoLib
)