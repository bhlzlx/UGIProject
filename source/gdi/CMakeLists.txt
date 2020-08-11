project ( GDI )

add_definitions( -DWIN32_LEAN_AND_MEAN )

include_directories(
    ${SOLUTION_DIR}/source
    ${SOLUTION_DIR}/thirdpart/include
    ${SOLUTION_DIR}/thirdpart/CMCore/inc
    ${SOLUTION_DIR}/thirdpart/CMPlatform/inc
    ${SOLUTION_DIR}/thirdpart/CMUtil/inc
    ${SOLUTION_DIR}/thirdpart/CMAssetsManage/inc    
    ${SOLUTION_DIR}/thirdpart/MathGeoLib/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set( GDI_SOURCE 
    ${CMAKE_CURRENT_SOURCE_DIR}/gdi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/geometryDefine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/geometryBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/geometryDrawData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widget/widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widget/component.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widget/GdiSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/widget/name.cpp
)

add_library( GDI STATIC
    ${GDI_SOURCE}
)