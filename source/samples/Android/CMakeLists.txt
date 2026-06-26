# Android HelloWorld build
if(NOT SOLUTION_DIR)
    set(SOLUTION_DIR ${CMAKE_SOURCE_DIR}/../../..)
endif()

add_library(ugipbr SHARED
    ${SOLUTION_DIR}/source/platform/Android/AndroidMain.cpp
    ${SOLUTION_DIR}/source/samples/HelloWorld/HelloWorld.cpp
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

target_include_directories(ugipbr PRIVATE
    ${SOLUTION_DIR}/source
    ${SOLUTION_DIR}/source/common
    ${SOLUTION_DIR}/thirdpart/stb
    ${ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(ugipbr
    UGI
    LightWeightCommon
    android
    log
    GLESv3
)

# Assets: triangle shader + 两张贴图
set(SHADER_DIR ${SOLUTION_DIR}/bin/shaders/triangle)
set(IMAGE_DIR ${SOLUTION_DIR}/bin/image)
