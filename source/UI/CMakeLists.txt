project ( GDI )

include_directories(
    ${SOLUTION_DIR}/source
    ${SOLUTION_DIR}/thirdpart/include
    ${SOLUTION_DIR}/thirdpart/CMCore/inc
    ${SOLUTION_DIR}/thirdpart/CMPlatform/inc
    ${SOLUTION_DIR}/thirdpart/CMUtil/inc
    ${SOLUTION_DIR}/thirdpart/CMAssetsManage/inc    
    ${SOLUTION_DIR}/thirdpart/MathGeoLib/src
)

set( GDI_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/UI2D.cpp
)

add_library( GDI STATIC
    ${GDI_SOURCE}
)

