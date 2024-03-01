if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

add_executable(fgui WIN32)

target_sources(fgui
PRIVATE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/fgui_test.cpp
)

target_link_libraries( fgui
PRIVATE
    UGI
    LightWeightCommon
)

target_include_directories(fgui
PRIVATE
    ../../common
)