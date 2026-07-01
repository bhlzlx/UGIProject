include_guard(GLOBAL)

# -------------------------------------------------------------------
# Base output directory
# -------------------------------------------------------------------
if(ANDROID)
    set(OUTPUT_BASE ${CMAKE_SOURCE_DIR}/bin/android/${ANDROID_ABI})
elseif(IOS)
    set(OUTPUT_BASE ${CMAKE_SOURCE_DIR}/bin/iOS/${IOS_PLATFORM})
else()
    set(OUTPUT_BASE ${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}_${ARCH_BITS})
endif()

# -------------------------------------------------------------------
# Per-configuration output directories
# -------------------------------------------------------------------
foreach(CONFIG IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${OUTPUT_BASE}_${CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG} ${OUTPUT_BASE}_${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG} ${OUTPUT_BASE}_${CONFIG})
endforeach()

# Fallback for unspecified / single-config generators
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BASE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BASE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${OUTPUT_BASE})

message(STATUS "Output directory: ${OUTPUT_BASE}")
