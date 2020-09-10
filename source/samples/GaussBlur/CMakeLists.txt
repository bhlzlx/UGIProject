project( GaussBlurTest )

include_directories( ${CMAKE_SOURCE_DIR}/source )
include_directories( ${CMAKE_SOURCE_DIR}/thirdpart/tweeny/include )

if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

set( PROJECT_SOURCE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/GaussBlur.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GaussBlurProcessor.cpp
)

set ( PROJECT_HEADER 
    ${CMAKE_CURRENT_SOURCE_DIR}/GaussBlur.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GaussBlurProcessor.h
)

add_executable( GaussBlurTest WIN32
    ${PROJECT_SOURCE}
    ${PROJECT_HEADER}
)

target_link_libraries(
    GaussBlurTest
    CMCore
    CMAssetsManage
    CMPlatform
    UGI
)