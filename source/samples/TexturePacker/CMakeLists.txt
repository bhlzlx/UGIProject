project( TPTest )

include_directories( ${CMAKE_SOURCE_DIR}/source )
include_directories( ${CMAKE_SOURCE_DIR}/thirdpart/include )

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/TPTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SDFTextureTileManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SDFFontRenderer.cpp
)

set ( PROJECT_HEADER 
    ${CMAKE_CURRENT_SOURCE_DIR}/TPTest.h
)

add_executable( TPTest WIN32
    ${PROJECT_SOURCE}
    ${PROJECT_HEADER}
)

target_link_libraries(
    TPTest
    CMCore
    CMAssetsManage
    CMPlatform
    MathGeoLib
    UGI
)