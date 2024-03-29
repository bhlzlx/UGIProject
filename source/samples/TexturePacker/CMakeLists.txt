project( SDFTest )

include_directories( ${CMAKE_SOURCE_DIR}/source )
include_directories( ${CMAKE_SOURCE_DIR}/thirdpart/include )
include_directories( ${CMAKE_SOURCE_DIR}/thirdpart/tweeny/include )

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

add_executable( SDFTest WIN32
    ${PROJECT_SOURCE}
    ${PROJECT_HEADER}
)

target_include_directories(SDFTest
PRIVATE
    ../../common
)

target_link_libraries(
    SDFTest
    CMCore
    CMAssetsManage
    CMPlatform
    MathGeoLib
    UGI
)