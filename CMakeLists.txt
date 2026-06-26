cmake_minimum_required (VERSION 3.17)
project( UGI-Project )

set(CXX_STANDARD 20)
set(SOLUTION_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

if( ANDROID )
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    add_subdirectory( thirdpart )
    add_subdirectory( source/ugi )
    add_subdirectory( source/samples/Android )
else()
    include(cmakeconf/compiler_conf.cmake)
    include(cmakeconf/building_output.cmake)

    set(WINDOW_SYSTEM Win32 )
    message( "target platform : ${CMAKE_SYSTEM_NAME}")

    link_directories(
        ${SOLUTION_DIR}/lib/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH}
    )

    if( MSVC )
        set(THIRD_PART_LIB_DIR ${SOLUTION_DIR}/lib/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH} )
    endif()

    add_subdirectory( thirdpart )
    add_subdirectory( source )
endif()