cmake_minimum_required(VERSION 3.16)
project(PixLegends VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Try to find SDL2 packages with multiple methods
# Method 1: Try find_package first
find_package(SDL2 QUIET)
find_package(SDL2_image QUIET)
find_package(SDL2_ttf QUIET)
find_package(SDL2_mixer QUIET)

# Method 2: If not found, try to find SDL2 manually
if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found via find_package, trying manual detection...")
    
    # Common SDL2 installation paths
    set(SDL2_SEARCH_PATHS
        "C:/SDL2"
        "C:/Program Files/SDL2"
        "C:/Program Files (x86)/SDL2"
        "${CMAKE_SOURCE_DIR}/external/SDL2"
        "${CMAKE_SOURCE_DIR}/../SDL2"
    )
    
    # Look for SDL2.lib
    find_library(SDL2_LIBRARY
        NAMES SDL2
        PATHS ${SDL2_SEARCH_PATHS}
        PATH_SUFFIXES lib lib/x64 lib/x86
    )
    
    # Look for SDL2main.lib separately
    find_library(SDL2MAIN_LIBRARY
        NAMES SDL2main
        PATHS ${SDL2_SEARCH_PATHS}
        PATH_SUFFIXES lib lib/x64 lib/x86
    )
    
    find_path(SDL2_INCLUDE_DIR
        NAMES SDL.h
        PATHS ${SDL2_SEARCH_PATHS}
        PATH_SUFFIXES include include/SDL2
    )
    
    if(SDL2_LIBRARY AND SDL2_INCLUDE_DIR)
        set(SDL2_FOUND TRUE)
        set(SDL2_LIBRARIES ${SDL2_LIBRARY})
        set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
        message(STATUS "SDL2 found manually: ${SDL2_LIBRARY}")
    endif()
endif()

# Similar manual detection for SDL2_mixer
if(NOT SDL2_mixer_FOUND AND NOT SDL2_MIXER_FOUND)
    set(SDL2_MIXER_SEARCH_PATHS
        "C:/SDL2_mixer"
        "C:/Program Files/SDL2_mixer"
        "C:/Program Files (x86)/SDL2_mixer"
        "${CMAKE_SOURCE_DIR}/external/SDL2_mixer"
        "${CMAKE_SOURCE_DIR}/external/SDL2_mixer-2.6.3"
        "${CMAKE_SOURCE_DIR}/../SDL2_mixer"
    )
    find_library(SDL2_MIXER_LIBRARY NAMES SDL2_mixer PATHS ${SDL2_MIXER_SEARCH_PATHS} PATH_SUFFIXES lib lib/x64 lib/x86)
    find_path(SDL2_MIXER_INCLUDE_DIR NAMES SDL_mixer.h PATHS ${SDL2_MIXER_SEARCH_PATHS} PATH_SUFFIXES include include/SDL2)
    if(SDL2_MIXER_LIBRARY AND SDL2_MIXER_INCLUDE_DIR)
        set(SDL2_MIXER_FOUND TRUE)
        set(SDL2_MIXER_LIBRARIES ${SDL2_MIXER_LIBRARY})
        set(SDL2_MIXER_INCLUDE_DIRS ${SDL2_MIXER_INCLUDE_DIR})
        message(STATUS "SDL2_mixer found manually: ${SDL2_MIXER_LIBRARY}")
    endif()
endif()

# Similar manual detection for SDL2_image
if(NOT SDL2_IMAGE_FOUND)
    find_library(SDL2_IMAGE_LIBRARY
        NAMES SDL2_image
        PATHS ${SDL2_SEARCH_PATHS}
        PATH_SUFFIXES lib lib/x64 lib/x86
    )
    
    find_path(SDL2_IMAGE_INCLUDE_DIR
        NAMES SDL_image.h
        PATHS ${SDL2_SEARCH_PATHS}
        PATH_SUFFIXES include include/SDL2
    )
    
    if(SDL2_IMAGE_LIBRARY AND SDL2_IMAGE_INCLUDE_DIR)
        set(SDL2_IMAGE_FOUND TRUE)
        set(SDL2_IMAGE_LIBRARIES ${SDL2_IMAGE_LIBRARY})
        set(SDL2_IMAGE_INCLUDE_DIRS ${SDL2_IMAGE_INCLUDE_DIR})
        message(STATUS "SDL2_image found manually: ${SDL2_IMAGE_LIBRARY}")
    endif()
endif()

# Similar manual detection for SDL2_ttf
if(NOT SDL2_TTF_FOUND)
    find_library(SDL2_TTF_LIBRARY
        NAMES SDL2_ttf
        PATHS ${SDL2_SEARCH_PATHS}
        PATH_SUFFIXES lib lib/x64 lib/x86
    )
    
    find_path(SDL2_TTF_INCLUDE_DIR
        NAMES SDL_ttf.h
        PATHS ${SDL2_SEARCH_PATHS}
        PATH_SUFFIXES include include/SDL2
    )
    
    if(SDL2_TTF_LIBRARY AND SDL2_TTF_INCLUDE_DIR)
        set(SDL2_TTF_FOUND TRUE)
        set(SDL2_TTF_LIBRARIES ${SDL2_TTF_LIBRARY})
        set(SDL2_TTF_INCLUDE_DIRS ${SDL2_TTF_INCLUDE_DIR})
        message(STATUS "SDL2_ttf found manually: ${SDL2_TTF_LIBRARY}")
    endif()
endif()

# Check if all SDL2 components are found
if(NOT SDL2_FOUND OR NOT SDL2_IMAGE_FOUND OR NOT SDL2_TTF_FOUND)
    message(FATAL_ERROR "
SDL2 libraries not found! Please install SDL2 development libraries.

Installation options:
1. Download from https://github.com/libsdl-org/SDL/releases
2. Extract to C:/SDL2 or another directory
3. Make sure to include the following files:
   - SDL2.lib (or SDL2main.lib)
   - SDL2_image.lib  
   - SDL2_ttf.lib
   - Include headers in include/SDL2/

Alternatively, you can set SDL2_DIR environment variable to point to your SDL2 installation.
")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
include_directories(${SDL2_TTF_INCLUDE_DIRS})
if(SDL2_mixer_FOUND)
    include_directories(${SDL2_MIXER_INCLUDE_DIRS})
elseif(SDL2_MIXER_FOUND)
    include_directories(${SDL2_MIXER_INCLUDE_DIRS})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Database.cpp
    src/AssetManager.cpp
    src/InputManager.cpp
    src/Player.cpp
    src/Projectile.cpp
    src/Renderer.cpp
    src/World.cpp
    src/UISystem.cpp
    src/AudioManager.cpp
    src/Object.cpp
    src/Enemy.cpp
    src/AutotileDemo.cpp
)

# Create executable
add_executable(PixLegends ${SOURCES})

# Link libraries
target_link_libraries(PixLegends
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
)
if(SDL2_mixer_FOUND)
    target_link_libraries(PixLegends ${SDL2_mixer_LIBRARIES})
    target_compile_definitions(PixLegends PRIVATE USE_SDL_MIXER)
elseif(SDL2_MIXER_FOUND)
    target_link_libraries(PixLegends ${SDL2_MIXER_LIBRARIES})
    target_compile_definitions(PixLegends PRIVATE USE_SDL_MIXER)
endif()

# On Windows, we need to link against SDL2main for the main function
if(WIN32 AND SDL2MAIN_LIBRARY)
    target_link_libraries(PixLegends ${SDL2MAIN_LIBRARY})
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings - use console application for debugging
    set_target_properties(PixLegends PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    # Derive DLL directories from the resolved .lib locations and copy DLLs next to the exe
    if(SDL2_LIBRARY)
        get_filename_component(SDL2_LIB_DIR "${SDL2_LIBRARY}" DIRECTORY)
        set(SDL2_DLL "${SDL2_LIB_DIR}/SDL2.dll")
        if(EXISTS "${SDL2_DLL}")
            add_custom_command(TARGET PixLegends POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_DLL}" $<TARGET_FILE_DIR:PixLegends>
            )
        endif()
    endif()

    # Copy SDL2_mixer DLL to output if present
    if(SDL2_MIXER_LIBRARY)
        get_filename_component(SDL2_MIXER_LIB_DIR "${SDL2_MIXER_LIBRARY}" DIRECTORY)
        # Copy mixer DLL if present
        if(EXISTS "${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll")
            add_custom_command(TARGET PixLegends POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SDL2_MIXER_LIB_DIR}/SDL2_mixer.dll"
                $<TARGET_FILE_DIR:PixLegends>
            )
        endif()
        # Copy entire mixer lib dir (brings in codec DLLs too)
        add_custom_command(TARGET PixLegends POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SDL2_MIXER_LIB_DIR}"
            "$<TARGET_FILE_DIR:PixLegends>"
        )
    endif()

    if(SDL2_IMAGE_LIBRARY)
        get_filename_component(SDL2_IMAGE_LIB_DIR "${SDL2_IMAGE_LIBRARY}" DIRECTORY)
        set(SDL2_IMAGE_DLL "${SDL2_IMAGE_LIB_DIR}/SDL2_image.dll")
        if(EXISTS "${SDL2_IMAGE_DLL}")
            add_custom_command(TARGET PixLegends POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_IMAGE_DLL}" $<TARGET_FILE_DIR:PixLegends>
            )
        endif()
    endif()

    if(SDL2_TTF_LIBRARY)
        get_filename_component(SDL2_TTF_LIB_DIR "${SDL2_TTF_LIBRARY}" DIRECTORY)
        set(SDL2_TTF_DLL "${SDL2_TTF_LIB_DIR}/SDL2_ttf.dll")
        if(EXISTS "${SDL2_TTF_DLL}")
            add_custom_command(TARGET PixLegends POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_TTF_DLL}" $<TARGET_FILE_DIR:PixLegends>
            )
        endif()
    endif()

elseif(APPLE)
    # macOS-specific settings
    set_target_properties(PixLegends PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
    )
    
elseif(UNIX)
    # Linux-specific settings
    target_link_libraries(PixLegends m)
endif()

# Compiler-specific settings
if(MSVC)
    target_compile_options(PixLegends PRIVATE /W4)
else()
    target_compile_options(PixLegends PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(PixLegends PRIVATE DEBUG)
endif()

# Install target
install(TARGETS PixLegends
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# Copy assets to build directory
add_custom_command(TARGET PixLegends POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:PixLegends>/assets
)

# Print configuration info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "SDL2 found: ${SDL2_FOUND}")
message(STATUS "SDL2_image found: ${SDL2_IMAGE_FOUND}")
message(STATUS "SDL2_ttf found: ${SDL2_TTF_FOUND}")
message(STATUS "SDL2 library: ${SDL2_LIBRARY}")
message(STATUS "SDL2main library: ${SDL2MAIN_LIBRARY}")
message(STATUS "SDL2_mixer found: ${SDL2_mixer_FOUND}${SDL2_MIXER_FOUND}")
