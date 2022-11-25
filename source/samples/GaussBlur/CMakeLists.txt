add_executable(GaussBlur WIN32)

target_sources(GaussBlur
PRIVATE
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/GaussBlur.cpp
)

target_compile_options(GaussBlur
PRIVATE
    $<$<IN_LIST:${CMAKE_CXX_COMPILER_ID}, "GNU">: -municode>
)

target_include_directories(GaussBlur
PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/thirdpart/tweeny/include
)

target_link_libraries( GaussBlur
PRIVATE
    UGI
)