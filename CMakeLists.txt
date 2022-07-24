cmake_minimum_required(VERSION 3.18)
project(storm-engine)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(STORM_EXTERN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern")

include(conan)
include(StormSetup)

option(STORM_ENABLE_CRASH_REPORTS "Enable automatic crash reports" OFF)
option(STORM_ENABLE_STEAM "Enable Steam integration" OFF)

### Set up output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)

set(STORM_WATERMARK_FILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/watermark.hpp CACHE FILEPATH "Include file containing build revision, etc." FORCE)

message(STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR})
if (CMAKE_SYSTEM_PROCESSOR MATCHES "e2k")
    set(PROJECT_PLATFORM_E2K TRUE)
endif()

if (PROJECT_PLATFORM_E2K)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -ffast -ffast-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${Spdlog_INCLUDE_DIR} -march=native -ffast -ffast-math")
endif()

### Set up third-party dependencies
set(ENV{CONAN_REVISIONS_ENABLED} 1)
conan_add_remote(NAME bincrafters
    URL https://bincrafters.jfrog.io/artifactory/api/conan/public-conan
    VERIFY_SSL True
)
conan_add_remote(NAME storm
    URL https://storm.jfrog.io/artifactory/api/conan/conan-remote
    VERIFY_SSL True
)
normalize_booleans(STORM_ENABLE_CRASH_REPORTS STORM_ENABLE_STEAM)
conan_cmake_run(CONANFILE conanfile.py
    BASIC_SETUP CMAKE_TARGETS
    BUILD missing
    OPTIONS
        output_directory=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        watermark_file=${STORM_WATERMARK_FILE}
        crash_reports=${STORM_ENABLE_CRASH_REPORTS}
        steam=${STORM_ENABLE_STEAM}
)

if (NOT WIN32)
    find_package(Spdlog REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${Spdlog_INCLUDE_DIR}")
    find_package(SDL REQUIRED)
    message(STATUS "SDL_LIBRARIES=${SDL_LIBRARIES}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${SDL_INCLUDE_DIR} -fsigned-char")
    
    find_package(OpenAL REQUIRED)
    find_package(OGGVORBIS REQUIRED)
    if (OPENAL_FOUND AND OGGVORBIS_FOUND)
        INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})
        INCLUDE_DIRECTORIES(${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${OPENAL_INCLUDE_DIR} -I${OGG_INCLUDE_DIR} -I${VORBIS_INCLUDE_DIR}")
    endif()
    
    find_package(SteamSdk REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${SteamSdk_INCLUDE_DIR}")

    message(STATUS "CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")

    message("Using DXVK-native for D3D9 API")

    include(ExternalProject)
    ExternalProject_Add(dxvk-native
        GIT_REPOSITORY    https://github.com/ugeen4/dxvk-native
        GIT_TAG           7a4dda1fdf0568d42708badcebc6e1aa487e6201
        GIT_SHALLOW       ON
        BUILD_ALWAYS      OFF
        CONFIGURE_HANDLED_BY_BUILD ON
        CONFIGURE_COMMAND meson --buildtype "release" ../dxvk-native
        BUILD_COMMAND     ninja
        INSTALL_COMMAND   ""
    )
    ExternalProject_Get_property(dxvk-native SOURCE_DIR BINARY_DIR)
    set(DXVK_NATIVE_INCLUDE_DIRS
        "${SOURCE_DIR}/include/native/directx"
        "${SOURCE_DIR}/include/native/windows"
    )
    set(DXVK_NATIVE_D3D9_LIB ${BINARY_DIR}/src/d3d9/libdxvk_d3d9.so)
    include_directories("${DXVK_NATIVE_INCLUDE_DIRS}")
    ADD_CUSTOM_TARGET(dependencies ALL DEPENDS dxvk-native)
endif()

### Define library ALIASes for use without CONAN_PKG:: prefix
foreach (conan_target ${CONAN_TARGETS})
    string(REPLACE "CONAN_PKG::" "" unprefixed_target ${conan_target})
	add_library(${unprefixed_target} ALIAS ${conan_target})
endforeach()
###

### Set up build scripts
set(SRC_DIRS "src" CACHE STRING "File locations for public header files" FORCE)
set(PUBLIC_INCLUDE_DIRS "include" CACHE STRING "File locations for source files" FORCE)
set(TESTSUITE_DIRS "testsuite" CACHE STRING "File locations for tests" FORCE)
set(RESOURCE_DIRS "rsrc" CACHE STRING "File locations for tests" FORCE)

set(SOURCE_FILE_EXTENSIONS "*.cpp" CACHE STRING "File extensions for source files" FORCE)
set(HEADER_FILE_EXTENSIONS "*.h" "*.hpp" CACHE STRING "File extensions for header files" FORCE)
###

add_subdirectory(src)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT engine)
