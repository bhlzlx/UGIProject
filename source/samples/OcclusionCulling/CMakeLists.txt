add_subdirectory(MOCCapsule)
add_executable( OcclusionCulling WIN32
    ${PLATFORM_SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
)

target_include_directories( OcclusionCulling
PRIVATE
    ${CMAKE_SOURCE_DIR}/source
)

target_link_libraries( OcclusionCulling
PRIVATE
    UGI
    CMAssetsManage
    CMCore
    CMPlatform
    MathGeoLib
    OCCapsule
)