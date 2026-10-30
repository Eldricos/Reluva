cmake_minimum_required(VERSION 3.24)
project(reluva VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RELUVA_USE_SYSTEM_SDL "Use an installed SDL3 instead of building it" OFF)
option(RELUVA_BUILD_TESTS "Build unit tests" ON)

include(FetchContent)

# --- SDL3 -------------------------------------------------------------------
if(RELUVA_USE_SYSTEM_SDL)
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
add_library(reluva_warnings INTERFACE)
if(MSVC)
    target_compile_options(reluva_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(reluva_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# --- common: platform-independent core, no SDL allowed here -------------------
add_library(reluva_common STATIC
    src/common/config.cpp
    src/common/fs.cpp
    src/common/md5.cpp
    src/common/detection.cpp
    src/common/audio/decode.cpp
    src/common/audio/mixer.cpp
    src/common/audio/mp3.cpp
    src/common/audio/wav.cpp
    src/common/font.cpp
    src/common/text.cpp
    src/common/savestate.cpp
    src/common/video/bink.cpp
    src/common/video/bink_tables.cpp)
target_include_directories(reluva_common PUBLIC src)
# Third-party headers (third_party/, see each README) as system includes, so
# our warnings don't apply to them.
target_include_directories(reluva_common SYSTEM PRIVATE third_party/minimp3)
target_link_libraries(reluva_common PRIVATE reluva_warnings)

# --- engines: talk only to common/ --------------------------------------------
add_library(reluva_engines STATIC
    src/engines/registry.cpp
    src/engines/testbed/testbed.cpp
    src/engines/newworlds/newworlds.cpp
    src/engines/newworlds/cat_archive.cpp
    src/engines/newworlds/string_table.cpp
    src/engines/newworlds/texture.cpp
    src/engines/newworlds/geo.cpp
    src/engines/newworlds/gmf.cpp
    src/engines/newworlds/model_textures.cpp
    src/engines/newworlds/ground_textures.cpp
    src/engines/newworlds/mdl.cpp
    src/engines/newworlds/arf.cpp
    src/engines/newworlds/map_viewer.cpp
    src/engines/newworlds/mission_viewer.cpp
    src/engines/newworlds/mission_play.cpp
    src/engines/newworlds/mission_input.cpp
    src/engines/newworlds/mission_hud.cpp
    src/engines/newworlds/mission_briefing.cpp
    src/engines/newworlds/mission_flow.cpp
    src/engines/newworlds/mission_audio.cpp
    src/engines/newworlds/movie_player.cpp
    src/engines/newworlds/briefing.cpp
    src/engines/newworlds/briefing_data.cpp
    src/engines/newworlds/mission_script.cpp
    src/engines/newworlds/mission_scripts.cpp
    src/engines/newworlds/game_ui.cpp
    src/engines/newworlds/colony.cpp
    src/engines/newworlds/model_render.cpp
    src/engines/newworlds/star_system.cpp
    src/engines/newworlds/world.cpp
    src/engines/newworlds/model_pose.cpp
    src/engines/newworlds/economy.cpp
    src/engines/newworlds/start_data.cpp
    src/engines/newworlds/ai_data.cpp
    src/engines/newworlds/ai_sides.cpp
    src/engines/newworlds/field_work.cpp
    src/engines/newworlds/research.cpp
    src/engines/newworlds/script_effects.cpp
    src/engines/newworlds/fog.cpp
    src/engines/newworlds/repairs.cpp
    src/engines/newworlds/ground.cpp
    src/engines/newworlds/mission_sim.cpp
    src/engines/newworlds/placement.cpp
    src/engines/newworlds/model_viewer.cpp
    src/engines/newworlds/texture_viewer.cpp
    src/engines/newworlds/sound_viewer.cpp
    src/engines/newworlds/string_viewer.cpp
    src/engines/newworlds/fif.cpp
    src/engines/newworlds/font_viewer.cpp
    src/engines/newworlds/levels.cpp)
target_link_libraries(reluva_engines PUBLIC reluva_common PRIVATE reluva_warnings)
# The mission scripts (src/engines/newworlds/scripts/), built into the program.
include(cmake/EmbedFiles.cmake)
include(src/engines/newworlds/scripts/scripts.cmake)
reluva_embed_files(reluva_engines mission_scripts.inc
    BASE ${CMAKE_CURRENT_SOURCE_DIR}/src/engines/newworlds/scripts
    FILES ${RELUVA_MISSION_SCRIPTS})

# --- executable: SDL3 backend + ImGui launcher --------------------------------
add_executable(reluva
    src/main.cpp
    src/backends/sdl3/sdl_system.cpp
    src/launcher/launcher.cpp
    src/launcher/game_menu.cpp)
target_link_libraries(reluva PRIVATE reluva_engines imgui SDL3::SDL3 reluva_warnings)
target_compile_definitions(reluva PRIVATE RELUVA_VERSION="${PROJECT_VERSION}")

# --- developer tools -----------------------------------------------------------
add_executable(reluva-tool src/tools/reluva_tool.cpp)
target_link_libraries(reluva-tool PRIVATE reluva_engines reluva_warnings)
if(WIN32)
    target_link_libraries(reluva-tool PRIVATE shell32) # CommandLineToArgvW: UTF-8 arguments
endif()

# --- tests ----------------------------------------------------------------------
if(RELUVA_BUILD_TESTS)
    enable_testing()
    add_executable(reluva_tests tests/test_common.cpp)
    target_link_libraries(reluva_tests PRIVATE reluva_common reluva_warnings)
    target_include_directories(reluva_tests PRIVATE tests)
    add_test(NAME common COMMAND reluva_tests)

    add_executable(reluva_newworlds_tests tests/test_newworlds_cat.cpp)
    target_link_libraries(reluva_newworlds_tests PRIVATE reluva_engines reluva_warnings)
    add_test(NAME newworlds_cat COMMAND reluva_newworlds_tests)
endif()
