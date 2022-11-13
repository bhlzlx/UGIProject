project( SOURCE )

include_directories(
    ${SOLUTION_DIR}/source/common
    ${SOLUTION_DIR}/include
)

add_subdirectory( ugi )
add_subdirectory( gdi )
add_subdirectory( samples )
# add_subdirectory( utility )