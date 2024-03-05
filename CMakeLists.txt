cmake_minimum_required (VERSION 3.20)
project( UGI-Project )

set(CXX_STANDARD 20)

include(cmakeconf/compiler_conf.cmake)
include(cmakeconf/building_output.cmake)

# set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/CMCMakeModule )
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/UGICMakeModule )

set(WINDOW_SYSTEM Win32 )
message( "target platform : ${CMAKE_SYSTEM_NAME}")

set(SOLUTION_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

link_directories(
    ${SOLUTION_DIR}/lib/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH}
)

if( MSVC )
    set(THIRD_PART_LIB_DIR ${SOLUTION_DIR}/lib/${CMAKE_SYSTEM_NAME}/${TARGET_ARCH} )
endif()

add_subdirectory( thirdpart )

add_subdirectory( source )