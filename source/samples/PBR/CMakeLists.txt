if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

add_executable(PBR WIN32)

target_sources(PBR
PRIVATE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/PBRTest.cpp
)

target_link_libraries(PBR
PRIVATE
    UGI
    LightWeightCommon
)

target_include_directories(PBR
PRIVATE
    ../../common
)
