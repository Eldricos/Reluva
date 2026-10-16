cmake_minimum_required(VERSION 3.24)
project(kestrel VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(KESTREL_USE_SYSTEM_SDL "Use an installed SDL3 instead of building it" OFF)
option(KESTREL_BUILD_TESTS "Build unit tests" ON)

include(FetchContent)

# --- SDL3 -------------------------------------------------------------------
if(KESTREL_USE_SYSTEM_SDL)
    find_package(SDL3 REQUIRED CONFIG)
else()
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.14
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(SDL3)
endif()

# --- Dear ImGui (no CMake support upstream, so build it here) -----------------
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9b
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

# --- Warnings -----------------------------------------------------------------
add_library(kestrel_warnings INTERFACE)
if(MSVC)
    target_compile_options(kestrel_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(kestrel_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# --- common: platform-independent core, no SDL allowed here -------------------
add_library(kestrel_common STATIC
    src/common/config.cpp
    src/common/fs.cpp
    src/common/md5.cpp
    src/common/detection.cpp)
target_include_directories(kestrel_common PUBLIC src)
target_link_libraries(kestrel_common PRIVATE kestrel_warnings)

# --- engines: talk only to common/ --------------------------------------------
add_library(kestrel_engines STATIC
    src/engines/registry.cpp
    src/engines/testbed/testbed.cpp
    src/engines/newworlds/newworlds.cpp)
target_link_libraries(kestrel_engines PUBLIC kestrel_common PRIVATE kestrel_warnings)

# --- executable: SDL3 backend + ImGui launcher --------------------------------
add_executable(kestrel
    src/main.cpp
    src/backends/sdl3/sdl_system.cpp
    src/launcher/launcher.cpp)
target_link_libraries(kestrel PRIVATE kestrel_engines imgui SDL3::SDL3 kestrel_warnings)

# --- tests ----------------------------------------------------------------------
if(KESTREL_BUILD_TESTS)
    enable_testing()
    add_executable(kestrel_tests tests/test_common.cpp)
    target_link_libraries(kestrel_tests PRIVATE kestrel_common kestrel_warnings)
    add_test(NAME common COMMAND kestrel_tests)
endif()
