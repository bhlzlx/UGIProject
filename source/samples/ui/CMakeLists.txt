if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
    set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -municode")
endif()

add_executable(ui WIN32)

target_sources(ui
PRIVATE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/fgui_test.cpp
)

target_link_libraries(ui
PRIVATE
    UGI
    gui
    LightWeightCommon
)

target_include_directories(ui
PRIVATE
    ../../common
)