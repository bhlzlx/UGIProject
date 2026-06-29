# Android SDFTest build
if(NOT SOLUTION_DIR)
    set(SOLUTION_DIR ${CMAKE_SOURCE_DIR}/../../..)
endif()

add_library(ugipbr SHARED
    ${SOLUTION_DIR}/source/platform/Android/AndroidMain.cpp
    ${SOLUTION_DIR}/source/samples/TexturePacker/TPTest.cpp
    ${SOLUTION_DIR}/source/samples/TexturePacker/SDFFontRenderer.cpp
    ${SOLUTION_DIR}/source/samples/TexturePacker/SDFTextureTileManager.cpp
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(ugipbr PRIVATE
    ${SOLUTION_DIR}/source
    ${SOLUTION_DIR}/source/common
    ${SOLUTION_DIR}/thirdpart/stb
    ${SOLUTION_DIR}/thirdpart/tweeny/include
    ${SOLUTION_DIR}/bin/shaders/SDF2D
    ${ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(ugipbr
    UGI
    LightWeightCommon
    android
    log
    GLESv3
)
