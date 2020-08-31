project( Samples )

##if(WIN32)
##    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
##    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
##    set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
##    set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
##endif()

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

if( MSVC )
    add_definitions( -DWIN32_LEAN_AND_MEAN )
endif()


include_directories(
    ${SOLUTION_DIR}/thirdpart/include
    ${SOLUTION_DIR}/thirdpart/CMCore/inc
    ${SOLUTION_DIR}/thirdpart/CMPlatform/inc
    ${SOLUTION_DIR}/thirdpart/CMUtil/inc
    ${SOLUTION_DIR}/thirdpart/CMAssetsManage/inc    
    ${SOLUTION_DIR}/thirdpart/MathGeoLib/src
)

set( EXECUTABLE_TYPE )

if( WIN32 )
    if( ${WINDOW_SYSTEM} STREQUAL SDL2 )
        set( PLATFORM_SOURCE
            ${SOLUTION_DIR}/source/platform/Windows/SDL2/SDL2Main.cpp
        )
    elseif( ${WINDOW_SYSTEM} STREQUAL Win32 )
        if( MINGW )
            set( PLATFORM_SOURCE 
                ${SOLUTION_DIR}/source/platform/Windows/WinMain/WinMain.cpp
                ${SOLUTION_DIR}/source/platform/Windows/WinMain/UGI-MINGW.rc
            )
        else()
            set( PLATFORM_SOURCE 
                ${SOLUTION_DIR}/source/platform/Windows/WinMain/WinMain.cpp
                ${SOLUTION_DIR}/source/platform/Windows/WinMain/UGI-MSVC.rc
            )
        endif()
        
    endif()
    set( EXECUTABLE_TYPE WIN32 )
endif()

if( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" )
    set( PLATFORM_SOURCE ${SOLUTION_DIR}/source/platform/Linux/LinuxMain.cpp )
endif()

if( ANDROID )
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -E")
    add_subdirectory( Android )
    SET_PROPERTY(TARGET AndroidSample PROPERTY FOLDER "Samples")
else()
    if( WIN32 )
    
        add_definitions(-DUNICODE -D_UNICODE)
        add_subdirectory( HelloWorld )
        add_subdirectory( Sphere )
        add_subdirectory( GDI )
        add_subdirectory( WidgetTest )
        add_subdirectory( MemTest )
        add_subdirectory( TexturePacker )
        add_subdirectory( DX11Test )
        
        SET_PROPERTY(TARGET HelloWorld PROPERTY FOLDER "Samples")
        SET_PROPERTY(TARGET Sphere PROPERTY FOLDER "Samples")
    endif()
endif()