include_guard(GLOBAL)

# -------------------------------------------------------------------
# Architecture detection
# -------------------------------------------------------------------
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_BITS 64)
else()
    set(ARCH_BITS 32)
endif()

# -------------------------------------------------------------------
# Target architecture identifier (used for library paths etc.)
# -------------------------------------------------------------------
if(NOT DEFINED TARGET_ARCH)
    if(ANDROID)
        set(TARGET_ARCH ${CMAKE_ANDROID_ARCH_ABI})
    elseif(IOS)
        set(TARGET_ARCH iOS)
    elseif(APPLE)
        set(TARGET_ARCH macOS)
    elseif(WIN32)
        if(CMAKE_CL_64 OR ARCH_BITS EQUAL 64)
            set(TARGET_ARCH x64)
        else()
            set(TARGET_ARCH x86)
        endif()
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        if(ARCH_BITS EQUAL 64)
            set(TARGET_ARCH x64)
        else()
            set(TARGET_ARCH x86)
        endif()
    else()
        set(TARGET_ARCH unknown)
    endif()
endif()

# -------------------------------------------------------------------
# Compiler flags
# -------------------------------------------------------------------
if(ANDROID)
    # Android — Clang toolchain
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fdeclspec -g -Wall -pthread")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -g -fdeclspec -pthread")
        add_link_options("-Wl,--build-id=sha1")
    else()
        message(WARNING "Android build expects Clang (found: ${CMAKE_CXX_COMPILER_ID})")
    endif()

elseif(APPLE)
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -std=c++20 -g -Wall")
    set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} -std=c99 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++20 -O2 -Wunused-function")
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} -std=c99 -O2 -pthread")

elseif(WIN32)
    if(MSVC)
        message(STATUS "Compiler: MSVC")
        set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} /std:c++20 /bigobj")
        set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /std:c++20 /Od /bigobj")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /std:c++20 /bigobj /fp:fast /Gy /Oi /Oy /O2 /Ot /Zi /EHsc")
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    elseif(MINGW)
        message(STATUS "Compiler: MinGW")
        set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -std=c++20 -g -Wall -pthread")
        set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} -std=c99 -g -pthread")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++20 -O2 -pthread -Wunused-function")
        set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} -O2 -pthread")

        if(ARCH_BITS EQUAL 64)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -m64")
            set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -m64")
        elseif(ARCH_BITS EQUAL 32)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
        endif()

    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "Compiler: Clang (Windows)")
        set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -std=c++20 -g -Wall -pthread -Wno-unused-private-field -Wno-unused-function -Wno-nullability-completeness")
        set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} -std=c99 -g -pthread")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++20 -O2 -pthread -Wno-unused-private-field -Wno-unused-function -Wno-nullability-completeness")
        set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} -O2 -pthread")
    endif()

elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g -Wall -pthread")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -g -pthread")

endif()

message(STATUS "Target platform: ${CMAKE_SYSTEM_NAME} (${TARGET_ARCH})")
