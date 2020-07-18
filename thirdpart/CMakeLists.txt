SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

if(Linux)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

if( MINGW )
    add_definitions(-DWIN32)
endif()

include_directories( 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/MathGeoLib/src
    ${CMAKE_CURRENT_SOURCE_DIR}/CMCore/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/CMUtil/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/CMPlatform/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/MathGeoLib/src
)

include(use_cm_module)

use_cm_module(Core)
use_cm_module(Platform)
use_cm_module(2D)
use_cm_module(Util)
use_cm_module(AssetsManage)
use_cm_module(SceneGraph)

add_subdirectory( MathGeoLib )
add_subdirectory( tweeny )

#SET_PROPERTY(TARGET kwheel_base PROPERTY FOLDER "Thirdpart")
